#
# Copyright (c) 2022 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(websocket_server)

FILE(GLOB app_headers
# 	"$ENV{ZEPHYR_BASE}/subsys/net/lib/websocket_server/include"
)

FILE(GLOB app_sources
	"src/*.c"
	# "$ENV{ZEPHYR_BASE}/subsys/net/lib/websocket_server/*.c"
)

target_include_directories(app PRIVATE ${app_headers})
target_sources(app PRIVATE ${app_sources})
