/** @file
 @brief Websocket Server TCP connection management

 This is not to be included by the application.
 */
/*
 * Copyright (c) 2022 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_WEBSOCKET_SERVER_TCP_H_
#define ZEPHYR_INCLUDE_NET_WEBSOCKET_SERVER_TCP_H_

#include <zephyr/net/websocket_server.h>

#ifdef __cplusplus
extern "C" {
#endif

void websocket_server_tcp_start(void);
void websocket_server_tcp_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_WEBSOCKET_SERVER_TCP_H_ */
