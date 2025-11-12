/*
 * Copyright (c) 2024 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_cordic

#include <errno.h>

#include <zephyr/drivers/cordic.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <stm32_ll_cordic.h>
#include <stm32_ll_bus.h>

#define LOG_LEVEL CONFIG_CORDIC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cordic_stm32);

/* Read-only driver configuration */
struct cordic_stm32_config {
	/* CORDIC instance */
	CORDIC_TypeDef *base;
	/* Clock configuration */
	struct stm32_pclken pclken;
	/* Interrupt configuration function */
	void (*irq_config_func)(const struct device *dev);
};

/* Runtime driver data */
struct cordic_stm32_data {
	/* Calculation configuration */
	struct cordic_config config;
	/* Semaphore for blocking operation */
	struct k_sem calc_complete;
	/* Error flag */
	volatile int error;
};

/* Mapping from Zephyr enum to STM32 LL defines */
static const uint32_t function_map[] = {
	[CORDIC_FUNC_COSINE] = LL_CORDIC_FUNCTION_COSINE,
	[CORDIC_FUNC_SINE] = LL_CORDIC_FUNCTION_SINE,
	[CORDIC_FUNC_PHASE] = LL_CORDIC_FUNCTION_PHASE,
	[CORDIC_FUNC_MODULUS] = LL_CORDIC_FUNCTION_MODULUS,
	[CORDIC_FUNC_ARCTANGENT] = LL_CORDIC_FUNCTION_ARCTANGENT,
	[CORDIC_FUNC_HCOSINE] = LL_CORDIC_FUNCTION_HCOSINE,
	[CORDIC_FUNC_HSINE] = LL_CORDIC_FUNCTION_HSINE,
	[CORDIC_FUNC_HARCTANGENT] = LL_CORDIC_FUNCTION_HARCTANGENT,
	[CORDIC_FUNC_NATURALLOG] = LL_CORDIC_FUNCTION_NATURALLOG,
	[CORDIC_FUNC_SQUAREROOT] = LL_CORDIC_FUNCTION_SQUAREROOT,
};

static const uint32_t precision_map[] = {
	[0] = 0,  /* Placeholder - precision starts at 1 */
	[CORDIC_PRECISION_1] = LL_CORDIC_PRECISION_1CYCLE,
	[CORDIC_PRECISION_2] = LL_CORDIC_PRECISION_2CYCLES,
	[CORDIC_PRECISION_3] = LL_CORDIC_PRECISION_3CYCLES,
	[CORDIC_PRECISION_4] = LL_CORDIC_PRECISION_4CYCLES,
	[CORDIC_PRECISION_5] = LL_CORDIC_PRECISION_5CYCLES,
	[CORDIC_PRECISION_6] = LL_CORDIC_PRECISION_6CYCLES,
	[CORDIC_PRECISION_7] = LL_CORDIC_PRECISION_7CYCLES,
	[CORDIC_PRECISION_8] = LL_CORDIC_PRECISION_8CYCLES,
	[CORDIC_PRECISION_9] = LL_CORDIC_PRECISION_9CYCLES,
	[CORDIC_PRECISION_10] = LL_CORDIC_PRECISION_10CYCLES,
	[CORDIC_PRECISION_11] = LL_CORDIC_PRECISION_11CYCLES,
	[CORDIC_PRECISION_12] = LL_CORDIC_PRECISION_12CYCLES,
	[CORDIC_PRECISION_13] = LL_CORDIC_PRECISION_13CYCLES,
	[CORDIC_PRECISION_14] = LL_CORDIC_PRECISION_14CYCLES,
	[CORDIC_PRECISION_15] = LL_CORDIC_PRECISION_15CYCLES,
};

static const uint32_t scale_map[] = {
	[CORDIC_SCALE_VAL_0] = LL_CORDIC_SCALE_0,
	[CORDIC_SCALE_VAL_1] = LL_CORDIC_SCALE_1,
	[CORDIC_SCALE_VAL_2] = LL_CORDIC_SCALE_2,
	[CORDIC_SCALE_VAL_3] = LL_CORDIC_SCALE_3,
	[CORDIC_SCALE_VAL_4] = LL_CORDIC_SCALE_4,
	[CORDIC_SCALE_VAL_5] = LL_CORDIC_SCALE_5,
	[CORDIC_SCALE_VAL_6] = LL_CORDIC_SCALE_6,
	[CORDIC_SCALE_VAL_7] = LL_CORDIC_SCALE_7,
};

