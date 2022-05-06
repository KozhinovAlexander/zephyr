/** @file
 * @brief Websocket API common header
 *
 * Websocket API common header for Server/Client common definitions
 */

/*
 * Copyright (c) 2022 Alexander Kozhinov <ak.alexander.kozhinov@gmail.comcomcomcom>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_WEBSOCKET_COMMON_H_
#define ZEPHYR_INCLUDE_NET_WEBSOCKET_COMMON_H_

/** Message type values. Returned in websocket_recv_msg() */
#define WEBSOCKET_FLAG_FINAL  0x00000001 /**< Final frame        */
#define WEBSOCKET_FLAG_TEXT   0x00000002 /**< Textual data       */
#define WEBSOCKET_FLAG_BINARY 0x00000004 /**< Binary data        */
#define WEBSOCKET_FLAG_CLOSE  0x00000008 /**< Closing connection */
#define WEBSOCKET_FLAG_PING   0x00000010 /**< Ping message       */
#define WEBSOCKET_FLAG_PONG   0x00000020 /**< Pong message       */

enum websocket_opcode  {
	WEBSOCKET_OPCODE_CONTINUE     = 0x00,
	WEBSOCKET_OPCODE_DATA_TEXT    = 0x01,
	WEBSOCKET_OPCODE_DATA_BINARY  = 0x02,
	WEBSOCKET_OPCODE_CLOSE        = 0x08,
	WEBSOCKET_OPCODE_PING         = 0x09,
	WEBSOCKET_OPCODE_PONG         = 0x0A,
};

#endif /* ZEPHYR_INCLUDE_NET_WEBSOCKET_COMMON_H_ */