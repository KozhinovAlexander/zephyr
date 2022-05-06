/** @file
 @brief Websocket Server API tests

 A test framework for WebSocket Server API
 */
/*
 * Copyright (c) 2022 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_websocket_server, LOG_LEVEL_DBG);

#include <zephyr/ztest.h>

ZTEST_SUITE(websocket_server_test_framework, NULL, NULL, NULL, NULL, NULL);

void test_main(void)
{
	ztest_run_test_suite(websocket_server_test_framework);
}