/**
 * @brief IRQ handler for CORDIC
 */
static void cordic_stm32_isr(const struct device *dev)
{
	const struct cordic_stm32_config *cfg = dev->config;
	struct cordic_stm32_data *data = dev->data;
	CORDIC_TypeDef *cordic = cfg->base;

	/* Check if result is ready */
	if (LL_CORDIC_IsActiveFlag_RRDY(cordic)) {
		/* Signal completion */
		k_sem_give(&data->calc_complete);
	}
}

/**
 * @brief Configure CORDIC peripheral
 */
static int cordic_stm32_configure(const struct device *dev,
				  const struct cordic_config *config)
{
	const struct cordic_stm32_config *cfg = dev->config;
	struct cordic_stm32_data *data = dev->data;
	CORDIC_TypeDef *cordic = cfg->base;
	uint32_t function, precision, scale;
	uint32_t nb_write, nb_read;
	uint32_t in_size, out_size;

	if (!config) {
		return -EINVAL;
	}

	/* Validate and map function */
	if (config->function >= ARRAY_SIZE(function_map)) {
		LOG_ERR("Invalid function: %d", config->function);
		return -EINVAL;
	}
	function = function_map[config->function];

	/* Validate and map precision */
	if (config->precision < 1 || config->precision > 15) {
		LOG_ERR("Invalid precision: %d", config->precision);
		return -EINVAL;
	}
	precision = precision_map[config->precision];

	/* Validate and map scale */
	if (config->scale >= ARRAY_SIZE(scale_map)) {
		LOG_ERR("Invalid scale: %d", config->scale);
		return -EINVAL;
	}
	scale = scale_map[config->scale];

	/* Determine number of writes and reads based on function */
	switch (config->function) {
	case CORDIC_FUNC_COSINE:
	case CORDIC_FUNC_SINE:
		/* Input: angle, Output: sine and/or cosine */
		nb_write = LL_CORDIC_NBWRITE_1;
		nb_read = LL_CORDIC_NBREAD_2;
		break;
	case CORDIC_FUNC_PHASE:
	case CORDIC_FUNC_MODULUS:
		/* Input: x, y, Output: phase or modulus */
		nb_write = LL_CORDIC_NBWRITE_2;
		nb_read = LL_CORDIC_NBREAD_1;
		break;
	case CORDIC_FUNC_ARCTANGENT:
		/* Input: x, Output: arctan(x) */
		nb_write = LL_CORDIC_NBWRITE_1;
		nb_read = LL_CORDIC_NBREAD_1;
		break;
	case CORDIC_FUNC_HCOSINE:
	case CORDIC_FUNC_HSINE:
		/* Input: angle, Output: hcosine and/or hsine */
		nb_write = LL_CORDIC_NBWRITE_1;
		nb_read = LL_CORDIC_NBREAD_2;
		break;
	case CORDIC_FUNC_HARCTANGENT:
	case CORDIC_FUNC_NATURALLOG:
		/* Input: x, Output: result */
		nb_write = LL_CORDIC_NBWRITE_1;
		nb_read = LL_CORDIC_NBREAD_1;
		break;
	case CORDIC_FUNC_SQUAREROOT:
		/* Input: x, Output: sqrt(x) */
		nb_write = LL_CORDIC_NBWRITE_1;
		nb_read = LL_CORDIC_NBREAD_1;
		break;
	default:
		return -EINVAL;
	}

	/* Map data width */
	in_size = (config->in_size == CORDIC_DATA_WIDTH_32BIT) ?
		  LL_CORDIC_INSIZE_32BITS : LL_CORDIC_INSIZE_16BITS;
	out_size = (config->out_size == CORDIC_DATA_WIDTH_32BIT) ?
		   LL_CORDIC_OUTSIZE_32BITS : LL_CORDIC_OUTSIZE_16BITS;

	/* Configure CORDIC */
	LL_CORDIC_Config(cordic, function, precision, scale,
			 nb_write, nb_read, in_size, out_size);

	/* Save configuration */
	memcpy(&data->config, config, sizeof(struct cordic_config));

	LOG_DBG("CORDIC configured: func=%d, prec=%d, scale=%d",
		config->function, config->precision, config->scale);

	return 0;
}

/**
 * @brief Perform CORDIC calculation
 */
