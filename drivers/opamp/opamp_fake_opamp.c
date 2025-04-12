/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/opamp/fake_opamp.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#endif

#define DT_DRV_COMPAT zephyr_fake_opamp

DEFINE_FAKE_VALUE_FUNC(int,
		       opamp_fake_opamp_get_output,
		       const struct device *);

DEFINE_FAKE_VALUE_FUNC(int,
		       opamp_fake_opamp_set_trigger,
		       const struct device *,
		       enum opamp_trigger);

DEFINE_FAKE_VALUE_FUNC(int,
		       opamp_fake_opamp_set_trigger_callback,
		       const struct device *,
		       opamp_callback_t,
		       void *);

DEFINE_FAKE_VALUE_FUNC(int,
		       opamp_fake_opamp_trigger_is_pending,
		       const struct device *);

static DEVICE_API(opamp, fake_opamp_api) = {
	.get_output = opamp_fake_opamp_get_output,
	.set_trigger = opamp_fake_opamp_set_trigger,
	.set_trigger_callback = opamp_fake_opamp_set_trigger_callback,
	.trigger_is_pending = opamp_fake_opamp_trigger_is_pending,
};

#ifdef CONFIG_ZTEST
static void fake_opamp_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);

	RESET_FAKE(opamp_fake_opamp_get_output);
	RESET_FAKE(opamp_fake_opamp_set_trigger);
	RESET_FAKE(opamp_fake_opamp_set_trigger_callback);
	RESET_FAKE(opamp_fake_opamp_trigger_is_pending);
}

ZTEST_RULE(opamp_fake_opamp_reset_rule, fake_opamp_reset_rule_before, NULL);
#endif

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
