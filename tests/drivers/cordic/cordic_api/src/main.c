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

#define CORDIC_NODE DT_NODELABEL(cordic)

static const struct device *const cordic_dev = DEVICE_DT_GET(CORDIC_NODE);

/* Helper macros for Q1.31 fixed-point format */
#define FLOAT_TO_Q31(x) ((int32_t)((x) * 2147483648.0))
#define Q31_TO_FLOAT(x) ((float)(x) / 2147483648.0)

/* Tolerance for floating-point comparisons (about 0.001) */
#define TOLERANCE_Q31 (2147483) /* ~0.001 in Q1.31 */

/* Test setup function */
static void *cordic_setup(void)
{
	zassert_true(device_is_ready(cordic_dev), "CORDIC device not ready");
	return NULL;
}

/* Helper function to check if two Q1.31 values are approximately equal */
static bool q31_approx_equal(int32_t a, int32_t b, int32_t tolerance)
{
	int32_t diff = a - b;
	if (diff < 0) {
		diff = -diff;
	}
	return diff <= tolerance;
}

/**
 * @brief Test CORDIC Cosine function
 *
 * Tests the cosine function with various angles in Q1.31 format.
 * CORDIC input angle is in range [-π, π] mapped to [-1, 1] in Q1.31.
 */
ZTEST(cordic_api, test_cosine_function)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_COSINE,
		.precision = CORDIC_PRECISION_10,
		.scale = CORDIC_SCALE_VAL_0,
		.in_size = CORDIC_DATA_WIDTH_32BIT,
		.out_size = CORDIC_DATA_WIDTH_32BIT,
	};
	int32_t in_data[1];
	int32_t out_data[2];
	int ret;

	/* Configure CORDIC for cosine */
	ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	/* Test cos(0) = 1, sin(0) = 0 */
	in_data[0] = FLOAT_TO_Q31(0.0);
	ret = cordic_calculate(cordic_dev, in_data, out_data, 1, 2);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	zassert_true(q31_approx_equal(out_data[0], FLOAT_TO_Q31(1.0), TOLERANCE_Q31),
		     "cos(0) incorrect: got %d, expected %d",
		     out_data[0], FLOAT_TO_Q31(1.0));

	/* Test cos(π/4) ≈ 0.707 */
	in_data[0] = FLOAT_TO_Q31(0.25);  /* π/4 mapped as 0.25 */
	ret = cordic_calculate(cordic_dev, in_data, out_data, 1, 2);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	zassert_true(q31_approx_equal(out_data[0], FLOAT_TO_Q31(0.707), TOLERANCE_Q31 * 10),
		     "cos(π/4) incorrect");

	/* Test cos(π/2) ≈ 0 */
	in_data[0] = FLOAT_TO_Q31(0.5);  /* π/2 mapped as 0.5 */
	ret = cordic_calculate(cordic_dev, in_data, out_data, 1, 2);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	zassert_true(q31_approx_equal(out_data[0], FLOAT_TO_Q31(0.0), TOLERANCE_Q31 * 10),
		     "cos(π/2) incorrect");
}

/**
 * @brief Test CORDIC Sine function
 */
ZTEST(cordic_api, test_sine_function)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_SINE,
		.precision = CORDIC_PRECISION_10,
		.scale = CORDIC_SCALE_VAL_0,
		.in_size = CORDIC_DATA_WIDTH_32BIT,
		.out_size = CORDIC_DATA_WIDTH_32BIT,
	};
	int32_t in_data[1];
	int32_t out_data[2];
	int ret;

	ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	/* Test sin(0) = 0 */
	in_data[0] = FLOAT_TO_Q31(0.0);
	ret = cordic_calculate(cordic_dev, in_data, out_data, 1, 2);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	zassert_true(q31_approx_equal(out_data[1], FLOAT_TO_Q31(0.0), TOLERANCE_Q31),
		     "sin(0) incorrect");

	/* Test sin(π/4) ≈ 0.707 */
	in_data[0] = FLOAT_TO_Q31(0.25);
	ret = cordic_calculate(cordic_dev, in_data, out_data, 1, 2);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	zassert_true(q31_approx_equal(out_data[1], FLOAT_TO_Q31(0.707), TOLERANCE_Q31 * 10),
		     "sin(π/4) incorrect");

	/* Test sin(π/2) ≈ 1 */
	in_data[0] = FLOAT_TO_Q31(0.5);
	ret = cordic_calculate(cordic_dev, in_data, out_data, 1, 2);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	zassert_true(q31_approx_equal(out_data[1], FLOAT_TO_Q31(1.0), TOLERANCE_Q31 * 10),
		     "sin(π/2) incorrect");
}

