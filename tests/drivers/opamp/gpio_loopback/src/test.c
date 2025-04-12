/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/opamp.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static const struct device *test_dev = DEVICE_DT_GET(DT_ALIAS(test_opamp));
static const struct gpio_dt_spec test_pin = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), test_gpios);
static K_SEM_DEFINE(test_sem, 0, 1);

static void *test_setup(void)
{
	zassert_ok(gpio_pin_configure_dt(&test_pin, GPIO_OUTPUT_INACTIVE));
	return NULL;
}

static void test_before(void *f)
{
	ARG_UNUSED(f);

	k_sem_reset(&test_sem);
	zassert_ok(gpio_pin_set_dt(&test_pin, 0));
	// zassert_ok(opamp_set_trigger(test_dev, OPAMP_TRIGGER_NONE));
	// zassert_ok(opamp_set_trigger_callback(test_dev, NULL, NULL));
	// zassert_between_inclusive(opamp_trigger_is_pending(test_dev), 0, 1);
}

ZTEST_SUITE(opamp_gpio_loopback, NULL, test_setup, test_before, NULL, NULL);

ZTEST(opamp_gpio_loopback, test_enable)
{
	zassert_ok(gpio_pin_set_dt(&test_pin, 1));
	k_msleep(1);
	zassert_ok(opamp_enable(test_dev));
	// k_msleep(1);
	// zassert_equal(opamp_is_enabled(test_dev), 1);
}
