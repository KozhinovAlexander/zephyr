/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_OPAMP_H_
#define ZEPHYR_INCLUDE_DRIVERS_OPAMP_H_

/**
 * @brief OpAmp Interface
 * @defgroup opamp_interface OpAmp Interface
 * @since 4.0
 * @version 0.0.1
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/** OpAmp operation mode enumerations */
enum opamp_operation_mode {
	/** No trigger */
	OPAMP_MODE_NON_INVERTING = 0,
	/** Trigger on rising edge of opamp output */
	OPAMP_MODE_INVERTING,
};

/** OpAmp confg */
struct opamp_config {
	int gain;
	enum opamp_operation_mode mode;
};

/** @cond INTERNAL_HIDDEN */

typedef int (*opamp_api_enable)(const struct device *dev);
typedef int (*opamp_api_disable)(const struct device *dev);
typedef int (*opamp_api_configure)(const struct device *dev,
									const struct opamp_config config);

__subsystem struct opamp_driver_api {
	opamp_api_enable enable;
	opamp_api_disable disable;
	opamp_api_configure configure;
};

/** @endcond */

/**
 * @brief Enable OpAmp
 *
 * @param dev OpAmp device
 * @retval -errno code Failure
 */
__syscall int opamp_enable(const struct device *dev);

static inline int z_impl_opamp_enable(const struct device *dev)
{
	return DEVICE_API_GET(opamp, dev)->enable(dev);
}

/**
 * @brief Disable OpAmp
 *
 * @param dev OpAmp device
 * @retval -errno code Failure
 */
__syscall int opamp_disable(const struct device *dev);

static inline int z_impl_opamp_disable(const struct device *dev)
{
	return DEVICE_API_GET(opamp, dev)->disable(dev);
}

/**
 * @brief Configure OpAmp
 *
 * @param dev OpAmp device
 * @param config OpAmp configuration
 * @retval -errno code Failure
 */
__syscall int opamp_configure(const struct device *dev,
							  const struct opamp_config config);

static inline int z_impl_opamp_configure(const struct device *dev,
										 const struct opamp_config config)
{
	return DEVICE_API_GET(opamp, dev)->configure(dev, config);
}

#ifdef __cplusplus
}
#endif

/** @} */

#include <zephyr/syscalls/opamp.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_OPAMP_H_ */