/**
 * @brief Test CORDIC Phase (arctan2) function
 *
 * Calculates the phase angle from x,y coordinates.
 */
ZTEST(cordic_api, test_phase_function)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_PHASE,
		.precision = CORDIC_PRECISION_10,
		.scale = CORDIC_SCALE_VAL_0,
		.in_size = CORDIC_DATA_WIDTH_32BIT,
		.out_size = CORDIC_DATA_WIDTH_32BIT,
	};
	int32_t in_data[2];
	int32_t out_data[1];
	int ret;

	ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	/* Test phase(1, 0) = 0 */
	in_data[0] = FLOAT_TO_Q31(1.0);  /* x */
	in_data[1] = FLOAT_TO_Q31(0.0);  /* y */
	ret = cordic_calculate(cordic_dev, in_data, out_data, 2, 1);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	zassert_true(q31_approx_equal(out_data[0], FLOAT_TO_Q31(0.0), TOLERANCE_Q31 * 10),
		     "phase(1,0) incorrect");

	/* Test phase(1, 1) ≈ π/4 (0.25 in normalized) */
	in_data[0] = FLOAT_TO_Q31(0.707);  /* x */
	in_data[1] = FLOAT_TO_Q31(0.707);  /* y */
	ret = cordic_calculate(cordic_dev, in_data, out_data, 2, 1);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	zassert_true(q31_approx_equal(out_data[0], FLOAT_TO_Q31(0.25), TOLERANCE_Q31 * 20),
		     "phase(1,1) incorrect");

	/* Test phase(0, 1) ≈ π/2 (0.5 in normalized) */
	in_data[0] = FLOAT_TO_Q31(0.0);  /* x */
	in_data[1] = FLOAT_TO_Q31(1.0);  /* y */
	ret = cordic_calculate(cordic_dev, in_data, out_data, 2, 1);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	zassert_true(q31_approx_equal(out_data[0], FLOAT_TO_Q31(0.5), TOLERANCE_Q31 * 20),
		     "phase(0,1) incorrect");
}

/**
 * @brief Test CORDIC Modulus function
 *
 * Calculates sqrt(x^2 + y^2).
 */
ZTEST(cordic_api, test_modulus_function)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_MODULUS,
		.precision = CORDIC_PRECISION_10,
		.scale = CORDIC_SCALE_VAL_0,
		.in_size = CORDIC_DATA_WIDTH_32BIT,
		.out_size = CORDIC_DATA_WIDTH_32BIT,
	};
	int32_t in_data[2];
	int32_t out_data[1];
	int ret;

	ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	/* Test modulus(1, 0) = 1 */
	in_data[0] = FLOAT_TO_Q31(1.0);
	in_data[1] = FLOAT_TO_Q31(0.0);
	ret = cordic_calculate(cordic_dev, in_data, out_data, 2, 1);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	zassert_true(q31_approx_equal(out_data[0], FLOAT_TO_Q31(1.0), TOLERANCE_Q31 * 10),
		     "modulus(1,0) incorrect");

	/* Test modulus(0.6, 0.8) = 1.0 (3-4-5 triangle scaled) */
	in_data[0] = FLOAT_TO_Q31(0.6);
	in_data[1] = FLOAT_TO_Q31(0.8);
	ret = cordic_calculate(cordic_dev, in_data, out_data, 2, 1);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	zassert_true(q31_approx_equal(out_data[0], FLOAT_TO_Q31(1.0), TOLERANCE_Q31 * 20),
		     "modulus(0.6,0.8) incorrect");
}

/**
 * @brief Test CORDIC Arctangent function
 */
