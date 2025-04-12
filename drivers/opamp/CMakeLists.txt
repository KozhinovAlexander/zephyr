# Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/opamp.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USERSPACE opamp_handlers.c)
zephyr_library_sources_ifdef(CONFIG_OPAMP_FAKE_OPAMP opamp_fake_opamp.c)
zephyr_library_sources_ifdef(CONFIG_OPAMP_SHELL opamp_shell.c)
zephyr_library_sources_ifdef(CONFIG_OPAMP_STM32_OPAMP opamp_stm32_opamp.c)
