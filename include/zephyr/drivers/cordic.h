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
	CORDIC_FUNC_COSINE = 0x00,
	/** Sine function */
	CORDIC_FUNC_SINE = 0x01,
	/** Phase (arctan2) function */
	CORDIC_FUNC_PHASE = 0x02,
	/** Modulus (sqrt(x^2+y^2)) function */
	CORDIC_FUNC_MODULUS = 0x03,
	/** Arctangent function */
	CORDIC_FUNC_ARCTANGENT = 0x04,
	/** Hyperbolic Cosine function */
	CORDIC_FUNC_HCOSINE = 0x05,
	/** Hyperbolic Sine function */
	CORDIC_FUNC_HSINE = 0x06,
	/** Hyperbolic Arctangent function */
	CORDIC_FUNC_HARCTANGENT = 0x07,
	/** Natural Logarithm function */
	CORDIC_FUNC_NATURALLOG = 0x08,
	/** Square Root function */
	CORDIC_FUNC_SQUAREROOT = 0x09,
};

/**
 * @brief CORDIC precision in calculation cycles
 * Values represent the number of iteration cycles (1-15)
 */
enum cordic_precision {
	CORDIC_PRECISION_1 = 1,
	CORDIC_PRECISION_2 = 2,
	CORDIC_PRECISION_3 = 3,
	CORDIC_PRECISION_4 = 4,
	CORDIC_PRECISION_5 = 5,
	CORDIC_PRECISION_6 = 6,
	CORDIC_PRECISION_7 = 7,
	CORDIC_PRECISION_8 = 8,
	CORDIC_PRECISION_9 = 9,
	CORDIC_PRECISION_10 = 10,
	CORDIC_PRECISION_11 = 11,
	CORDIC_PRECISION_12 = 12,
	CORDIC_PRECISION_13 = 13,
	CORDIC_PRECISION_14 = 14,
	CORDIC_PRECISION_15 = 15,
};

/**
 * @brief CORDIC scale factor (number of iterations)
 * Values range from 0 to 7
 */
enum cordic_scale {
	CORDIC_SCALE_VAL_0 = 0,
	CORDIC_SCALE_VAL_1 = 1,
	CORDIC_SCALE_VAL_2 = 2,
	CORDIC_SCALE_VAL_3 = 3,
	CORDIC_SCALE_VAL_4 = 4,
	CORDIC_SCALE_VAL_5 = 5,
	CORDIC_SCALE_VAL_6 = 6,
	CORDIC_SCALE_VAL_7 = 7,
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
	enum cordic_data_width in_size;
	/** Output data width */
	enum cordic_data_width out_size;
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
				      int32_t *in_data,
				      int32_t *out_data,
				      uint32_t num_in,
				      uint32_t num_out);

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
 * - 1 input: in_data[0] contains the input value
 * - 2 inputs: in_data[0] and in_data[1] contain the input values
 *
 * For functions producing:
 * - 1 output: out_data[0] receives the result
 * - 2 outputs: out_data[0] and out_data[1] receive the results
 *
 * @param dev Pointer to the device structure for the CORDIC controller
 * @param in_data Pointer to input data array
 * @param out_data Pointer to output data array
 * @param num_in Number of input values (1 or 2)
 * @param num_out Number of output values (1 or 2)
 *
 * @retval 0 If successful
 * @retval -errno Negative errno code on failure
 */
__syscall int cordic_calculate(const struct device *dev,
			       int32_t *in_data,
			       int32_t *out_data,
			       uint32_t num_in,
			       uint32_t num_out);

static inline int z_impl_cordic_calculate(const struct device *dev,
					  int32_t *in_data,
					  int32_t *out_data,
					  uint32_t num_in,
					  uint32_t num_out)
{
	const struct cordic_driver_api *api =
		(const struct cordic_driver_api *)dev->api;

	if (api->calculate == NULL) {
		return -ENOSYS;
	}

	return api->calculate(dev, in_data, out_data, num_in, num_out);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/cordic.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_CORDIC_H_ */