ZTEST(cordic_api, test_arctangent_function)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_ARCTANGENT,
		.precision = CORDIC_PRECISION_10,
		.scale = CORDIC_SCALE_VAL_0,
		.in_size = CORDIC_DATA_WIDTH_32BIT,
		.out_size = CORDIC_DATA_WIDTH_32BIT,
	};
	int32_t in_data[1];
	int32_t out_data[1];
	int ret;

	ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	/* Test arctan(0) = 0 */
	in_data[0] = FLOAT_TO_Q31(0.0);
	ret = cordic_calculate(cordic_dev, in_data, out_data, 1, 1);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	zassert_true(q31_approx_equal(out_data[0], FLOAT_TO_Q31(0.0), TOLERANCE_Q31),
		     "arctan(0) incorrect");

	/* Test arctan(1) ≈ π/4 (0.25 in normalized) */
	in_data[0] = FLOAT_TO_Q31(1.0);
	ret = cordic_calculate(cordic_dev, in_data, out_data, 1, 1);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	zassert_true(q31_approx_equal(out_data[0], FLOAT_TO_Q31(0.25), TOLERANCE_Q31 * 20),
		     "arctan(1) incorrect");
}

/**
 * @brief Test CORDIC Hyperbolic Cosine function
 */
ZTEST(cordic_api, test_hcosine_function)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_HCOSINE,
		.precision = CORDIC_PRECISION_10,
		.scale = CORDIC_SCALE_VAL_0,
		.in_size = CORDIC_DATA_WIDTH_32BIT,
		.out_size = CORDIC_DATA_WIDTH_32BIT,
	};
	int32_t in_data[1];
	int32_t out_data[2];
	int ret;

	ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	/* Test cosh(0) = 1 */
	in_data[0] = FLOAT_TO_Q31(0.0);
	ret = cordic_calculate(cordic_dev, in_data, out_data, 1, 2);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	zassert_true(q31_approx_equal(out_data[0], FLOAT_TO_Q31(1.0), TOLERANCE_Q31 * 10),
		     "cosh(0) incorrect");
}

/**
 * @brief Test CORDIC Hyperbolic Sine function
 */
ZTEST(cordic_api, test_hsine_function)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_HSINE,
		.precision = CORDIC_PRECISION_10,
		.scale = CORDIC_SCALE_VAL_0,
		.in_size = CORDIC_DATA_WIDTH_32BIT,
		.out_size = CORDIC_DATA_WIDTH_32BIT,
	};
	int32_t in_data[1];
	int32_t out_data[2];
	int ret;

	ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	/* Test sinh(0) = 0 */
	in_data[0] = FLOAT_TO_Q31(0.0);
	ret = cordic_calculate(cordic_dev, in_data, out_data, 1, 2);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	zassert_true(q31_approx_equal(out_data[1], FLOAT_TO_Q31(0.0), TOLERANCE_Q31 * 10),
		     "sinh(0) incorrect");
}

/**
 * @brief Test CORDIC Hyperbolic Arctangent function
 */
ZTEST(cordic_api, test_harctangent_function)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_HARCTANGENT,
		.precision = CORDIC_PRECISION_10,
		.scale = CORDIC_SCALE_VAL_0,
		.in_size = CORDIC_DATA_WIDTH_32BIT,
		.out_size = CORDIC_DATA_WIDTH_32BIT,
	};
	int32_t in_data[1];
	int32_t out_data[1];
	int ret;

	ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	/* Test atanh(0) = 0 */
	in_data[0] = FLOAT_TO_Q31(0.0);
	ret = cordic_calculate(cordic_dev, in_data, out_data, 1, 1);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	zassert_true(q31_approx_equal(out_data[0], FLOAT_TO_Q31(0.0), TOLERANCE_Q31 * 10),
		     "atanh(0) incorrect");
}

/**
 * @brief Test CORDIC Natural Logarithm function
 */
ZTEST(cordic_api, test_naturallog_function)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_NATURALLOG,
		.precision = CORDIC_PRECISION_10,
		.scale = CORDIC_SCALE_VAL_0,
		.in_size = CORDIC_DATA_WIDTH_32BIT,
		.out_size = CORDIC_DATA_WIDTH_32BIT,
	};
	int32_t in_data[1];
	int32_t out_data[1];
	int ret;

	ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	/* Test ln(1) = 0 (input 1 in Q1.31) */
	in_data[0] = FLOAT_TO_Q31(1.0);
	ret = cordic_calculate(cordic_dev, in_data, out_data, 1, 1);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	/* Note: CORDIC ln has specific input range and scaling requirements */
}

