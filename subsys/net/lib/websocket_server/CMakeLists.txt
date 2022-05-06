#
# Copyright (c) 2022 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library_named(websocket_server)
zephyr_include_directories(${ZEPHYR_BASE}/subsys/net/lib/sockets)

zephyr_library_sources(
	websocket_frame.c
	websocket_server.c
	websocket_server_tcp.c
	websocket_server_http_parser.c
)

zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
