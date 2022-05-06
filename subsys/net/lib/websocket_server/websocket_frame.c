/** @file
 * @brief WebSocket Frame
 *
 * Reperesents WebSocet frame / packet.
 */
/*
 * Copyright (c) 2022 Alexander Kozhinov <ak.alexander.kozhinov@gmail.comcom>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_websocket_server_frame, CONFIG_WEBSOCKET_SRV_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <errno.h>

#include <zephyr/sys/util.h>

#include "include/websocket_frame.h"

#define WS_FIN_POS	7
#define WS_RSV1_POS	6

#define WS_RSV2_POS	5

#define WS_RSV3_POS	4

#define WS_OPCODE_POS	0

#define WS_PAYLOAD_POS	0

#define WS_MASK_POS		7
#define WS_MASK_KEY_LEN		4  // masking key length in bytes

#define PAYLOAD_LEN_POS			2

#define  WS_PLD_LEN_7P16		0x7e  // extended payload lenght mask 7+16 bits
#define  WS_PLD_LEN_7P64		0x7f  // extended payload lenght mask 7+64 bits

#define BITS_IN_BYTE			(8*sizeof(uint8_t))

#define WS_UNDEFINED_RSV_ERR	"ERROR: Undefined RSV%d: %d"

static const uint8_t shiftr_n_mask(const uint8_t a, const uint8_t s,
				   const uint8_t m)
{
	return (a >> s) & m;
}

static void print_frame(const char *pref, const websocket_frame_t f)
{
	// ToDo: This function must be deleted later!
	printk("%s\n", pref);
	printk("header:\n");
	printk("\tfin: 0x%x\n", f.header.fin);
	printk("\trsv1: 0x%x\n", f.header.rsv1);
	printk("\trsv2: 0x%x\n", f.header.rsv2);
	printk("\trsv3: 0x%x\n", f.header.rsv3);
	printk("\topcode: 0x%x\n", f.header.opcode);
	printk("\tmask: 0x%x\n", f.header.mask);
	printk("\tpayload_len: 0x%x\n", f.header.payload_len);

	printk("ext_payload_len: %lld\n", f.ext_payload_len);

	printk("payload_data: [ ");
	for(size_t i = 0; i < f.ext_payload_len; i++) {
		printk(" (%c: 0x%x)", f.payload_data[i], f.payload_data[i]);
	}
	printk(" ]\n");
}

/**
 * @brief Transforms the data by applying mask key. It can be used to mask and
 * 	unmask the data.
 * 
 * @param buff - data buffer
 * @param buff_len - buffer length
 * @param mask_key - masking key
 */
void transform_with_mask(uint8_t *buff, const uint64_t buff_len,
			 const uint32_t mask_key)
{
	uint8_t m[4];
	memcpy(m, &mask_key, sizeof(mask_key));
	for(size_t i = 0; i < buff_len; i++) {
		buff[i] = buff[i] ^ m[i % 4];
	}
}

/**
 * @brief Validates opcode type.
 * 
 * @param opcode - the opcode to be validated
 * @return int - returns 0 if opcode is ok or negative number with error code
 */
int opcode_validate(const websocket_frame_opcode_t opcode)
{
	int ret = 0;
	if(opcode > PONG) {
		ret = -opcode;
	}
	return ret;
}

const uint8_t get_fin(const uint8_t byte) {
	return shiftr_n_mask(byte, WS_FIN_POS, BIT_MASK(WS_FIN_POS));
}

const uint8_t get_rsv1(const uint8_t byte) {
	return shiftr_n_mask(byte, WS_RSV1_POS, BIT_MASK(WS_RSV1_POS));
}

const uint8_t get_rsv2(const uint8_t byte) {
	return shiftr_n_mask(byte, WS_RSV2_POS, BIT_MASK(WS_RSV2_POS));
}

const uint8_t get_rsv3(const uint8_t byte) {
	return shiftr_n_mask(byte, WS_RSV3_POS, BIT_MASK(WS_RSV3_POS));
}

const uint8_t get_opcode(const uint8_t byte) {
	return shiftr_n_mask(byte, WS_OPCODE_POS, BIT_MASK(WS_OPCODE_POS));
}

/**
 * @brief Reads header from the buffer
 * 
 * @param buff - data buffer
 * @param h - a pointer to the header to write the data
 * @return int - error code (0 - if ok)
 */
int read_header(const char* buff, websocket_frame_header_t *h) {
	int ret = 0;

	// Read first part of the header:
	h->fin =  get_fin(buff[0]);
	h->rsv1 = get_rsv1(buff[0]);
	h->rsv2 = get_rsv2(buff[0]);
	h->rsv3 = get_rsv3(buff[0]);
	h->opcode = get_opcode(buff[0]);

	// Read second part of the header:
	h->mask = shiftr_n_mask(buff[1], WS_MASK_POS, BIT_MASK(WS_MASK_POS));
	h->payload_len = shiftr_n_mask(buff[1], WS_PAYLOAD_POS,
				       BIT_MASK(WS_PAYLOAD_POS));

	// Defines the interpretation of the "Payload data", thus find it's
	// type:
	ret = opcode_validate(h->opcode);
	if(ret) {
		LOG_ERR("Invalid opcode %d!", h->opcode);
		return ret;
	}

	if(h->rsv1) {
		LOG_ERR(WS_UNDEFINED_RSV_ERR, 1, h->rsv1);
		ret = -1;
		return ret;  // ToDo: Use prorper error code
	}

	if(h->rsv2) {
		LOG_ERR(WS_UNDEFINED_RSV_ERR, 2, h->rsv2);
		ret = -1;
		return ret;  // ToDo: Use prorper error code
	}

	if(h->rsv3) {
		LOG_ERR(WS_UNDEFINED_RSV_ERR, 3, h->rsv3);
		ret = -1;
		return ret;  // ToDo: Use prorper error code
	}

	return ret;
}

