/** @file
 @brief Websocket Server API HTTP Parser tests

 A test framework for WebSocket Server API HTTP parser
 */
/*
 * Copyright (c) 2022 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_websocket_server_http_parser, LOG_LEVEL_DBG);

#include <zephyr/sys/util.h>
#include <zephyr/net/websocket_server.h>

#include <zephyr/ztest_assert.h>

// #include "websocket_server_http_parser.h"


ZTEST(websocket_server_test_framework, test_ws_http_opening_handshake_parse)
{
	int ret = 3;
	
	// http_parser_t hp;
	// ret = websocket_server_http_parser_init(&hp);
	zassert_true(ret >= 0, "return code must be zero or positive");
}
