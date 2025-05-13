# Copyright (c) 2024 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd.cfg")
board_runner_args(pyocd "--target=stm32g431rbtx")
board_runner_args(jlink "--device=STM32G431RB" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
