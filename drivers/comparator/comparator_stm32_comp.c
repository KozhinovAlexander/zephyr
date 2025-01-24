/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/comparator/stm32_comp.h>

#include <stm32_ll_exti.h>
#include <stm32_ll_system.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stm32_comp, CONFIG_COMPARATOR_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_comp

#define STM32_COMP_ENUM(name, value) \
	_CONCAT_4(COMP_STM32_COMP_, name, _, value)

#define STM32_COMP_DT_INST_ENUM(inst, name, prop) \
	STM32_COMP_ENUM(name, DT_INST_STRING_TOKEN(inst, prop))

#define STM32_COMP_DT_INST_P_IN(inst) \
	STM32_COMP_DT_INST_ENUM(inst, P_INPUT, positive_input)

#define STM32_COMP_DT_INST_N_IN(inst) \
	STM32_COMP_DT_INST_ENUM(inst, N_INPUT, negative_input)

#define STM32_COMP_DT_INST_HYST_MODE(inst) \
	STM32_COMP_DT_INST_ENUM(inst, HYSTERESIS_MODE, hysteresis)

#define STM32_COMP_DT_INST_INV_OUT(inst) \
	STM32_COMP_DT_INST_ENUM(inst, INV_OUTPUT, invert_output)

#define STM32_COMP_DT_INST_BLANK_SEL(inst) \
	STM32_COMP_DT_INST_ENUM(inst, BLANK_SEL, st_blank_sel)

#define STM32_COMP_DT_INST_LOCK(inst) \
	DT_INST_PROP(inst, st_lock_enable)

#define STM32_COMP_DT_MILLER_EFFECT_HOLD_ENABLE(inst) \
	DT_INST_PROP(inst, st_miller_effect_hold_enable)

#define STM32_COMP_DT_EXTI_LINE(inst) \
	CONCAT(LL_EXTI_LINE_, DT_INST_PROP(0, st_exti_line))

#define STM32_COMP_DT_EXTI_LINE_NUMBER(inst) \
	DT_INST_PROP(0, st_exti_line)

#define STM32_COMP_DT_INST_COMP_CONFIG_INIT(inst)					\
	{																\
		.InputPlus = STM32_COMP_DT_INST_P_IN(inst),					\
		.InputMinus = STM32_COMP_DT_INST_N_IN(inst),				\
		.InputHysteresis = STM32_COMP_DT_INST_HYST_MODE(inst),		\
		.OutputPolarity = STM32_COMP_DT_INST_INV_OUT(inst),			\
		.OutputBlankingSource = STM32_COMP_DT_INST_BLANK_SEL(inst),	\
	}

struct stm32_comp_config {
	COMP_TypeDef *comp;
	const struct pinctrl_dev_config *pincfg;
	void (*irq_init)(void);
	const LL_COMP_InitTypeDef comp_config;
	const uint32_t exti_line;
	const uint8_t exti_line_number;
	const bool lock_enable;
	const bool miller_effect_hold_enable;
};

struct stm32_comp_data {
	uint32_t interrupt_mask;
	comparator_callback_t callback;
	void *user_data;
};

static bool stm32_comp_is_resumed(const struct device *dev)
{
#if CONFIG_PM_DEVICE
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	return state == PM_DEVICE_STATE_ACTIVE;
#else
	ARG_UNUSED(dev);
	return true;
#endif
}

static int stm32_comp_get_output(const struct device *dev)
{
	const struct stm32_comp_config *cfg = dev->config;
	return LL_COMP_ReadOutputLevel(cfg->comp);
}

static int stm32_comp_set_trigger(const struct device *dev,
								  enum comparator_trigger trigger)
{
	const struct stm32_comp_config *cfg = dev->config;
	struct stm32_comp_data *data = dev->data;
	int ret = 0;

	if (LL_COMP_IsLocked(cfg->comp)) {
		ret = EACCES;
		LOG_ERR("%s device is locked - abort (%d).", dev->name, ret);
		return -ret;
	}