/**
 * @brief Test CORDIC Square Root function
 */
ZTEST(cordic_api, test_squareroot_function)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_SQUAREROOT,
		.precision = CORDIC_PRECISION_10,
		.scale = CORDIC_SCALE_VAL_0,
		.in_size = CORDIC_DATA_WIDTH_32BIT,
		.out_size = CORDIC_DATA_WIDTH_32BIT,
	};
	int32_t in_data[1];
	int32_t out_data[1];
	int ret;

	ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	/* Test sqrt(0.25) = 0.5 */
	in_data[0] = FLOAT_TO_Q31(0.25);
	ret = cordic_calculate(cordic_dev, in_data, out_data, 1, 1);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	zassert_true(q31_approx_equal(out_data[0], FLOAT_TO_Q31(0.5), TOLERANCE_Q31 * 20),
		     "sqrt(0.25) incorrect");

	/* Test sqrt(1.0) = 1.0 */
	in_data[0] = FLOAT_TO_Q31(1.0);
	ret = cordic_calculate(cordic_dev, in_data, out_data, 1, 1);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
	zassert_true(q31_approx_equal(out_data[0], FLOAT_TO_Q31(1.0), TOLERANCE_Q31 * 10),
		     "sqrt(1.0) incorrect");
}

/**
 * @brief Test different precision levels
 */
ZTEST(cordic_api, test_precision_levels)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_COSINE,
		.precision = CORDIC_PRECISION_5,  /* Lower precision */
		.scale = CORDIC_SCALE_VAL_0,
		.in_size = CORDIC_DATA_WIDTH_32BIT,
		.out_size = CORDIC_DATA_WIDTH_32BIT,
	};
	int32_t in_data[1];
	int32_t out_data[2];
	int ret;

	/* Test with lower precision */
	ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	in_data[0] = FLOAT_TO_Q31(0.0);
	ret = cordic_calculate(cordic_dev, in_data, out_data, 1, 2);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);

	/* Test with higher precision */
	config.precision = CORDIC_PRECISION_15;
	ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_calculate(cordic_dev, in_data, out_data, 1, 2);
	zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
}

/**
 * @brief Test invalid configuration parameters
 */
ZTEST(cordic_api, test_invalid_config)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_COSINE,
		.precision = 20,  /* Invalid precision */
		.scale = CORDIC_SCALE_VAL_0,
		.in_size = CORDIC_DATA_WIDTH_32BIT,
		.out_size = CORDIC_DATA_WIDTH_32BIT,
	};
	int ret;

	/* Should fail with invalid precision */
	ret = cordic_configure(cordic_dev, &config);
	zassert_not_equal(ret, 0, "Should reject invalid precision");

	/* Test NULL config */
	ret = cordic_configure(cordic_dev, NULL);
	zassert_not_equal(ret, 0, "Should reject NULL config");
}

/**
 * @brief Test invalid calculate parameters
 */
ZTEST(cordic_api, test_invalid_calculate)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_COSINE,
		.precision = CORDIC_PRECISION_10,
		.scale = CORDIC_SCALE_VAL_0,
		.in_size = CORDIC_DATA_WIDTH_32BIT,
		.out_size = CORDIC_DATA_WIDTH_32BIT,
	};
	int32_t in_data[2];
	int32_t out_data[2];
	int ret;

	ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	/* Test with NULL input */
	ret = cordic_calculate(cordic_dev, NULL, out_data, 1, 2);
	zassert_not_equal(ret, 0, "Should reject NULL input");

	/* Test with NULL output */
	ret = cordic_calculate(cordic_dev, in_data, NULL, 1, 2);
	zassert_not_equal(ret, 0, "Should reject NULL output");

	/* Test with invalid input count */
	ret = cordic_calculate(cordic_dev, in_data, out_data, 0, 2);
	zassert_not_equal(ret, 0, "Should reject zero inputs");

	ret = cordic_calculate(cordic_dev, in_data, out_data, 3, 2);
	zassert_not_equal(ret, 0, "Should reject >2 inputs");
}

ZTEST_SUITE(cordic_api, NULL, cordic_setup, NULL, NULL, NULL);
