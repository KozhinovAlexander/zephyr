/*
 * Copyright (c) 2024 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/cordic.h>
#include <zephyr/dsp/types.h>
#include <zephyr/dsp/utils.h>
#include <zephyr/dsp/dsp.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>

#define CORDIC_NODE DT_NODELABEL(cordic)

#define PI_F32 (3.14159265358979323846f)

#define F32_TO_Q31(v) Z_SHIFT_F32_TO_Q31(v, 0)
#define Q31_TO_F32(v) Z_SHIFT_Q31_TO_F32(v, 0)

#define F16_TO_Q15(v) Z_SHIFT_F32_TO_Q15(v, 0)
#define Q15_TO_F16(v) Z_SHIFT_Q15_TO_F32(v, 0)

/* The tolerance is choosen intentionally low to keep all possible platforms pass.
 * If lower tolerance is needed a separate platform specific test shall be created.
 */
#define TOLERANCE_F32 (1e-3f)

static inline float int_to_float(const int val, const uint32_t width)
{
	return width == CORDIC_DATA_WIDTH_16BIT ?
		(float32_t)Q15_TO_F16((int16_t)val) :
		(float32_t)Q31_TO_F32((int32_t)val);
}

static inline int float_to_int(const float val, const uint32_t width)
{
	return width == CORDIC_DATA_WIDTH_16BIT ?
		(int16_t)F16_TO_Q15((float16_t)val) :
		(int32_t)F32_TO_Q31((float32_t)val);
}

typedef struct {
	const struct device *const dev;  /* Pointer to CORDIC device */
	struct cordic_config config; /* Pointer to CORDIC device configuration */
	const float32_t *const args1; /* first argument array */
	const float32_t *const args2; /* second argument array - pass NULL if not applicable */
	const float32_t *const exp_res1; /* expected first result array */
	const float32_t *const exp_res2; /* expected second result array - pass NULL if not applicable */
	/* size of the arrays. All arrays shall have same size if not NULL. Pass f32 NaN to an
	 * array element if it is not applicable.
	 */
	const size_t size;
	float tolerance; /* tolerance for comparison */
	const char *const fn1_str; /* first cordic function string */
	const char *const fn2_str; /* second cordic function string */
} cordic_run_config_t;

/* Test setup function */
static void *cordic_setup(void)
{
	static const struct device *const cordic_dev = DEVICE_DT_GET(CORDIC_NODE);

	zassert_true(device_is_ready(cordic_dev), "CORDIC device not ready");
	return NULL;
}

/**
 * @brief Compare float values a and b within given tolerance
 *
 * @param a - first value
 * @param b - second value
 * @param tolerance - tolerance value for comparison
 * @return true - values are equal within tolerance
 * @return false - values are not equal within tolerance
 */
static bool float_cmp(const float a, const float b, const float tolerance)
{
	const float d = a - b;
	return ((-1 * tolerance) <= d) && (d <= (tolerance));
}


/**
 * @brief Test CORDIC Configure function. This test is allowed to apply some
 * arbitrary configuration.
 * @note: If a target platform does not support certain configurations,
 * it may better to create a separate test for the platform itself.
 */
