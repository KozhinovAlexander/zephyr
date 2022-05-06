/** @file
 @brief WebSocket Server HTTP parser

 This is not to be included by the application.
 */
/*
 * Copyright (c) 2022 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_WEBSOCKET_SERVER_HTTP_PARSER_H_
#define ZEPHYR_INCLUDE_NET_WEBSOCKET_SERVER_HTTP_PARSER_H_

#include <zephyr/kernel.h>

#include <zephyr/net/websocket_server.h>

#ifdef __cplusplus
extern "C" {
#endif


 /* The method of the request MUST be GET, and the HTTP version MUST be at
   least 1.1
   ToDo: use KConfig for it!
*/
#define WEBSOCKET_SRV_HTTP_VERSION_MAJOR	1
#define WEBSOCKET_SRV_HTTP_VERSION_MINOR	1

#define WEBSOCKET_PROTOCOL_VERSION	13  /* see RFC 6455 Section 4.1 (page 18) --> ToDo: Use as KConfig parameter later! */

int websocket_server_http_parser_init(http_parser_t *hp);


/**
 * @brief Parses HTTP opening handshake from client according to section 4.2.1
 * 	of RFC 6455
 * 
 * @param hp - the http parser
 * @param buff - the HTTP request input buffer
 * @param length - the length of the input buffer
 * @return int - the state of the parsing
 */
int websocket_server_http_opening_handshake_parse(http_parser_t *hp,
						  const char *buff,
						  const size_t length);

/**
 * @brief Fills buff with http response information
 * 
 * @param hp - the http parser
 * @param offset - the number of written bytes
 * @param buff - the HTTP response input buffer
 * @param length - the length of the input buffer
 * @return int - the state of the parsing
 */
int websocket_server_http_opening_handshake_response(http_parser_t *hp,
						     size_t *offset,
						     char *buff,
						     const size_t length);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_WEBSOCKET_SERVER_HTTP_PARSER_H_ */