/*
 * Copyright (c) 2024 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/cordic.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>
#include <math.h>

#define CORDIC_NODE DT_NODELABEL(cordic)

/* The tolerance is choosen intentionally low to keep all possible platforms pass.
 * If lower tolerance is needed a separate platform specific test shall be created.
 */
#define TOLERANCE_F32 (1e-3f)

#define PI_F32 (3.14159265358979323846f)


typedef struct {
	const struct device *const dev;  /* Pointer to CORDIC device */
	struct cordic_config config; /* Pointer to CORDIC device configuration */
	const float *const args1; /* first argument array */
	const float *const args2; /* second argument array - pass NULL if not applicable */
	const float *const exp_res1; /* expected first result array */
	const float *const exp_res2; /* expected second result array - pass NULL if not applicable */
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

	zassert_true(device_is_ready(cordic_dev),
		     "CORDIC device %s not ready", cordic_dev->name);
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
		/* TODO: Handle 1 or 2 possible arguments */
		float args[2] = { run_cfg->args1[i], run_cfg->args2[i] };
		float res[2];
		const uint32_t args_len = ARRAY_SIZE(args);
		const uint32_t res_len = ARRAY_SIZE(res);

		ret = cordic_calculate(run_cfg->dev, args, res, args_len, res_len);
		zassert_equal(ret, 0, "CORDIC calculate failed: %d", ret);
		zassert_true(float_cmp(res[0], run_cfg->exp_res1[i], run_cfg->tolerance),
			     "idx: %d: in: %s: out: %s: FN: fn(x,y) = %s: RESULT: fn(%f,%f) = %f EXPECTED: %f TOLERANCE: %f",
			     i, in_width_str, out_width_str,
			     run_cfg->fn1_str == NULL ? "unknown" :run_cfg->fn1_str,
			     (double)args[0], (double)args[1], (double)res[0],
			     (double)run_cfg->exp_res1[i],
			     (double)run_cfg->tolerance);
		if (run_cfg->exp_res2 == NULL) {
			continue;
		}
		zassert_true(float_cmp(res[1], run_cfg->exp_res2[i], run_cfg->tolerance),
			     "in: %s: out: %s: idx: %d: FN: fn(x,y) = %s: RESULT: fn(%f,%f) = %f EXPECTED: %f TOLERANCE: %f",
			     in_width_str, out_width_str, i,
			     run_cfg->fn2_str == NULL ? "unknown" :run_cfg->fn2_str,
			     (double)args[0], (double)args[1], (double)res[1],
			     (double)(run_cfg->exp_res2[i]),
			     (double)run_cfg->tolerance);
	}
}

/**
 * @brief Test CORDIC Phase function
 */
ZTEST(cordic_api, test_phase)
{
	const float args1[] = {1.0f, 0.5f, 0.5f}; /* x - coordinate in radians */
	const float args2[] = {0.0f, 0.5f, 0.25f}; /* y - coordinate in radians */
	const float exp_res1[] = {0.0f, 0.785398f, 0.463648f}; /* atan2(y,x) */
	const float exp_res2[] = {1.0f, 0.707107f, 0.559017f}; /* m = sqrt(x^2 + y^2) - modulus */

	/* A test for q31_t format */
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_PHASE,
			.precision = CORDIC_PRECISION_10,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = args2,
		.exp_res1 = exp_res1,
		.exp_res2 = exp_res2,
		.size = ARRAY_SIZE(args1),
		.tolerance = TOLERANCE_F32,
		.fn1_str = "atan2(y,x)",
		.fn2_str = "sqrt(x^2 + y^2)",
	};
	run_function_test(&run_cfg);

	/* A test for q15_t format */
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}

/**
 * @brief Test CORDIC Modulus function
 *
 * Calculates sqrt(x^2 + y^2).
 */
ZTEST(cordic_api, test_modulus)
{
	const float args1[] = {0.456f, 0.123f, 1.0f, 0.0f}; /* x - coordinate */
	const float args2[] = {0.123f, 0.456f, 0.0f, 1.0f}; /* y - coordinate */
	const float exp_res1[] = {0.472298f, 0.472298f, 1.0f, 1.0f}; /* modulus: m = sqrt(x^2 + y^2) */
	const float exp_res2[] = {0.26346654f, 1.3073297f, 0.0f, 1.57079633f}; /* phase angle: atan2(y,x) */

	/* A test for q31_t format */
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_MODULUS,
			.precision = CORDIC_PRECISION_10,
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
	const float args1[] = {0.027f, 0.25f, 0.74f};
	const float exp_res1[] = {0.164317f, 0.5f, 0.86023f};

	/* A test for q31_t format */
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_SQUAREROOT,
			.precision = CORDIC_PRECISION_2,
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

/**
 * @brief Test CORDIC Natural Logarithm function
 */
ZTEST(cordic_api, test_natural_log)
{
	const float args1[] = {0.107f, 0.95f};
	const float exp_res1[] = {-2.2349267f, -0.051293f};

	/* A test for q31_t format */
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_NATURALLOG,
			.precision = CORDIC_PRECISION_15,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = NULL,
		.exp_res1 = exp_res1,
		.exp_res2 = NULL,
		.size = ARRAY_SIZE(args1),
		.tolerance = 0.1f,  /* natural logarithm may have higher error */
		.fn1_str = "ln(x)",
	};
	run_function_test(&run_cfg);

	/* A test for q15_t format */
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}

/**
 * @brief Test CORDIC Cosine function
 */
