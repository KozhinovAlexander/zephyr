/** @file
 @brief WebSocket Server private header

 This is not to be included by the application.
 */
/*
 * Copyright (c) 2022 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_WEBSOCKET_SERVER_INTERNAL_H_
#define ZEPHYR_INCLUDE_NET_WEBSOCKET_SERVER_INTERNAL_H_

#include <zephyr/net/websocket_server.h>

#include "websocket_server_http_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WebSocket server recive data handler
 * 
 * @param buff - recive buffer
 * @param length - buffer length
 * @return int - the execution status
 */
int websocket_server_rcv_hnd(const char* buff, const size_t length);

/**
 * @brief WebSocket server send data handler
 * 
 * @param buff - send buffer
 * @param length - buffer length
 * @param offset - the number of written bytes
 * @return int - the execution status
 */
int websocket_server_snd_hnd(char* buff, const size_t length, size_t *offset);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_WEBSOCKET_SERVER_INTERNAL_H_ */