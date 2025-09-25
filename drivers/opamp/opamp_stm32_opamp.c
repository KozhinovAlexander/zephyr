/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/opamp.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include <stm32_ll_opamp.h>
#include <stm32_ll_system.h>


LOG_MODULE_REGISTER(stm32_opamp, CONFIG_OPAMP_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_opamp

#define STM32_COMP_DT_POWER_MODE(inst)	\
	CONCAT(LL_OPAMP_POWERMODE_, DT_INST_STRING_TOKEN(inst, slew_rate))

struct stm32_opamp_config {
	OPAMP_TypeDef *opamp;
	uint8_t functional_mode;
	uint32_t power_mode;
	uint32_t input_inverting;
	uint32_t input_non_inverting;
	bool lock_enable;
	const struct stm32_pclken *pclken;
	size_t pclk_len;
	const struct pinctrl_dev_config *pincfg;
};

static bool stm32_opamp_is_resumed(const struct device *dev)
{
#if CONFIG_PM_DEVICE
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	return state == PM_DEVICE_STATE_ACTIVE;
#else
	return true;
#endif /* CONFIG_PM_DEVICE */
}

static int stm32_opamp_pm_callback(const struct device *dev, enum pm_device_action action)
{
	const struct stm32_opamp_config *cfg = dev->config;
	OPAMP_TypeDef *opamp = cfg->opamp;

	/// TODO: Not all series support OPAMP locking

	if (LL_OPAMP_IsLocked(opamp)) {
		return -EPERM;
	}

	if (action == PM_DEVICE_ACTION_RESUME) {
		LL_OPAMP_Enable(opamp);
		if(cfg->lock_enable) {
			LL_OPAMP_Lock(opamp);
		}
	}

	if (action == PM_DEVICE_ACTION_SUSPEND) {
		LL_OPAMP_Disable(opamp);
	}

	return 0;
}

static int stm32_opamp_set_functional_mode(const struct device *dev,
				enum opamp_functional_mode mode)
{
	const struct stm32_opamp_config *cfg = dev->config;
	OPAMP_TypeDef *opamp = cfg->opamp;
	uint32_t stm32_ll_functional_mode;

	switch (mode) {
	case OPAMP_FUNCTIONAL_MODE_DIFFERENTIAL:
		// cfg->functional_mode = mode;
		break;
	case OPAMP_FUNCTIONAL_MODE_INVERTING:
		// cfg->functional_mode = mode;
		break;
	case OPAMP_FUNCTIONAL_MODE_NON_INVERTING:
		// LL_OPAMP_MODE_PGA or LL_OPAMP_MODE_PGA_IO0
		// cfg->functional_mode = mode;
		break;
	case OPAMP_FUNCTIONAL_MODE_FOLLOWER:
		stm32_ll_functional_mode = LL_OPAMP_MODE_FOLLOWER;
		break;
	case OPAMP_FUNCTIONAL_MODE_STANDALONE:
		stm32_ll_functional_mode = LL_OPAMP_MODE_STANDALONE;
		break;
	default:
		LOG_ERR("%s: invalid functional mode: %d", dev->name, mode);
		return -EINVAL;
	}

	LL_OPAMP_SetFunctionalMode(opamp, stm32_ll_functional_mode);

	return 0;
}

static int stm32_opamp_set_gain(const struct device *dev, enum opamp_gain gain)
{
	const struct stm32_opamp_config *cfg = dev->config;
	OPAMP_TypeDef *opamp = cfg->opamp;
	uint32_t stm32_ll_gain;

	if (gain == OPAMP_GAIN_2 || gain == OPAMP_GAIN_1) {
		stm32_ll_gain = LL_OPAMP_PGA_GAIN_2_OR_MINUS_1;
	} else if (gain == OPAMP_GAIN_4 || gain == OPAMP_GAIN_3) {
		stm32_ll_gain = LL_OPAMP_PGA_GAIN_4_OR_MINUS_3;
	} else if (gain == OPAMP_GAIN_8 || gain == OPAMP_GAIN_7) {
		stm32_ll_gain = LL_OPAMP_PGA_GAIN_8_OR_MINUS_7;
	} else if (gain == OPAMP_GAIN_16 || gain == OPAMP_GAIN_15) {
		stm32_ll_gain = LL_OPAMP_PGA_GAIN_16_OR_MINUS_15;
	} else if (gain == OPAMP_GAIN_32 || gain == OPAMP_GAIN_31) {
		stm32_ll_gain = LL_OPAMP_PGA_GAIN_32_OR_MINUS_31;
	} else if (gain == OPAMP_GAIN_64 || gain == OPAMP_GAIN_63) {
		stm32_ll_gain = LL_OPAMP_PGA_GAIN_64_OR_MINUS_63;
	} else {
		LOG_ERR("%s: invalid gain value %d for mode %d", dev->name,
			gain, cfg->functional_mode);
		return -EINVAL;
	}

	LL_OPAMP_SetPGAGain(opamp, stm32_ll_gain);

	return 0;
}

static int stm32_opamp_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct stm32_opamp_config *cfg = dev->config;
	OPAMP_TypeDef *opamp = cfg->opamp;
	int ret = 0;

	ret = device_is_ready(clk);
	if (!ret) {
		LOG_ERR("%s clock control device not ready (%d)", dev->name, ret);
		return ret;
	}

	/* Enable OPAMP bus clock */
	ret = clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken[0]);
	if (ret != 0) {
		LOG_ERR("%s clock op failed (%d)", dev->name, ret);
		return ret;
	}

	/* Enable OPAMP clock source if provided */
	if (cfg->pclk_len > 1) {
		ret = clock_control_configure(clk, (clock_control_subsys_t)&cfg->pclken[1], NULL);
		if (ret != 0) {
			LOG_ERR("%s clock configure failed (%d)", dev->name, ret);
			return ret;
		}
	}

	/* Configure COMP inputs as specified in Device Tree, if any */
	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		/*
		* If the OPAMP is used only with internal channels, then no pinctrl is
		* provided in Device Tree, and pinctrl_apply_state returns -ENOENT,
		* but this should not be treated as an error.
		*/
		LOG_ERR("%s pinctrl setup failed (%d)", dev->name, ret);
		return ret;
	}



	LL_OPAMP_SetPowerMode(opamp, cfg->power_mode);
	LL_OPAMP_SetInputInverting(opamp, cfg->input_inverting);
	LL_OPAMP_SetInputNonInverting(opamp, cfg->input_non_inverting);

	ret = stm32_opamp_set_functional_mode(dev, cfg->functional_mode);
	if (ret != 0) {
		return ret;
	}

	return pm_device_driver_init(dev, stm32_opamp_pm_callback);
}

