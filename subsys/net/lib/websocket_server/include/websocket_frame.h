/** @file
 @brief WebSocket Server private header

 This is not to be included by the application.
 */
/*
 * Copyright (c) 2022 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_WEBSOCKET_FRAME_H_
#define ZEPHYR_INCLUDE_NET_WEBSOCKET_FRAME_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Defines websocket frame opcodes (see RFC6455, p. 29)
 * 
 */
typedef enum {
	CONTINUATION = 0x0,
	TEXT = 0x1,
	BINARY = 0x2,
	CLOSE = 0x8,
	PING = 0x9,
	PONG = 0xA,
} websocket_frame_opcode_t;

/**
 * @brief WebSockeet frame header
 * ToDo: put masking key, and extended payload lengths here also!
 * 
 */
typedef struct {
	uint16_t fin : 1;
	uint16_t rsv1 : 1;
	uint16_t rsv2 : 1;
	uint16_t rsv3 : 1;
	websocket_frame_opcode_t opcode : 4;
	uint16_t mask : 1;  // mask bit
	uint16_t payload_len : 7;
} websocket_frame_header_t;

/**
 * @brief Defines websocket frame data structure
 * 
 */
typedef struct {
	websocket_frame_header_t header;
	uint64_t ext_payload_len;
	uint8_t *payload_data;
} websocket_frame_t;

/**
 * @brief Deserealizes WebsocketFrame from byte buffer.
 * 
 * @param f - a pointer to resulting websocket frame
 * @param buff - byte buffer with frame data
 * @param len - pointer to buffer length
 * @return int - negative value with error code / zero otherwise
 */
int websocket_frame_deserialize(websocket_frame_t *f, const char* buff,
				const size_t len);

/**
 * @brief Serealizes WebsocketFrame to byte buffer.
 * 
 * @param f - websocket frame
 * @param buff - byte buffer with for serealized frame data
 * @param len - pointer to buffer length
 * @return int - negative value with error code / zero otherwise
 */
int websocket_frame_serialize(const websocket_frame_t f, char* buff,
				size_t *len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_WEBSOCKET_FRAME_H_ */