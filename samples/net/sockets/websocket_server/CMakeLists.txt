#
# Copyright (c) 2022 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(websocket_server_sample)

if(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED AND
  (CONFIG_NET_SAMPLE_PSK_HEADER_FILE STREQUAL "dummy_psk.h"))
	add_custom_target(development_psk
		COMMAND ${CMAKE_COMMAND} -E echo "----------------------------------------------------------"
		COMMAND ${CMAKE_COMMAND} -E echo "--- WARNING: Using dummy PSK! Only suitable for        ---"
		COMMAND ${CMAKE_COMMAND} -E echo "--- development. Set NET_SAMPLE_PSK_HEADER_FILE to use ---"
		COMMAND ${CMAKE_COMMAND} -E echo "--- own pre-shared key.                                ---"
		COMMAND ${CMAKE_COMMAND} -E echo "----------------------------------------------------------"
	)
	add_dependencies(app development_psk)
endif()

target_sources(app PRIVATE src/main.c)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

set(certs_path ${CMAKE_CURRENT_SOURCE_DIR}/certificates/)
file(GLOB_RECURSE certs_list ${certs_path}/*.der)
foreach(inc_file ${certs_list})
	get_filename_component(inc_file_name ${inc_file} NAME)
	generate_inc_file_for_target(
		app
		${inc_file}
		${gen_dir}/${inc_file_name}.inc
	)
endforeach()