	// ACMP_DisableInterrupts(cfg->comp, UINT32_MAX);

	/*
	The comparator outputs are internally connected to the Extended interrupts and events
	controller. Each comparator has its own EXTI line and can generate either interrupts or
	events. The same mechanism is used to exit Sleep and Stop low-power modes.
	Refer to Interrupt and events section for more details.

	To enable the COMPx interrupt, it is required to follow this sequence:
	1. Configure and enable the EXTI line corresponding to the COMPx output event in
	interrupt mode and select sensitivity to rising edge, falling edge or to both edges
	2. Configure and enable the NVIC IRQ channel mapped to the corresponding EXTI lines
	3. Enable COMPx
	Interrupt events are flagged through flags in EXTI_PR1/EXTI_PR2 registers.
	*/

	LL_EXTI_InitTypeDef exti_init_data = {0};
	LL_EXTI_StructInit(&exti_init_data);
	exti_init_data.LineCommand = ENABLE;
	exti_init_data.Mode = LL_EXTI_MODE_IT;

	if (cfg->exti_line_number < 32U) {
		/* COMP EXTI line bit in range 0 to 31 */
		exti_init_data.Line_0_31 = cfg->exti_line;
	} else {
		/* COMP EXTI line bit in range 32 to 63 */
		exti_init_data.Line_32_63 = cfg->exti_line;
	}

	switch (trigger) {
	case COMPARATOR_TRIGGER_NONE:
		data->interrupt_mask = 0;
		exti_init_data.Trigger = LL_EXTI_TRIGGER_NONE;
		break;

	case COMPARATOR_TRIGGER_RISING_EDGE:
		exti_init_data.Trigger = LL_EXTI_TRIGGER_RISING;
		break;

	case COMPARATOR_TRIGGER_FALLING_EDGE:
		exti_init_data.Trigger = LL_EXTI_TRIGGER_FALLING;
		break;

	case COMPARATOR_TRIGGER_BOTH_EDGES:
		exti_init_data.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
		break;
	}

	ret = (int)LL_EXTI_Init(&exti_init_data);
	if (ret != 0) {
		LOG_ERR("%s: EXTI init failed (%d)", dev->name, ret);
		return -ret;
	}

	if (data->interrupt_mask && data->callback != NULL) {
		// ACMP_EnableInterrupts(cfg->comp, data->interrupt_mask);
	}

	return ret;
}

static int stm32_comp_set_trigger_callback(const struct device *dev,
										   comparator_callback_t callback,
										   void *user_data)
{
	const struct stm32_comp_config *cfg = dev->config;
	struct stm32_comp_data *data = dev->data;

	// ACMP_DisableInterrupts(cfg->comp, UINT32_MAX);

	data->callback = callback;
	data->user_data = user_data;

	if (data->callback == NULL) {
		LL_COMP_Enable(cfg->comp);
		return 0;
	}

	if (data->interrupt_mask) {
		// ACMP_EnableInterrupts(cfg->comp, data->interrupt_mask);
	}

	return 0;
}

static int stm32_comp_trigger_is_pending(const struct device *dev)
{
	// const struct stm32_comp_config *cfg = dev->config;
	return 0;
}

static DEVICE_API(comparator, stm32_comp_comp_api) = {
	.get_output = stm32_comp_get_output,
	.set_trigger = stm32_comp_set_trigger,
	.set_trigger_callback = stm32_comp_set_trigger_callback,
	.trigger_is_pending = stm32_comp_trigger_is_pending,
};

static int stm32_comp_pm_callback(const struct device *dev,
								  enum pm_device_action action)
{
	const struct stm32_comp_config *cfg = dev->config;

	if (action == PM_DEVICE_ACTION_RESUME) {
		if (cfg->lock_enable) {
			LL_COMP_Lock(cfg->comp);
		}

		LL_COMP_Enable(cfg->comp);
	}

#if CONFIG_PM_DEVICE
	if (action == PM_DEVICE_ACTION_SUSPEND) {
		LL_COMP_Disable(cfg->comp)
	}
#endif

