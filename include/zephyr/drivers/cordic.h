/*
 * Copyright (c) 2024 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CORDIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_CORDIC_H_

/**
 * @brief CORDIC Interface
 * @defgroup cordic_interface CORDIC Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CORDIC function selection
 */
enum cordic_function {
	/** Cosine function */
	CORDIC_FUNC_COSINE,
	/** Sine function */
	CORDIC_FUNC_SINE,
	/** Phase (arctan2) function */
	CORDIC_FUNC_PHASE,
	/** Modulus (sqrt(x^2+y^2)) function */
	CORDIC_FUNC_MODULUS,
	/** Arctangent function */
	CORDIC_FUNC_ARCTANGENT,
	/** Hyperbolic Cosine function */
	CORDIC_FUNC_HCOSINE,
	/** Hyperbolic Sine function */
	CORDIC_FUNC_HSINE,
	/** Hyperbolic Arctangent function */
	CORDIC_FUNC_HARCTANGENT,
	/** Natural Logarithm function */
	CORDIC_FUNC_NATURALLOG,
	/** Square Root function */
	CORDIC_FUNC_SQUAREROOT,
};

/**
 * @brief CORDIC precision in calculation cycles
 * Values represent the number of iteration cycles (1-15)
 */
enum cordic_precision {
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

/**
 * @brief CORDIC scale factor (number of iterations)
 * Values range from 0 to 7
 */
enum cordic_scale {
	CORDIC_SCALE_VAL_0,
	CORDIC_SCALE_VAL_1,
	CORDIC_SCALE_VAL_2,
	CORDIC_SCALE_VAL_3,
	CORDIC_SCALE_VAL_4,
	CORDIC_SCALE_VAL_5,
	CORDIC_SCALE_VAL_6,
	CORDIC_SCALE_VAL_7,
};

/**
 * @brief CORDIC data width
 */
enum cordic_data_width {
	/** 32-bit data width (Q1.31 format) */
	CORDIC_DATA_WIDTH_32BIT = 32,
	/** 16-bit data width (Q1.15 format) */
	CORDIC_DATA_WIDTH_16BIT = 16,
};

/**
 * @brief CORDIC configuration structure
 */
struct cordic_config {
	/** Function to compute */
	enum cordic_function function;
	/** Precision (number of cycles) */
	enum cordic_precision precision;
	/** Scale factor */
	enum cordic_scale scale;
	/** Input data width */
	enum cordic_data_width in_width;
	/** Output data width */
	enum cordic_data_width out_width;
};

/**
 * @typedef cordic_api_configure_t
 * @brief Callback API for configuring CORDIC computation
 *
 * @see cordic_configure() for argument descriptions
 */
typedef int (*cordic_api_configure_t)(const struct device *dev,
				      const struct cordic_config *config);

/**
 * @typedef cordic_api_calculate_t
 * @brief Callback API for performing CORDIC calculation
 *
 * @see cordic_calculate() for argument descriptions
 */
typedef int (*cordic_api_calculate_t)(const struct device *dev,
				      int *arg_in,
				      int *arg_out,
				      float *arg_out_float,
				      uint32_t arg_in_len,
				      uint32_t arg_out_len);

/**
 * @brief CORDIC driver API
 */
__subsystem struct cordic_driver_api {
	cordic_api_configure_t configure;
	cordic_api_calculate_t calculate;
};

/**
 * @brief Configure CORDIC for a specific computation
 *
 * This function configures the CORDIC peripheral with the desired function,
 * precision, scale factor, and data widths.
 *
 * @param dev Pointer to the device structure for the CORDIC controller
 * @param config Pointer to CORDIC configuration structure
 *
 * @retval 0 If successful
 * @retval -errno Negative errno code on failure
 */
__syscall int cordic_configure(const struct device *dev,
			       const struct cordic_config *config);

static inline int z_impl_cordic_configure(const struct device *dev,
					  const struct cordic_config *config)
{
	const struct cordic_driver_api *api =
		(const struct cordic_driver_api *)dev->api;

	if (api->configure == NULL) {
		return -ENOSYS;
	}

	return api->configure(dev, config);
}

/**
 * @brief Perform CORDIC calculation
 *
 * This function performs a CORDIC calculation with the previously configured
 * parameters. The input and output are in Q1.31 or Q1.15 fixed-point format
 * depending on the configured data width.
 *
 * For functions requiring:
 * - 1 input: arg_in[0] contains the input value
 * - 2 inputs: arg_in[0] and arg_in[1] contain the input values
 *
 * For functions producing:
 * - 1 output: arg_out[0] receives the result
 * - 2 outputs: arg_out[0] and arg_out[1] receive the results
 *
 * @note Due to CORDIC implementation nature the are up to two input and/or two output
 * values supported. Ensure that the number of input and output values matches the
 * function used.
 *
 * @note If choosen function does not support two arguments and/or results then
 * the second argument/result will be ignored or will not provide valid data.
 *
 * @param dev Pointer to the device structure for the CORDIC controller
 * @param arg_in Pointer to input data array
 * @param arg_out Pointer to output data array
 * @param arg_out_float Pointer to output data array in float format
 *			(same length as arg_out). Use NULL if not float result is not needed.
 * @param arg_in_len The length of @p arg_in array
 * @param arg_out_len The length of @p arg_out array
 *
 * @retval 0 If successful
 * @retval -errno Negative errno code on failure
 */
__syscall int cordic_calculate(const struct device *dev,
			       int *arg_in,
			       int *arg_out,
			       float *arg_out_float,
			       uint32_t arg_in_len,
			       uint32_t arg_out_len);

static inline int z_impl_cordic_calculate(const struct device *dev,
					  int *arg_in,
					  int *arg_out,
					  float *arg_out_float,
					  uint32_t arg_in_len,
					  uint32_t arg_out_len)
{
	const struct cordic_driver_api *api =
		(const struct cordic_driver_api *)dev->api;

	if (api->calculate == NULL) {
		return -ENOSYS;
	}

	return api->calculate(dev, arg_in, arg_out, arg_out_float, arg_in_len, arg_out_len);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/cordic.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_CORDIC_H_ */
