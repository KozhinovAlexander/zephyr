/*
 * Copyright (c) 2024 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/cordic.h>
#include <zephyr/device.h>
#include <math.h>

#include <stm32_ll_cordic.h>

#define CORDIC_NODE DT_NODELABEL(cordic)
#define CORDIC_LL_DEV ((CORDIC_TypeDef *)DT_REG_ADDR(CORDIC_NODE))

static const struct device *const cordic_dev = DEVICE_DT_GET(CORDIC_NODE);

/* Test setup function */
static void *cordic_setup(void)
{
	zassert_true(device_is_ready(cordic_dev), "CORDIC device not ready");
	return NULL;
}

/**
 * @brief Test CORDIC Configure function. This test is allowed to apply some
 * arbitrary configuration.
 */
ZTEST(stm32_cordic, test_cordic_configure_function)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_SINE,
		.precision = CORDIC_PRECISION_1,
		.in_width = CORDIC_DATA_WIDTH_16BIT,
		.out_width = CORDIC_DATA_WIDTH_16BIT,
	};
	int ret = 0;

	const enum cordic_function fn_list[] = {
		CORDIC_FUNC_COSINE,
		CORDIC_FUNC_SINE,
		CORDIC_FUNC_PHASE,
		CORDIC_FUNC_MODULUS,
		CORDIC_FUNC_ARCTANGENT,
		CORDIC_FUNC_HCOSINE,
		CORDIC_FUNC_HSINE,
		CORDIC_FUNC_HARCTANGENT,
		CORDIC_FUNC_NATURALLOG,
		CORDIC_FUNC_SQUAREROOT,
	};

	const uint32_t mapped_fn_list[] = {
		LL_CORDIC_FUNCTION_COSINE,
		LL_CORDIC_FUNCTION_SINE,
		LL_CORDIC_FUNCTION_PHASE,
		LL_CORDIC_FUNCTION_MODULUS,
		LL_CORDIC_FUNCTION_ARCTANGENT,
		LL_CORDIC_FUNCTION_HCOSINE,
		LL_CORDIC_FUNCTION_HSINE,
		LL_CORDIC_FUNCTION_HARCTANGENT,
		LL_CORDIC_FUNCTION_NATURALLOG,
		LL_CORDIC_FUNCTION_SQUAREROOT,
	};

	/* Iterate through all possible function configurations */
	for(int i = 0; i < ARRAY_SIZE(fn_list); i++) {
		config.function = fn_list[i];
		ret = cordic_configure(cordic_dev, &config);
		zassert_equal(ret, 0, "%s: CORDIC function configure failed: %d",
				cordic_dev->name, ret);
		const uint32_t ll_fn = LL_CORDIC_GetFunction(CORDIC_LL_DEV);
		zassert_equal(ll_fn, mapped_fn_list[i],
				"%s: CORDIC function 0x%x != 0x%x (expected_fn != current_fn)"
				": CORDIC.CSR: 0x%x",
				cordic_dev->name, mapped_fn_list[i], ll_fn,
				CORDIC_LL_DEV->CSR);

		/* Verify number of argumernts an results */
		const uint32_t ll_nargs = LL_CORDIC_GetNbWrite(CORDIC_LL_DEV);
		const uint32_t ll_nres = LL_CORDIC_GetNbRead(CORDIC_LL_DEV);
		const uint32_t ll_in_width = LL_CORDIC_GetInSize(CORDIC_LL_DEV);
		const uint32_t ll_out_width = LL_CORDIC_GetOutSize(CORDIC_LL_DEV);
		const uint32_t ll_scale = LL_CORDIC_GetScale(CORDIC_LL_DEV);
		if(ll_fn == CORDIC_FUNC_COSINE || ll_fn == CORDIC_FUNC_SINE ||
		   ll_fn == CORDIC_FUNC_PHASE || ll_fn == CORDIC_FUNC_MODULUS) {
			const uint32_t nargs_expected = ll_in_width == LL_CORDIC_INSIZE_32BITS ?
							LL_CORDIC_NBWRITE_2 : LL_CORDIC_NBWRITE_1;
			const uint32_t nres_expected = ll_out_width == LL_CORDIC_OUTSIZE_32BITS ?
						       LL_CORDIC_NBREAD_2 : LL_CORDIC_NBREAD_1;
			zassert_equal(ll_nargs, nargs_expected,
				"%s: CORDIC nargs 0x%x != 0x%x (expected_nargs != current_nargs)"
				": CORDIC.CSR: 0x%x",
				cordic_dev->name, nargs_expected, ll_nargs,
				CORDIC_LL_DEV->CSR);
			zassert_equal(ll_nres, nres_expected,
				"%s: CORDIC nres 0x%x != 0x%x (expected_nres != current_nres)"
				": CORDIC.CSR: 0x%x",
				cordic_dev->name, nres_expected, ll_nres,
				CORDIC_LL_DEV->CSR);
			zassert_equal(ll_scale, LL_CORDIC_SCALE_0,
				"%s: CORDIC scale 0x%x != 0x%x (expected_scale != current_scale)",
				cordic_dev->name, LL_CORDIC_SCALE_0, ll_scale);
		} else if(ll_fn == CORDIC_FUNC_ARCTANGENT || ll_fn == CORDIC_FUNC_HARCTANGENT ||
			  ll_fn == CORDIC_FUNC_NATURALLOG || ll_fn == CORDIC_FUNC_SQUAREROOT) {
			zassert_equal(ll_nargs, LL_CORDIC_NBWRITE_1,
				"%s: CORDIC nargs 0x%x != 0x%x (expected_nargs != current_nargs)"
				": CORDIC.CSR: 0x%x",
				cordic_dev->name, LL_CORDIC_NBWRITE_1, ll_nargs,
				CORDIC_LL_DEV->CSR);
			zassert_equal(ll_nres, LL_CORDIC_NBREAD_1,
				"%s: CORDIC nres 0x%x != 0x%x (expected_nres != current_nres)"
				": CORDIC.CSR: 0x%x",
				cordic_dev->name, LL_CORDIC_NBREAD_1, ll_nres,
				CORDIC_LL_DEV->CSR);

			if (ll_fn == CORDIC_FUNC_SQUAREROOT || ll_fn == CORDIC_FUNC_ARCTANGENT) {
				zassert_equal(ll_scale, LL_CORDIC_SCALE_0,
					"%s: CORDIC scale 0x%x != 0x%x (expected_scale != current_scale)",
					cordic_dev->name, LL_CORDIC_SCALE_0, ll_scale);
				continue;
			} else {
				zassert_equal(ll_scale, LL_CORDIC_SCALE_1,
					"%s: CORDIC scale 0x%x != 0x%x (expected_scale != current_scale)",
					cordic_dev->name, LL_CORDIC_SCALE_1, ll_scale);
			}
		} else {
			zassert_equal(ll_nargs, LL_CORDIC_NBWRITE_1,
				"%s: CORDIC nargs 0x%x != 0x%x (expected_nargs != current_nargs)"
				": CORDIC.CSR: 0x%x",
				cordic_dev->name, LL_CORDIC_NBWRITE_1, ll_nargs,
				CORDIC_LL_DEV->CSR);
			const uint32_t nres_expected = ll_out_width == LL_CORDIC_OUTSIZE_32BITS ?
						       LL_CORDIC_NBREAD_2 : LL_CORDIC_NBREAD_1;
			zassert_equal(ll_nres, nres_expected,
				"%s: CORDIC nres 0x%x != 0x%x (expected_nres != current_nres)"
				": CORDIC.CSR: 0x%x",
				cordic_dev->name, nres_expected, ll_nres,
				CORDIC_LL_DEV->CSR);
			if (ll_fn == LL_CORDIC_FUNCTION_HCOSINE || ll_fn == LL_CORDIC_FUNCTION_HSINE) {
				zassert_equal(ll_scale, LL_CORDIC_SCALE_1,
					"%s: CORDIC scale 0x%x != 0x%x (expected_scale != current_scale)",
					cordic_dev->name, LL_CORDIC_SCALE_1, ll_scale);
				continue;
			} else {
				zassert_equal(ll_scale, LL_CORDIC_SCALE_0,
					"%s: CORDIC scale 0x%x != 0x%x (expected_scale != current_scale)",
					cordic_dev->name, LL_CORDIC_SCALE_0, ll_scale);
			}
	}
	}
}