static DEVICE_API(opamp, api) = {
	.set_gain = stm32_opamp_set_gain,
};

#define STM32_OPAMP_DEVICE(inst)						\
	PINCTRL_DT_INST_DEFINE(inst);						\
	static const struct stm32_pclken _CONCAT(pclken_, inst)[] = 		\
		STM32_DT_INST_CLOCKS(inst);					\
	static const struct stm32_opamp_config _CONCAT(config, inst) = {	\
		.opamp = (OPAMP_TypeDef *)DT_INST_REG_ADDR(inst),		\
		.functional_mode = DT_INST_ENUM_IDX(inst, functional_mode),	\
		.power_mode = 0,						\
		.input_inverting = 0,						\
		.input_non_inverting = 0,					\
		.lock_enable = DT_INST_PROP(inst, st_lock_enable),		\
		.pclken = _CONCAT(pclken_, inst),				\
		.pclk_len = DT_INST_NUM_CLOCKS(inst),				\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			\
	};									\
										\
	PM_DEVICE_DT_INST_DEFINE(inst, stm32_opamp_pm_callback);		\
										\
	DEVICE_DT_INST_DEFINE(inst, stm32_opamp_init,				\
			PM_DEVICE_DT_INST_GET(inst), NULL,			\
			&_CONCAT(config, inst), POST_KERNEL,			\
			CONFIG_OPAMP_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(STM32_OPAMP_DEVICE)
