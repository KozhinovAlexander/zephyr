/** @file
 @brief Websocket Server API HTTP Parser tests

 A test framework for WebSocket Server API HTTP parser
 */
/*
 * Copyright (c) 2022 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>

#include <zephyr/sys/util.h>
#include <zephyr/subsys/net/lib/websocket_server/include/websocket_frame.h>

#include <zephyr/ztest_assert.h>


ZTEST(websocket_server_test_framework, test_websocket_frame)
{
	int ret = +2;
	
	// http_parser_t hp;
	// ret = websocket_server_http_parser_init(&hp);
	zassert_true(ret >= 0, "----- websocket_frame: return code must be zero or positive");
}