/**
 * @brief Writed data from WebSocket frame header to the buffer.
 * 
 * @param buff - data buffer
 * @param h - WebSocket frame header
 */
void write_header(char* buff, const websocket_frame_header_t h) {
	buff[0] = 0;
	WRITE_BIT(buff[0], WS_FIN_POS, h.fin);
	WRITE_BIT(buff[0], WS_RSV1_POS, h.rsv1);
	WRITE_BIT(buff[0], WS_RSV2_POS, h.rsv2);
	WRITE_BIT(buff[0], WS_RSV3_POS, h.rsv3);
	buff[0] |= h.opcode;

	buff[1] = 0;
	WRITE_BIT(buff[1], WS_MASK_POS, h.mask);
	buff[1] |= (uint8_t)h.payload_len;

	// Write extended payload length:
	if(h.payload_len >= WS_PLD_LEN_7P16) {
		// ToDo: set extended payload length!
		// ToDo: the buffer length must be taken into account here!
		// buff[2] = f.ext_payload_len;
		if (h.payload_len == WS_PLD_LEN_7P64) {
		}
	}
}

int websocket_frame_deserialize(websocket_frame_t *f, const char* buff,
				const size_t len)
{
	int ret = 0;

	ret = read_header(buff, &f->header);
	if(ret) {
		LOG_ERR("Broken WebSocket frame header %d!", ret);
		return ret;
	}

	// Determine payload length and masking key position:
	uint8_t mask_key_pos = PAYLOAD_LEN_POS;  // masking key position
	if(f->header.payload_len == WS_PLD_LEN_7P16) {
		for(size_t i = 0; i < sizeof(uint16_t); i++) {
			f->ext_payload_len +=
				((uint16_t)buff[i + PAYLOAD_LEN_POS]) <<
				(BITS_IN_BYTE * sizeof(uint16_t) -
				(i+1) * BITS_IN_BYTE);
		}
		mask_key_pos += sizeof(uint16_t);
	} else if(f->header.payload_len == WS_PLD_LEN_7P64) {
		for(size_t i = 0; i < sizeof(f->ext_payload_len); i++) {
			f->ext_payload_len +=
				((uint64_t)buff[i + PAYLOAD_LEN_POS]) <<
				(BITS_IN_BYTE * sizeof(f->ext_payload_len) -
				(i+1) * BITS_IN_BYTE);
		}
		mask_key_pos += sizeof(f->ext_payload_len);
	} else {
		f->ext_payload_len = f->header.payload_len;
	}

	// Copy masking key to internal variable if present:
	uint32_t mask_key = 0;
	if(f->header.mask) {
		memcpy(&mask_key, buff + mask_key_pos, sizeof(mask_key));
	}

	const size_t mask_key_offset = f->header.mask * (mask_key_pos +
							sizeof(mask_key));
	f->payload_data = (uint8_t *)(buff + mask_key_offset);

	// Unmask the data:
	if (f->header.mask) {
		transform_with_mask(f->payload_data, f->ext_payload_len,
				mask_key);
	}

	return ret;
}

int websocket_frame_serialize(const websocket_frame_t f, char* buff, size_t *len)
{
	int ret = 0;

	// Set buffer length to be transmitted:
	uint8_t ext_payload_bytes = 0;
	if(f.header.payload_len >= WS_PLD_LEN_7P16) {
		ext_payload_bytes += sizeof(uint16_t);
		if (f.header.payload_len == WS_PLD_LEN_7P64) {
			ext_payload_bytes += sizeof(f.ext_payload_len);
		}
	}

#define WS_FRAME_HEADER_LEN	2
	const size_t frame_header_len = WS_FRAME_HEADER_LEN +
					f.header.mask * WS_MASK_KEY_LEN +
					ext_payload_bytes;
	const size_t need_buff_len = frame_header_len + f.ext_payload_len +
				     f.header.payload_len;

	if(need_buff_len > *len) {
		LOG_ERR("Frame length %d does not match buffer length %d!",
			need_buff_len, *len);
		ret = -1;
		return ret;  // ToDo: use correct error code!
	}
	*len = need_buff_len;

	// Write header:
	write_header(buff, f.header);

	// Write payload (consider extended payload length also!):
	memcpy(buff + frame_header_len, f.payload_data, f.header.payload_len);

	LOG_INF("serealize...");
	LOG_HEXDUMP_INF(buff, *len, "buff: ");

	print_frame("websocket_frame_serialize", f);

	return ret;
}
