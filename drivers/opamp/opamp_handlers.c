/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/opamp.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_opamp_enable(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_OPAMP(dev, enable));
	return z_impl_opamp_enable(dev);
}
#include <zephyr/syscalls/opamp_enable_mrsh.c>

static inline int z_vrfy_opamp_disable(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_OPAMP(dev, disable));
	return z_impl_opamp_disable(dev);
}
#include <zephyr/syscalls/opamp_disable_mrsh.c>

static inline int z_vrfy_opamp_configure(const struct device *dev,
	enum opamp_operation_mod mode)
{
K_OOPS(K_SYSCALL_DRIVER_OPAMP(dev, configure));
return z_impl_opamp_configure(dev, mode);
}
#include <zephyr/syscalls/opamp_configure_mrsh.c>
