/** @file
 * @brief Websocket Server API
 *
 * An API for applications to setup a websocket server connections.
 */
/*
 * Copyright (c) 2022 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_websocket_server, CONFIG_WEBSOCKET_SRV_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <errno.h>

#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/websocket_server.h>

#include "include/websocket_frame.h"
#include "include/websocket_server_tcp.h"
#include "include/websocket_server_internal.h"
#include "include/websocket_server_http_parser.h"

// ToDo: There are shall be multiple instances possible (use code/data separation)!
static websocket_server_hnd_t *ws_hnd_inst;
static websocket_frame_t wf;

int websocket_server_rcv_hnd(const char* buff, const size_t length)
{
	int ret = 0;

	websocket_server_state_t state_next = ws_hnd_inst->state;
	switch (ws_hnd_inst->state) {
	case CONNECTING:
		LOG_DBG("WebSocket Server: CONNECTING");
		ret = websocket_server_http_opening_handshake_parse(
				&ws_hnd_inst->http_parser, buff, length);
		state_next = CONNECTING;
		break;
	case OPEN:
		LOG_DBG("WebSocket Server: OPEN");
		// ToDo: In this state we just communicate either by processing
		// recived frames and responding to them or by processing
		// the input to this API and packing them into websocket frames
		websocket_frame_deserialize(&wf, buff, length);
		break;
	case CLOSING:
		LOG_DBG("WebSocket Server: CLOSING");
		break;
	case CLOSED:
		LOG_DBG("WebSocket Server: CLOSED");
		break;
	default:
		LOG_ERR("ERROR: WebSocket Server: Unknown state!");
		break;
	}
	ws_hnd_inst->state = state_next;

	return ret;
}

int websocket_server_snd_hnd(char* buff, const size_t length, size_t *offset)
{
	int ret = 0;

	websocket_server_state_t state_next = ws_hnd_inst->state;
	switch (ws_hnd_inst->state) {
	case CONNECTING:
		LOG_DBG("WebSocket Server: CONNECTING");
		if(ws_hnd_inst->http_parser.is_fin) {
			ret = websocket_server_http_opening_handshake_response(
					&ws_hnd_inst->http_parser, offset,
					buff, length);
			state_next = OPEN;
		}
		break;
	case OPEN:
		LOG_DBG("WebSocket Server: OPEN");
		wf.header.mask = false;  // the server shall not mask!
		wf.header.fin = true;
		wf.header.opcode = TEXT;
		wf.header.payload_len = 5;
		*offset = length;
		websocket_frame_serialize(wf, buff, offset);
		break;
	case CLOSING:
		LOG_DBG("WebSocket Server: CLOSING");
		break;
	case CLOSED:
		LOG_DBG("WebSocket Server: CLOSED");
		break;
	default:
		LOG_ERR("ERROR: WebSocket Server: Unknown state!");
		break;
	}
	ws_hnd_inst->state = state_next;

	return ret;
}

int websocket_server_init(websocket_server_hnd_t *hnd)
{
	int ret = 0;

	/* ToDo: Use instance register mechanism later
		to operate over multiple instances!
	*/
	ws_hnd_inst = hnd;

	memset(ws_hnd_inst, 0, sizeof(websocket_server_hnd_t));
	ws_hnd_inst->state = CONNECTING;  // see RFC6455. p.14

	ret = websocket_server_http_parser_init(&ws_hnd_inst->http_parser);

	websocket_server_tcp_start();

	return ret;
}