ZTEST(cordic_api, test_cosine)
{
	const float args1[] = {PI_F32/2.0f, PI_F32, PI_F32/4.0f}; /* x - angle in radians */
	const float args2[] = {0.25f, 0.5f, 0.1f}; /* y - modulus */
	const float exp_res1[] = {0.0f, -0.5f, 0.0707107f}; /* y * cos(x) */
	const float exp_res2[] = {0.25f, 0.0f, 0.0707107f}; /* y * sin(x) */

	/* A test for q31_t format */
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_COSINE,
			.precision = CORDIC_PRECISION_10,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = args2,
		.exp_res1 = exp_res1,
		.exp_res2 = exp_res2,
		.size = ARRAY_SIZE(args1),
		.tolerance = TOLERANCE_F32,
		.fn1_str = "y * cos(x)",
		.fn2_str = "y * sin(x)",
	};
	run_function_test(&run_cfg);

	/* A test for q15_t format */
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}

/**
 * @brief Test CORDIC Sine function
 */
ZTEST(cordic_api, test_sine)
{
	const float args1[] = {PI_F32/2.0f, PI_F32, PI_F32/4.0f}; /* x - angle in radians */
	const float args2[] = {0.25f, 0.5f, 0.1f}; /* y - modulus */
	const float exp_res1[] = {0.25f, 0.0f, 0.0707107f}; /* y * sin(x) */
	const float exp_res2[] = {0.0f, -0.5f, 0.0707107f}; /* y * cos(x) */

	/* A test for q31_t format */
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_SINE,
			.precision = CORDIC_PRECISION_10,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = args2,
		.exp_res1 = exp_res1,
		.exp_res2 = exp_res2,
		.size = ARRAY_SIZE(args1),
		.tolerance = TOLERANCE_F32,
		.fn1_str = "y * sin(x)",
		.fn2_str = "y * cos(x)",
	};
	run_function_test(&run_cfg);

	/* A test for q15_t format */
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}

/**
 * @brief Test CORDIC Hyperbolic Cosine function
 */
ZTEST(cordic_api, test_hcosine)
{
	const float args1[] = {-0.559f, 0.25f, 0.559f}; /* x */
	const float exp_res1[] = {1.16035163f, 1.0314131f,  1.16035163f}; /* cosh(x) */
	const float exp_res2[] = {-0.58857107f, 0.25261232f, 0.58857107f}; /* sinh(x) */

	/* A test for q31_t format */
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_HCOSINE,
			.precision = CORDIC_PRECISION_15,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = NULL,
		.exp_res1 = exp_res1,
		.exp_res2 = exp_res2,
		.size = ARRAY_SIZE(args1),
		.tolerance = TOLERANCE_F32 * 10.0f,  /* lower tolerance since not precise */
		.fn1_str = "cosh(x)",
		.fn2_str = "sinh(x)",
	};
	run_function_test(&run_cfg);

	/* A test for q15_t format */
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}

/**
 * @brief Test CORDIC Hyperbolic Sine function
 */
ZTEST(cordic_api, test_hsine)
{
	const float args1[] = {-0.559f, 0.25f, 0.559f}; /* x */
	const float exp_res1[] = {-0.58857107f, 0.25261232f, 0.58857107f}; /* sinh(x) */
	const float exp_res2[] = {1.16035163f, 1.0314131f,  1.16035163f}; /* cosh(x) */

	/* A test for q31_t format */
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_HSINE,
			.precision = CORDIC_PRECISION_15,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = NULL,
		.exp_res1 = exp_res1,
		.exp_res2 = exp_res2,
		.size = ARRAY_SIZE(args1),
		.tolerance = TOLERANCE_F32 * 10.0f,  /* lower tolerance since not precise */
		.fn1_str = "sinh(x)",
		.fn2_str = "cosh(x)",
	};
	run_function_test(&run_cfg);

	/* A test for q15_t format */
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}

/**
 * @brief Test CORDIC Arctangent function
 */
ZTEST(cordic_api, test_arctangent)
{
	const float args1[] = {128.0f}; /* x */
	const float exp_res1[] = {1.562942f}; /* atan(x) */

	/* A test for q31_t format */
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_ARCTANGENT,
			.precision = CORDIC_PRECISION_15,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = NULL,
		.exp_res1 = exp_res1,
		.exp_res2 = NULL,
		.size = ARRAY_SIZE(args1),
		.tolerance = TOLERANCE_F32,
		.fn1_str = "atan(x)",
		.fn2_str = NULL,
	};
	run_function_test(&run_cfg);

	/* A test for q15_t format */
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}

/**
 * @brief Test CORDIC Hyperbolic Arctangent function
 */
ZTEST(cordic_api, test_harctangent)
{
	const float args1[] = {0.0f, 0.5f, 0.9f};
	const float args2[] = {0.0f, 0.0f, 0.0f};
	const float exp_res1[] = {0.0f, 0.5493061f, 1.4722193f};

	/* A test for q31_t format */
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_HARCTANGENT,
			.precision = CORDIC_PRECISION_10,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = args2,
		.exp_res1 = exp_res1,
		.exp_res2 = NULL,
		.size = ARRAY_SIZE(args1),
		.tolerance = TOLERANCE_F32,
		.fn1_str = "atanh(x)",
		.fn2_str = NULL,
	};
	run_function_test(&run_cfg);

	/* A test for q15_t format */
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}


ZTEST_SUITE(cordic_api, NULL, cordic_setup, NULL, NULL, NULL);