/**
 * @brief Test CORDIC Configure precision. This test is allowed to apply some
 * arbitrary configuration.
 */
ZTEST(stm32_cordic, test_cordic_configure_precision)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_SINE,
		.precision = CORDIC_PRECISION_1,
		.in_width = CORDIC_DATA_WIDTH_16BIT,
		.out_width = CORDIC_DATA_WIDTH_16BIT,
	};
	int ret = 0;

	const enum cordic_precision prec_list[] = {
		CORDIC_PRECISION_1,
		CORDIC_PRECISION_2,
		CORDIC_PRECISION_3,
		CORDIC_PRECISION_4,
		CORDIC_PRECISION_5,
		CORDIC_PRECISION_6,
		CORDIC_PRECISION_7,
		CORDIC_PRECISION_8,
		CORDIC_PRECISION_9,
		CORDIC_PRECISION_10,
		CORDIC_PRECISION_11,
		CORDIC_PRECISION_12,
		CORDIC_PRECISION_13,
		CORDIC_PRECISION_14,
		CORDIC_PRECISION_15,
	};

	const uint32_t mapped_prec_list[] = {
		LL_CORDIC_PRECISION_1CYCLE,
		LL_CORDIC_PRECISION_2CYCLES,
		LL_CORDIC_PRECISION_3CYCLES,
		LL_CORDIC_PRECISION_4CYCLES,
		LL_CORDIC_PRECISION_5CYCLES,
		LL_CORDIC_PRECISION_6CYCLES,
		LL_CORDIC_PRECISION_7CYCLES,
		LL_CORDIC_PRECISION_8CYCLES,
		LL_CORDIC_PRECISION_9CYCLES,
		LL_CORDIC_PRECISION_10CYCLES,
		LL_CORDIC_PRECISION_11CYCLES,
		LL_CORDIC_PRECISION_12CYCLES,
		LL_CORDIC_PRECISION_13CYCLES,
		LL_CORDIC_PRECISION_14CYCLES,
		LL_CORDIC_PRECISION_15CYCLES,
	};

	/* Iterate through all possible precision configurations */
	for(int i = 0; i < ARRAY_SIZE(prec_list); i++) {
		config.precision = prec_list[i];
		ret = cordic_configure(cordic_dev, &config);
		zassert_equal(ret, 0, "CORDIC precision configure failed: %d", ret);
		const uint32_t ll_prec = LL_CORDIC_GetPrecision(CORDIC_LL_DEV);
		zassert_equal(ll_prec, mapped_prec_list[i],
				"%s: CORDIC precision 0x%x != 0x%x (expected_prec != current_prec)",
				cordic_dev->name, mapped_prec_list[i], ll_prec);
	}
}