static int cordic_stm32_calculate(const struct device *dev,
				  int32_t *in_data,
				  int32_t *out_data,
				  uint32_t num_in,
				  uint32_t num_out)
{
	const struct cordic_stm32_config *cfg = dev->config;
	struct cordic_stm32_data *data = dev->data;
	CORDIC_TypeDef *cordic = cfg->base;
	int ret = 0;
	uint32_t timeout_ms = 1000;

	if (!in_data || !out_data) {
		return -EINVAL;
	}

	/* Validate input/output counts */
	if (num_in == 0 || num_in > 2 || num_out == 0 || num_out > 2) {
		return -EINVAL;
	}

	/* Reset semaphore */
	k_sem_reset(&data->calc_complete);
	data->error = 0;

	/* Enable interrupt */
	LL_CORDIC_EnableIT(cordic);

	/* Write input data */
	if (num_in >= 1) {
		LL_CORDIC_WriteData(cordic, in_data[0]);
	}
	if (num_in >= 2) {
		LL_CORDIC_WriteData(cordic, in_data[1]);
	}

	/* Wait for calculation to complete */
	ret = k_sem_take(&data->calc_complete, K_MSEC(timeout_ms));
	if (ret != 0) {
		LOG_ERR("CORDIC calculation timeout");
		LL_CORDIC_DisableIT(cordic);
		return -ETIMEDOUT;
	}

	/* Check for errors */
	if (data->error) {
		LOG_ERR("CORDIC calculation error");
		LL_CORDIC_DisableIT(cordic);
		return -EIO;
	}

	/* Read output data */
	if (num_out >= 1) {
		out_data[0] = (int32_t)LL_CORDIC_ReadData(cordic);
	}
	if (num_out >= 2) {
		/* Wait for second result */
		k_sem_reset(&data->calc_complete);
		ret = k_sem_take(&data->calc_complete, K_MSEC(timeout_ms));
		if (ret != 0) {
			LOG_ERR("CORDIC second result timeout");
			LL_CORDIC_DisableIT(cordic);
			return -ETIMEDOUT;
		}
		out_data[1] = (int32_t)LL_CORDIC_ReadData(cordic);
	}

	/* Disable interrupt */
	LL_CORDIC_DisableIT(cordic);

	LOG_DBG("CORDIC calculation completed");

	return 0;
}

static const struct cordic_driver_api cordic_stm32_api = {
	.configure = cordic_stm32_configure,
	.calculate = cordic_stm32_calculate,
};

/**
 * @brief Initialize CORDIC peripheral
 */
static int cordic_stm32_init(const struct device *dev)
{
	const struct cordic_stm32_config *cfg = dev->config;
	struct cordic_stm32_data *data = dev->data;
	int ret;

	/* Enable clock for CORDIC */
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken);
	if (ret < 0) {
		LOG_ERR("Failed to enable clock");
		return ret;
	}

	/* Initialize semaphore */
	k_sem_init(&data->calc_complete, 0, 1);

	/* Configure interrupt */
	cfg->irq_config_func(dev);

	LOG_INF("CORDIC device initialized");

	return 0;
}

/* Device instantiation macro */
#define STM32_CORDIC_INIT(index)						\
										\
	static void cordic_stm32_irq_config_func_##index(const struct device *dev); \
										\
	static const struct cordic_stm32_config cordic_stm32_config_##index = {	\
		.base = (CORDIC_TypeDef *)DT_INST_REG_ADDR(index),		\
		.pclken = {							\
			.bus = DT_INST_CLOCKS_CELL(index, bus),			\
			.enr = DT_INST_CLOCKS_CELL(index, bits),		\
		},								\
		.irq_config_func = cordic_stm32_irq_config_func_##index,	\
	};									\
										\
	static struct cordic_stm32_data cordic_stm32_data_##index;		\
										\
	DEVICE_DT_INST_DEFINE(index,						\
			      cordic_stm32_init,				\
			      NULL,						\
			      &cordic_stm32_data_##index,			\
			      &cordic_stm32_config_##index,			\
			      POST_KERNEL,					\
			      CONFIG_CORDIC_INIT_PRIORITY,			\
			      &cordic_stm32_api);				\
										\
	static void cordic_stm32_irq_config_func_##index(const struct device *dev) \
	{									\
		IRQ_CONNECT(DT_INST_IRQN(index),				\
			    DT_INST_IRQ(index, priority),			\
			    cordic_stm32_isr,					\
			    DEVICE_DT_INST_GET(index),				\
			    0);							\
		irq_enable(DT_INST_IRQN(index));				\
	}

DT_INST_FOREACH_STATUS_OKAY(STM32_CORDIC_INIT)
