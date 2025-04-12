/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/opamp.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include <stm32_ll_opamp.h>
#include <stm32_ll_system.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stm32_opamp, CONFIG_OPAMP_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_opamp

/*
 * TODO: Map LOW -> NORMAL, NORMAL -> NORMAL, HIGH -> HIGHSPEED
 */
#define STM32_COMP_DT_POWER_MODE(inst)	\
	CONCAT(LL_OPAMP_POWERMODE_, DT_INST_STRING_TOKEN(inst, slew_rate))

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_opamp)
#define STM32_OPAMP_DT_INST_OPAMP_CONFIG_INIT(inst)								\
	{																			\
		.PowerMode = STM32_COMP_DT_POWER_MODE(inst),							\
		.FunctionalMode = 0,													\
		.InputNonInverting = 0,													\
		.InputInverting = 0,													\
	}
#else /* st_stm32h7_comp */
#error "Not supported SoC"
#endif /* st_stm32h7_comp */

struct stm32_opamp_config {
	OPAMP_TypeDef *opamp;
	const struct stm32_pclken *pclken;
	const struct pinctrl_dev_config *pincfg;
	LL_OPAMP_InitTypeDef opamp_config;
};

__maybe_unused static bool stm32_opamp_is_resumed(void)
{
#if CONFIG_PM_DEVICE
	enum pm_device_state state;

	(void)pm_device_state_get(DEVICE_DT_INST_GET(0), &state);
	return state == PM_DEVICE_STATE_ACTIVE;
#else
	return true;
#endif /* CONFIG_PM_DEVICE */
}

static int stm32_opamp_enable(const struct device *dev)
{
	const struct stm32_opamp_config *cfg = dev->config;

	LL_OPAMP_Enable(cfg->opamp);

	return 0;
}

static int stm32_opamp_disable(const struct device *dev)
{
	const struct stm32_opamp_config *cfg = dev->config;

	LL_OPAMP_Disable(cfg->opamp);

	return 0;
}

static int stm32_opamp_configure(const struct device *dev, const struct opamp_config config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(config);

	return 0;
}

static DEVICE_API(opamp, stm32_opamp_opamp_api) = {
	.enable = stm32_opamp_enable,
	.disable = stm32_opamp_disable,
	.configure = stm32_opamp_configure,
};

static int stm32_opamp_pm_callback(const struct device *dev, enum pm_device_action action)
{
	const struct stm32_opamp_config *cfg = dev->config;
	ARG_UNUSED(cfg);

	// if (action == PM_DEVICE_ACTION_RESUME) {
	// 	if (cfg->lock_enable) {
	// 		LL_COMP_Lock(cfg->comp);
	// 	}
	// 	LL_COMP_Enable(cfg->comp);
	// }

#if CONFIG_PM_DEVICE
	// if (action == PM_DEVICE_ACTION_SUSPEND) {
	// 	LL_COMP_Disable(cfg->comp);
	// }
#endif

	return 0;
}

static int stm32_opamp_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct stm32_opamp_config *cfg = dev->config;
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

	/* Enable OPAMP clock source */
	ret = clock_control_configure(clk, (clock_control_subsys_t)&cfg->pclken[1], NULL);
	if (ret != 0) {
		LOG_ERR("%s clock configure failed (%d)", dev->name, ret);
		return ret;
	}

	/* Configure COMP inputs as specified in Device Tree, if any */
	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		/*
		* If the COMP is used only with internal channels, then no pinctrl is
		* provided in Device Tree, and pinctrl_apply_state returns -ENOENT,
		* but this should not be treated as an error.
		*/
		LOG_ERR("%s pinctrl setup failed (%d)", dev->name, ret);
		return ret;
	}

	ret = LL_OPAMP_Init(cfg->opamp, (LL_OPAMP_InitTypeDef *)&cfg->opamp_config);
	if (ret != 0) {
		LOG_ERR("OpAmp instance is locked (%d)", ret);
		return ret;
	}

	return pm_device_driver_init(dev, stm32_opamp_pm_callback);
}

#define STM32_OPAMP_DEVICE(inst)																	\
	static const struct stm32_pclken opamp_clk[] = STM32_DT_INST_CLOCKS(inst);						\
																									\
	PINCTRL_DT_INST_DEFINE(inst);																	\
																									\
	static const struct stm32_opamp_config _CONCAT(config, inst) = {								\
		.opamp = (OPAMP_TypeDef *)DT_INST_REG_ADDR(inst),											\
		.pclken = opamp_clk,																		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),												\
		.opamp_config = STM32_OPAMP_DT_INST_OPAMP_CONFIG_INIT(inst)};								\
																									\
	PM_DEVICE_DT_INST_DEFINE(inst, stm32_opamp_pm_callback);										\
																									\
	DEVICE_DT_INST_DEFINE(inst, stm32_opamp_init, PM_DEVICE_DT_INST_GET(inst),						\
				NULL, &_CONCAT(config, inst), PRE_KERNEL_1,											\
				CONFIG_OPAMP_INIT_PRIORITY, &stm32_opamp_opamp_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_OPAMP_DEVICE)
