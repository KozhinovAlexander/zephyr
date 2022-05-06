/** @file
 * @brief Websocket Server API
 *
 * An API for applications to setup websocket server connections
 */

/*
 * Copyright (c) 2022 Alexander Kozhinov <ak.alexander.kozhinov@gmail.comcomcomcom>
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_NET_WEBSOCKET_SERVER_H_
#define ZEPHYR_INCLUDE_NET_WEBSOCKET_SERVER_H_

#include <zephyr/kernel.h>

#include "websocket_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WEBSOCKET_SRV_BUFF_DUMMY_LEN		128  // ToDo: use as KConfig parameters later instead!

/* Sec-WebSocket-Key field length in bytes */
#define WEBSOCKET_SRV_SEC_WEBSOCKET_KEY_LEN	24

/* Sec-WebSocket-Accept field length in bytes */
#define WEBSOCKET_SRV_SEC_WEBSOCKET_ACCEPT_LEN	29

/**
 * @brief WebSocket Server state showing one of the following:
 * 	OPEN - the server may begin sending (and receiving) data (initial state, refer to RFC6455, 4.2)
 * 	CLOSING - Upon either sending or receiving a Close control frame, it is said
 * 		that _The WebSocket Closing Handshake is Started_ and that the
 * 		WebSocket connection is in the CLOSING state (RFC6455, 7.1.3).
 * 	CLOSED - When the underlying TCP connection is closed, it is said that
 * 		_The WebSocket Connection is Closed_ and that the WebSocket connection is
 * 		in the CLOSED state. If the TCP connection was closed after the
 * 		WebSocket closing handshake was completed, the WebSocket connection
 * 		is said to have been closed _cleanly_.
 * 		If the WebSocket connection could not be established, it is also said
 * 		that _The WebSocket Connection is Closed_, but not _cleanly_.
 * ToDo: Update description!
 */
typedef enum {
	CONNECTING = 0,
	OPEN = 1,
	CLOSING = 2,
	CLOSED = 3,
} websocket_server_state_t;

/**
 * @brief The WebSocket HTTP parser is used to provide handshake step.
 * 
 */
typedef struct {
	/* Fields presence flags */
	struct {
		uint32_t get : 1;  /* true - if HTTP header is get */
		uint32_t key : 1;  /* true - if Sec-WebSocket-Key is present */
		uint32_t upgrade : 1;  /* true - if upgrade field is websocket */
		uint32_t connection : 1; /* true - if connection field is upgrade */
		uint32_t protocol : 1; /* true - if protocol filed present in header */
		uint32_t extensions : 1; /* true - if protocol filed present in header */
	} fields;

	char resource_name[WEBSOCKET_SRV_BUFF_DUMMY_LEN];
	char host[WEBSOCKET_SRV_BUFF_DUMMY_LEN];
	char origin[WEBSOCKET_SRV_BUFF_DUMMY_LEN];  // optional

	struct {
		uint8_t major;
		uint8_t minor;
	} http_ver;

	struct {
		uint8_t version;
		char accept[WEBSOCKET_SRV_SEC_WEBSOCKET_ACCEPT_LEN];
		char protocol[WEBSOCKET_SRV_BUFF_DUMMY_LEN];  // optional
		char extensions[WEBSOCKET_SRV_BUFF_DUMMY_LEN];  // optional
	} sec_websocket;

	bool is_fin;  /* true - if the last line in the header wer parsed */
} http_parser_t;

/**
 * @brief WebSocket server handler
 */
typedef struct {
	http_parser_t http_parser;
	websocket_server_state_t state;
} websocket_server_hnd_t;

int websocket_server_init(websocket_server_hnd_t *hnd);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_WEBSOCKET_SERVER_H_ */
