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
				      const float *arg_in,
				      float *arg_out,
				      const uint32_t arg_in_len,
				      const uint32_t arg_out_len);

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
 * @brief Perform a CORDIC calculation
 *
 * Executes a CORDIC operation using the current configuration.
 * Inputs and outputs are IEEE-754 single-precision floats.
 *
 * @note Since CORDIC is HW based implementation the calculations in behind are
 * performed in fixed-point format (Q1.15 or Q1.31 depending on configuration).
 *
 * For functions requiring:
 * - 1 input: arg_in[0] contains the input value
 * - 2 inputs: arg_in[0] and arg_in[1] contain the input values
 *
 * For functions producing:
 * - 1 output: arg_out[0] receives the result
 * - 2 outputs: arg_out[0] and arg_out[1] receive the results
 *
 * @note Only up to two input and/or two output values are supported. Ensure that
 * the lengths match the selected function; extra values may be ignored.
 *
 * @param dev Pointer to the device structure for the CORDIC controller
 * @param arg_in Pointer to input data array (can be NULL if @p arg_in_len is 0)
 * @param arg_out Pointer to output data array (must not be NULL if output is expected)
 * @param arg_in_len Number of input values in @p arg_in (0–2, per function requirements)
 * @param arg_out_len Capacity of @p arg_out (1–2, per function results)
 *
 * @retval 0 If successful
 * @retval -errno Negative errno code on failure
 */
__syscall int cordic_calculate(const struct device *dev,
			       const float *arg_in,
			       float *arg_out,
			       const uint32_t arg_in_len,
			       const uint32_t arg_out_len);

static inline int z_impl_cordic_calculate(const struct device *dev,
					  const float *arg_in,
					  float *arg_out,
					  const uint32_t arg_in_len,
					  const uint32_t arg_out_len)
{
	const struct cordic_driver_api *api =
		(const struct cordic_driver_api *)dev->api;

	if (api->calculate == NULL) {
		return -ENOSYS;
	}

	return api->calculate(dev, arg_in, arg_out, arg_in_len, arg_out_len);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/cordic.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_CORDIC_H_ */