ZTEST(cordic_api, test_cordic_configure)
{
	static const struct device *const cordic_dev = DEVICE_DT_GET(CORDIC_NODE);
	struct cordic_config config = {
		.function = CORDIC_FUNC_SINE,
		.precision = CORDIC_PRECISION_1,
		.scale = CORDIC_SCALE_VAL_1,
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

	/* Iterate through all possible function configurations */
	for(int i = 0; i < ARRAY_SIZE(fn_list); i++) {
		config.function = fn_list[i];
		ret = cordic_configure(cordic_dev, &config);
		zassert_equal(ret, 0, "%s: CORDIC function configure failed: %d",
			      cordic_dev->name, ret);
	}

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

	/* Iterate through all possible precision configurations */
	for(int i = 0; i < ARRAY_SIZE(prec_list); i++) {
		config.precision = prec_list[i];
		ret = cordic_configure(cordic_dev, &config);
		zassert_equal(ret, 0, "CORDIC precision configure failed: %d", ret);
	}

	const enum cordic_scale scale_list[] = {
		CORDIC_SCALE_VAL_0,
		CORDIC_SCALE_VAL_1,
		CORDIC_SCALE_VAL_2,
		CORDIC_SCALE_VAL_3,
		CORDIC_SCALE_VAL_4,
		CORDIC_SCALE_VAL_5,
		CORDIC_SCALE_VAL_6,
		CORDIC_SCALE_VAL_7,
	};

	/* Iterate through all possible scale configurations */
	for(int i = 0; i < ARRAY_SIZE(scale_list); i++) {
		config.scale = scale_list[i];
		ret = cordic_configure(cordic_dev, &config);
		zassert_equal(ret, 0, "CORDIC scale configure failed: %d", ret);
	}

	const enum cordic_data_width data_width_list[] = {
		CORDIC_DATA_WIDTH_32BIT,
		CORDIC_DATA_WIDTH_16BIT,
	};

	/* Iterate through all possible input data width configurations for
	 * input size.
	 */
	for(int i = 0; i < ARRAY_SIZE(data_width_list); i++) {
		config.in_width = data_width_list[i];
		ret = cordic_configure(cordic_dev, &config);
		zassert_equal(ret, 0, "CORDIC input data width configure failed: %d", ret);
	}

	/* Iterate through all possible input data width configurations for
	 * output size.
	 */
	for(int i = 0; i < ARRAY_SIZE(data_width_list); i++) {
		config.out_width = data_width_list[i];
		ret = cordic_configure(cordic_dev, &config);
		zassert_equal(ret, 0, "CORDIC output data width configure failed: %d", ret);
	}

}

/**
 * @brief Executes function test for given configuration and input/output data
 * @param rin_cfg - pointer to run configuration structure
 */
static inline void run_function_test(const cordic_run_config_t *const run_cfg)
{
	int ret = cordic_configure(run_cfg->dev, &run_cfg->config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	const char* in_width_str = run_cfg->config.in_width == CORDIC_DATA_WIDTH_32BIT ? "Q1.31" : "Q1.15";
	const char* out_width_str = run_cfg->config.out_width == CORDIC_DATA_WIDTH_32BIT ? "Q1.31" : "Q1.15";

	for (size_t i = 0; i < run_cfg->size; i++) {
		q31_t args[2] = {
			float_to_int(run_cfg->args1[i], run_cfg->config.out_width),
			float_to_int(run_cfg->args2[i], run_cfg->config.out_width)
		};
		q31_t int_res[2];
		float32_t float_res[2];
		const uint32_t args_len = ARRAY_SIZE(args);
		const uint32_t res_len = ARRAY_SIZE(int_res);

		ret = cordic_calculate(run_cfg->dev, args, int_res, float_res, args_len, res_len);
		zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
		zassert_true(float_cmp(float_res[0], run_cfg->exp_res1[i], run_cfg->tolerance),
			     "idx: %d: in: %s: out: %s: FN: fn(x,y) = %s: RESULT: fn(%f,%f) = %f EXPECTED: %f TOLERANCE: %f",
			     i, in_width_str, out_width_str,
			     run_cfg->fn1_str == NULL ? "unknown" :run_cfg->fn1_str,
			     (double)int_to_float(args[0], run_cfg->config.out_width),
			     (double)int_to_float(args[1], run_cfg->config.out_width),
			     (double)float_res[0],
			     (double)run_cfg->exp_res1[i],
			     (double)run_cfg->tolerance);
		if (run_cfg->exp_res2 == NULL) {
			continue;
		}
		zassert_true(float_cmp(float_res[1], run_cfg->exp_res2[i], run_cfg->tolerance),
			     "in: %s: out: %s: idx: %d: FN: fn(x,y) = %s: RESULT: fn(%f,%f) = %f EXPECTED: %f TOLERANCE: %f",
			     in_width_str, out_width_str, i,
			     run_cfg->fn2_str == NULL ? "unknown" :run_cfg->fn2_str,
			     (double)int_to_float(args[0], run_cfg->config.out_width),
			     (double)int_to_float(args[1], run_cfg->config.out_width),
			     (double)float_res[1],
			     (double)(run_cfg->exp_res2[i]),
			     (double)run_cfg->tolerance);
	}
}

/**
 * @brief Test CORDIC Modulus function
 *
 * Calculates sqrt(x^2 + y^2).
 */
ZTEST(cordic_api, test_modulus)
{
	const float32_t args1[] = {0.456f, 0.123f, 1.0f, 0.0f};
	const float32_t args2[] = {0.123f, 0.456f, 0.0f, 1.0f};
	const float32_t exp_res1[] = {0.472298f, 0.472298f, 1.0f, 1.0f};
	const float32_t exp_res2[] = {
		0.26346654f,
		1.3073297f,
		0.0f,
		1.57079633f,
	};

	/* A test for q31_t format */
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_MODULUS,
			.precision = CORDIC_PRECISION_10,
			.scale = CORDIC_SCALE_VAL_0,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = args2,
		.exp_res1 = exp_res1,
		.exp_res2 = exp_res2,
		.size = ARRAY_SIZE(args1),
		.tolerance = TOLERANCE_F32,
		.fn1_str = "sqrt(x^2 + y^2)",
		.fn2_str = "atan2(y,x)",
	};
	run_function_test(&run_cfg);

	/* A test for q15_t format */
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}

/**
 * @brief Test CORDIC Square Root function
 */
ZTEST(cordic_api, test_squareroot)
{
	const float32_t args1[] = {0.25f, 0.75f};
	const float32_t exp_res1[] = {0.5f, 0.866025f};

	/* A test for q31_t format */
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_SQUAREROOT,
			.precision = CORDIC_PRECISION_2,
			.scale = CORDIC_SCALE_VAL_0,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = NULL,
		.exp_res1 = exp_res1,
		.exp_res2 = NULL,
		.size = ARRAY_SIZE(args1),
		.tolerance = TOLERANCE_F32,
		.fn1_str = "sqrt(x)",
	};
	run_function_test(&run_cfg);

	/* A test for q15_t format */
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}


ZTEST_SUITE(cordic_api, NULL, cordic_setup, NULL, NULL, NULL);
