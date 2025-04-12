/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/opamp.h>
#include <zephyr/drivers/opamp/fake_opamp.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/ztest.h>

DEFINE_FFF_GLOBALS;

#define FAKE_OPAMP_NODE DT_NODELABEL(fake_opamp)
#define FAKE_OPAMP_NAME DEVICE_DT_NAME(FAKE_OPAMP_NODE)

static const struct shell *test_sh;
static const struct device *test_dev = DEVICE_DT_GET(FAKE_OPAMP_NODE);

static int test_enable_hnd(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

static int test_disable_hnd(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

static int test_configure_hnd(const struct device *dev, struct opamp_config cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);

	return 1;
}

static void *test_setup(void)
{
	test_sh = shell_backend_dummy_get_ptr();
	WAIT_FOR(shell_ready(test_sh), 20000, k_msleep(1));
	zassert_true(shell_ready(test_sh), "timed out waiting for dummy shell backend");
	return NULL;
}

ZTEST(opamp_shell, test_enable)
{
	int ret;
	const char *out;
	size_t out_size;

	shell_backend_dummy_clear_output(test_sh);
	opamp_fake_opamp_enable_fake.custom_fake = test_enable_hnd;
	ret = shell_execute_cmd(test_sh, "opamp enable " FAKE_OPAMP_NAME);
	zassert_ok(ret);
	zassert_equal(opamp_fake_opamp_enable_fake.call_count, 1);
	zassert_equal(opamp_fake_opamp_enable_fake.arg0_val, test_dev);
	out = shell_backend_dummy_get_output(test_sh, &out_size);
	zassert_str_equal(out, "\r\n1\r\n");
}

ZTEST(opamp_shell, test_disable)
{
	int ret;
	const char *out;
	size_t out_size;

	shell_backend_dummy_clear_output(test_sh);
	opamp_fake_opamp_disable_fake.custom_fake = test_disable_hnd;
	ret = shell_execute_cmd(test_sh, "opamp disable " FAKE_OPAMP_NAME);
	zassert_ok(ret);
	zassert_equal(opamp_fake_opamp_disable_fake.call_count, 1);
	zassert_equal(opamp_fake_opamp_disable_fake.arg0_val, test_dev);
	out = shell_backend_dummy_get_output(test_sh, &out_size);
	zassert_str_equal(out, "\r\n1\r\n");
}

ZTEST(opamp_shell, test_configure)
{
	int ret;
	const char *out;
	size_t out_size;

	shell_backend_dummy_clear_output(test_sh);
	opamp_fake_opamp_configure_fake.custom_fake = test_configure_hnd;
	ret = shell_execute_cmd(test_sh, "opamp configure " FAKE_OPAMP_NAME);
	zassert_ok(ret);
	zassert_equal(opamp_fake_opamp_configure_fake.call_count, 1);
	zassert_equal(opamp_fake_opamp_configure_fake.arg0_val, test_dev);
	out = shell_backend_dummy_get_output(test_sh, &out_size);
	zassert_str_equal(out, "\r\n1\r\n");
}

ZTEST_SUITE(opamp_shell, NULL, test_setup, NULL, NULL, NULL);