	return 0;
}

static void stm32_comp_irq_handler(const struct device *dev)
{
	const struct stm32_comp_config *cfg = dev->config;
	struct stm32_comp_data *data = dev->data;

	// ACMP_ClearStatusFlags(cfg->comp, UINT32_MAX);

	if (data->callback == NULL) {
		return;
	}

	data->callback(dev, data->user_data);
}

static int stm32_comp_init(const struct device *dev)
{
	const struct stm32_comp_config *cfg = dev->config;
	int ret = 0;

	if (LL_COMP_IsLocked(cfg->comp)) {
		ret = EACCES;
		LOG_ERR("%s device is locked - abort (%d).", dev->name, ret);
		return -ret;
	}

	LL_COMP_Disable(cfg->comp);

	/* Configure COMP inputs as specified in Device Tree, if any */
	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if ((ret < 0) && (ret != -ENOENT)) {
		/*
		 * If the COMP is used only with internal channels, then no pinctrl is
		 * provided in Device Tree, and pinctrl_apply_state returns -ENOENT,
		 * but this should not be treated as an error.
		 */
		LOG_ERR("%s pinctrl setup failed (%d)", dev->name, ret);
		return ret;
	}

	ret = LL_COMP_Init(cfg->comp, &cfg->comp_config);
	if (ret != 0) {
		LOG_ERR("COMP instance is locked (%d)", ret);
		return ret;
	}

	if (cfg->miller_effect_hold_enable) {
		SET_BIT(cfg->comp->CSR, BIT(1U));
	}

	cfg->irq_init();

	return pm_device_driver_init(dev, stm32_comp_pm_callback);
}

#define STM32_COMP_IRQ_HANDLER_SYM(inst) \
	_CONCAT(stm32_comp_irq_init, inst)

#define STM32_COMP_IRQ_HANDLER_DEFINE(inst)				\
	static void STM32_COMP_IRQ_HANDLER_SYM(inst)(void)	\
	{													\
		IRQ_CONNECT(DT_INST_IRQN(inst),					\
				DT_INST_IRQ(inst, priority),			\
				stm32_comp_irq_handler,					\
				DEVICE_DT_INST_GET(inst),				\
				0);										\
														\
		irq_enable(DT_INST_IRQN(inst));					\
	}

#define STM32_COMP_DEVICE(inst)														\
	PINCTRL_DT_INST_DEFINE(inst);													\
																					\
	static struct stm32_comp_data _CONCAT(data, inst);								\
																					\
	STM32_COMP_IRQ_HANDLER_DEFINE(inst)												\
																					\
	static const struct stm32_comp_config _CONCAT(config, inst) = {					\
		.comp = (COMP_TypeDef *)DT_INST_REG_ADDR(inst),								\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),								\
		.irq_init = STM32_COMP_IRQ_HANDLER_SYM(inst),								\
		.comp_config = STM32_COMP_DT_INST_COMP_CONFIG_INIT(inst),					\
		.exti_line = STM32_COMP_DT_EXTI_LINE(inst),									\
		.exti_line_number = STM32_COMP_DT_EXTI_LINE_NUMBER(inst),					\
		.lock_enable = STM32_COMP_DT_INST_LOCK(inst),								\
		.miller_effect_hold_enable = STM32_COMP_DT_MILLER_EFFECT_HOLD_ENABLE(inst)	\
	};																				\
																					\
	PM_DEVICE_DT_INST_DEFINE(inst, stm32_comp_pm_callback);							\
																					\
	DEVICE_DT_INST_DEFINE(inst,														\
				stm32_comp_init,													\
				PM_DEVICE_DT_INST_GET(inst),										\
				&_CONCAT(data, inst),												\
				&_CONCAT(config, inst),												\
				POST_KERNEL,														\
				CONFIG_COMPARATOR_INIT_PRIORITY,									\
				&stm32_comp_comp_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_COMP_DEVICE)