/**
 * @brief Test CORDIC Configure data width. This test is allowed to apply some
 * arbitrary configuration.
 */
ZTEST(stm32_cordic, test_cordic_configure_data_width)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_SINE,
		.precision = CORDIC_PRECISION_1,
		.in_width = CORDIC_DATA_WIDTH_16BIT,
		.out_width = CORDIC_DATA_WIDTH_16BIT,
	};
	int ret = 0;

	const enum cordic_data_width data_width_list[] = {
		CORDIC_DATA_WIDTH_32BIT,
		CORDIC_DATA_WIDTH_16BIT,
	};

	const uint32_t mapped_in_width_list[] = {
		LL_CORDIC_INSIZE_32BITS,
		LL_CORDIC_INSIZE_16BITS,
	};

	/* Iterate through all possible input data width configurations for
	 * input size.
	 */
	for(int i = 0; i < ARRAY_SIZE(data_width_list); i++) {
		config.in_width = data_width_list[i];
		ret = cordic_configure(cordic_dev, &config);
		zassert_equal(ret, 0, "CORDIC input data width configure failed: %d", ret);
		const uint32_t ll_in_width = LL_CORDIC_GetInSize(CORDIC_LL_DEV);
		zassert_equal(ll_in_width, mapped_in_width_list[i],
				"%s: CORDIC input size 0x%x != 0x%x (expected_in_width != current_in_width)",
				cordic_dev->name, mapped_in_width_list[i], ll_in_width);
	}

	const uint32_t mapped_out_width_list[] = {
		LL_CORDIC_OUTSIZE_32BITS,
		LL_CORDIC_OUTSIZE_16BITS,
	};

	/* Iterate through all possible input data width configurations for
	 * output size.
	 */
	for(int i = 0; i < ARRAY_SIZE(data_width_list); i++) {
		config.out_width = data_width_list[i];
		ret = cordic_configure(cordic_dev, &config);
		zassert_equal(ret, 0, "CORDIC output data width configure failed: %d", ret);
		const uint32_t ll_out_width = LL_CORDIC_GetOutSize(CORDIC_LL_DEV);
		zassert_equal(ll_out_width, mapped_out_width_list[i],
				"%s: CORDIC output size 0x%x != 0x%x (expected_out_width != current_out_width)",
				cordic_dev->name, mapped_out_width_list[i], ll_out_width);
	}

}

ZTEST_SUITE(stm32_cordic, NULL, cordic_setup, NULL, NULL, NULL);
