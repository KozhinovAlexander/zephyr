/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/opamp/fake_opamp.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#endif  /* CONFIG_ZTEST */

#define DT_DRV_COMPAT zephyr_fake_opamp

typedef int (*opamp_api_enable)(const struct device *dev);
typedef int (*opamp_api_disable)(const struct device *dev);
typedef int (*opamp_api_configure)(const struct device *dev,
				   const struct opamp_config config);

DEFINE_FAKE_VALUE_FUNC(int, opamp_fake_opamp_enable, const struct device *);

DEFINE_FAKE_VALUE_FUNC(int, opamp_fake_opamp_disable,  const struct device *);

DEFINE_FAKE_VALUE_FUNC(int, opamp_fake_opamp_configure, const struct device *,
			const struct opamp_config);

static DEVICE_API(opamp, fake_opamp_api) = {
	.enable = opamp_fake_opamp_enable,
	.disable = opamp_fake_opamp_disable,
	.configure = opamp_fake_opamp_configure,
};

#ifdef CONFIG_ZTEST
static void fake_opamp_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);

	RESET_FAKE(opamp_fake_opamp_enable);
	RESET_FAKE(opamp_fake_opamp_disable);
	RESET_FAKE(opamp_fake_opamp_configure);
}

ZTEST_RULE(opamp_fake_opamp_reset_rule, fake_opamp_reset_rule_before, NULL);
#endif  /* CONFIG_ZTEST */

DEVICE_DT_INST_DEFINE(
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	POST_KERNEL,
	CONFIG_OPAMP_INIT_PRIORITY,
	&fake_opamp_api
);
