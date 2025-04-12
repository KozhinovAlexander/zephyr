/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_OPAMP_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_OPAMP_FAKE_H_

#include <zephyr/drivers/opamp.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_FAKE_VALUE_FUNC(int,
			opamp_fake_opamp_enable,
			const struct device *);

DECLARE_FAKE_VALUE_FUNC(int,
			opamp_fake_opamp_disable,
			const struct device *);

DECLARE_FAKE_VALUE_FUNC(int,
			opamp_fake_opamp_configure,
			const struct device *,
			const struct opamp_config);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_OPAMP_FAKE_H_ */
