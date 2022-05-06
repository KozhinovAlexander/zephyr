/** @file
 @brief WebSocket Server HTTP parser
 */
/*
 * Copyright (c) 2022 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_websocket_server_http_parser,
		    CONFIG_WEBSOCKET_SRV_LOG_LEVEL);

#include <zephyr/kernel.h>

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <errno.h>

#include <zephyr/net/net_ip.h>

#include <mbedtls/sha1.h>
#include <zephyr/sys/base64.h>

#include "include/websocket_server_http_parser.h"

/* Globally Unique Identifier (GUID, [RFC4122]): */
#define WEBSOCKET_SRV_GUID	"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

#define SHA1_LEN	20  // SHA-1 length in bytes

/**
 * @brief Defines HTTP field entry
 * 
 */
#define HTTP_FIELD(type, name, parser_cb)	{type, name, sizeof(name), \
						 parser_cb}

/**
 * @brief Empty HTTP field
 * 
 */
#define HTTP_FIELD_EMPTY(type)			{type, NULL, 0, NULL}

#define HTTP_PARSER_CB(name)	static int name(http_parser_t*, const void*, \
						const size_t)

#define CRLF	((uint16_t)0x0d0a)  // CRLF ascii sequence

/* HTTP header field strings definition */
#define HTTP_STR			"HTTP"
#define WEBSOCKET_STR			"websocket"

#define GET_STR				"GET "
#define HOST_STR			"host: "
#define ORIGIN_STR			"origin: "
#define UPGRADE_STR			"upgrade: "
#define CONNECTION_STR			"connection: "
#define SEC_WEBSOCKET_KEY_STR		"sec-websocket-key: "
#define SEC_WEBSOCKET_VERSION_STR	"sec-websocket-version: "
#define SEC_WEBSOCKET_PROTOCOL_STR	"sec-websocket-protocol: "
#define SEC_WEBSOCKET_EXTENSIONS_STR	"sec-websocket-extensions: "
#define SEC_WEBSOCKET_ACCEPT_STR	"sec-webSocket-accept: "

#define HTTP_SWITCHING_PROTOCOLS_STR	"101 Switching Protocols"

typedef enum {
	GET = 0,
	host,
	origin,
	upgrade,
	connection,
	sec_websocket_key,
	sec_websocket_version,
	sec_websocket_protocol,
	sec_websocket_extensions,
	empty,
} http_field_type_t;

/**
 * @brief HTTP field parser callback
 * 
 * @param http_parser_t* - pointer to HTTP parser
 * @param const void* - pointer to data buffer
 * @param size_t - total length of buffer in bytes
 * @return the index of first occurence of the CRLF sequence in buff.
 * 	It returns total length if the sequence were not found.
 */
typedef int (*http_field_parser_cb)(http_parser_t*, const void*,
				    const size_t);

/**
 * @brief HTTP field type showing position and name in the header
 * 	type - HTTP header type
 * 	name - ASCII name of the field
 */
typedef struct {
	http_field_type_t type;
	char *name;
	const size_t name_length;
	http_field_parser_cb parser_cb;
} http_field_t;

HTTP_PARSER_CB(get_parse);
HTTP_PARSER_CB(host_parse);
HTTP_PARSER_CB(origin_parse);
HTTP_PARSER_CB(upgrade_parse);
HTTP_PARSER_CB(connection_parse);
HTTP_PARSER_CB(sec_websocket_key_parse);
HTTP_PARSER_CB(sec_websocket_version_parse);
HTTP_PARSER_CB(sec_websocket_protocol_parse);
HTTP_PARSER_CB(sec_websocket_extensions_parse);

const http_field_t http_fields[] = {
	HTTP_FIELD(GET, GET_STR, &get_parse),
	HTTP_FIELD(host, HOST_STR, &host_parse),
	HTTP_FIELD(origin, ORIGIN_STR, &origin_parse),
	HTTP_FIELD(upgrade, UPGRADE_STR, &upgrade_parse),
	HTTP_FIELD(connection, CONNECTION_STR, &connection_parse),
	HTTP_FIELD(sec_websocket_key, SEC_WEBSOCKET_KEY_STR,
		&sec_websocket_key_parse),
	HTTP_FIELD(sec_websocket_version, SEC_WEBSOCKET_VERSION_STR,
		&sec_websocket_version_parse),
	HTTP_FIELD(sec_websocket_protocol, SEC_WEBSOCKET_PROTOCOL_STR,
		&sec_websocket_protocol_parse),
	HTTP_FIELD(sec_websocket_extensions, SEC_WEBSOCKET_EXTENSIONS_STR,
		&sec_websocket_extensions_parse),
	HTTP_FIELD_EMPTY(empty),  /* must be always last one */
};


/**
 * @brief Searches for the index of the line end as CRLF sequence
 * 
 * @param buff - data buffer
 * @param len - total length of buffer in bytes
 * @return the index of first occurence of the CRLF sequence in buff.
 * 	It returns total length if the sequence were not found.
 */
static size_t header_line_length(const void *buff, const size_t len)
{
	if(len < sizeof(CRLF) || !buff) {
		/* The line lenght is smaller than CRLF (2 bytes) or
		the index exceeds the length or the buffer is invalid */
		LOG_ERR("No buffer given or wrong buffer length found!");
		return len == 0 ? len : len - 1;
	}

	uint16_t ln_end_seq = 0;
	size_t i = 0;
	const char* ch_buff = (char*)buff;
	do {
		ln_end_seq = ((uint16_t*)(ch_buff + i))[0];
		ln_end_seq = sys_cpu_to_be16(ln_end_seq);
		if(ln_end_seq - CRLF == 0) {
			i += sizeof(CRLF);
			break;
		}
	} while(++i < len - 1);

	return i;
}

/**
 * @brief Searches in the first characters for the tape of HTTP field beginning
 * 	from the line end.
 * 
 * @param buff - buffer to search in
 * @param len - total length of the buffer
 * @return http_field_type_t - the HTTP header field type
 */
static http_field_type_t field_type(const void *buff, const size_t len)
{
	http_field_type_t field_type = empty;
	const char* ch_buff = (char*)buff;

	size_t i = 0;
	do {
		if(!strncasecmp(http_fields[i].name, ch_buff,
				http_fields[i].name_length - 1)) {
			field_type = http_fields[i].type;
			break;
		}
		i++;
	} while(http_fields[i].name);

	return field_type;
}

/**
 * @brief Parse the line with GET method.
 * 
 * @param hp - pointer to HTTP parser
 * @param buff - buffer to search in
 * @param len - total length of the buffer
 * @return int - negative error code or zero if ok.
 */
static int get_parse(http_parser_t *hp, const void *buff, const size_t len)
{
	int ret = 0;

	size_t idx1 = sizeof(GET_STR) - 2;
	size_t idx2 = len - 1;

	if(hp->fields.get) {
		/* The GET request were already parsed */
		return -1;
	}

	const char* ch_buff = (char*)buff;
	do {
		/* Search for HTTP substring */
		if(!strncasecmp(HTTP_STR, ch_buff + idx2,
				sizeof(HTTP_STR) - 1)) {
			break;
		}
	} while(--idx2 > idx1);

	if(idx2 < idx1) {
		/* No HTTP substring found */
		ret = -1;  /* ToDo: use proper return code */
		return ret;
	}

	/* Read resource name */
	size_t sz = MIN(ARRAY_SIZE(hp->resource_name),(idx2 - idx1 - 2));
	memcpy(hp->resource_name, ch_buff + idx1 + 1, sz);

	/* Read HTTP version */
	idx1 = idx2 + sizeof(HTTP_STR);
	idx2 = len - sizeof(CRLF);
	size_t idx_dot = idx1;  // index of the dot
	do {
		/* Search for '.' charachter */
		if(ch_buff[idx_dot] == '.') {
			char arr[4] = {0};

			sz = MIN(ARRAY_SIZE(arr),(idx_dot - idx1));
			memcpy(arr, ch_buff + idx1, sz);
			hp->http_ver.major = atoi(arr);

			sz = MIN(ARRAY_SIZE(arr),(idx2 - idx_dot));
			memcpy(arr, ch_buff + idx_dot + 1, sz - 1);
			hp->http_ver.minor = atoi(arr);

			break;
		}
	} while(++idx_dot < idx2);

	if(hp->http_ver.major < WEBSOCKET_SRV_HTTP_VERSION_MAJOR &&
	   hp->http_ver.minor < WEBSOCKET_SRV_HTTP_VERSION_MINOR) {
		return -1;
	}

	/* Set GET request presence flag */
	hp->fields.get = true;

	return ret;
}

static int host_parse(http_parser_t *hp, const void *buff, const size_t len)
{
	const char *ch_buff = (void*)buff;
	const size_t idx1 = sizeof(HOST_STR) - 1;
	const size_t idx2 = len - sizeof(CRLF);

	/* Read host */
	const size_t sz = MIN(ARRAY_SIZE(hp->host),(idx2 - idx1));
	memcpy(hp->host, ch_buff + idx1, sz);

	/* ToDo: The request MUST contain a |Host| header field whose value
	   contains /host/ plus optionally ":" followed by /port/
	   (when not using the default port). (RFC 6455, p.17) */

	return 0;
}

static int origin_parse(http_parser_t *hp, const void *buff, const size_t len)
{
	const char *ch_buff = (void*)buff;
	const size_t idx1 = sizeof(ORIGIN_STR) - 1;
	const size_t idx2 = len - sizeof(CRLF);

	/* Read origin */
	const size_t sz = MIN(ARRAY_SIZE(hp->origin),(idx2 - idx1));
	memcpy(hp->origin, ch_buff + idx1, sz);

	return 0;
}

static int upgrade_parse(http_parser_t *hp, const void *buff, const size_t len)
{
	const char *ch_buff = (void*)buff;
	const size_t idx1 = sizeof(UPGRADE_STR) - 1;
	const size_t idx2 = len - sizeof(CRLF);

	if((idx2 - idx1) < (sizeof(WEBSOCKET_STR) - 1)) {
		/* Expected string length is too big */
		return -1;  /* ToDo: use proper return code */
	}

	/* Read upgrade */
	hp->fields.upgrade = !strncasecmp(WEBSOCKET_STR, ch_buff + idx1,
					  sizeof(WEBSOCKET_STR) - 1);

	if(!hp->fields.upgrade) {
		/* The value of "upgrade: " field is not "websocket"
		   (case-insensitive) */
		return -1;  /* ToDo: use proper return code */
	}

	return 0;
}

static int connection_parse(http_parser_t *hp, const void *buff,
			    const size_t len)
{
	const char *ch_buff = (void*)buff;
	const size_t idx1 = sizeof(CONNECTION_STR) - 1;
	const size_t idx2 = len - sizeof(CRLF);

	/* Use part of UPGRADE_STR that contains the word only (-3 chars) */
	if((idx2 - idx1) < (sizeof(UPGRADE_STR) - 3)) {
		/* Expected string length is too big */
		return -1;  /* ToDo: use proper return code */
	}

	/* Read connection */
	hp->fields.connection = !strncasecmp(UPGRADE_STR, ch_buff + idx1,
					     sizeof(UPGRADE_STR) - 3);

	if(!hp->fields.connection) {
		/* The value of "connection: " field is not "upgrade"
		   (case-insensitive) */
		return -1;  /* ToDo: use proper return code */
	}

	return 0;
}

static int sec_websocket_key_parse(http_parser_t *hp, const void *buff,
				   const size_t len)
{
	int ret = 0;
	char *sec_websocket_key = (void*)buff;
	const size_t idx1 = sizeof(SEC_WEBSOCKET_KEY_STR) - 1;
	const size_t idx2 = len - sizeof(CRLF);

	if(hp->fields.key) {
		/* The |Sec-WebSocket-Key| header field MUST NOT appear more
		   than once in an HTTP request.
		   (RFC 6455, Section 11.3.1.  Sec-WebSocket-Key) */
		return -1;  /* ToDo: use proper return code */
	}

	if((idx2 - idx1) != WEBSOCKET_SRV_SEC_WEBSOCKET_KEY_LEN) {
		/* The size must match expected one */
		return -1;  /* ToDo: use proper return code */
	}

	sec_websocket_key += idx1;

	/* Calculate accept key: */
	char buff_str[WEBSOCKET_SRV_SEC_WEBSOCKET_KEY_LEN +
		sizeof(WEBSOCKET_SRV_GUID) - 1] = { 0 };

	memcpy(buff_str, sec_websocket_key, WEBSOCKET_SRV_SEC_WEBSOCKET_KEY_LEN);
	memcpy(buff_str + WEBSOCKET_SRV_SEC_WEBSOCKET_KEY_LEN, WEBSOCKET_SRV_GUID,
		sizeof(WEBSOCKET_SRV_GUID) - 1);

	ret = mbedtls_sha1(buff_str, ARRAY_SIZE(buff_str), buff_str);
	if(ret) {
		LOG_ERR("ERROR: SHA1 failed with code: %d!", ret);
		return ret;
	}

	size_t olen = 0;
	ret = base64_encode((uint8_t *)hp->sec_websocket.accept,
			ARRAY_SIZE(hp->sec_websocket.accept), &olen,
			(uint8_t *)buff_str,SHA1_LEN);
	if(ret) {
		LOG_ERR("ERROR:base64 encode failed with code: %d! "
			"Need at least %d bytes buffer!", ret, olen);
		return ret;
	}

	hp->fields.key = true;

	return 0;
}

static int sec_websocket_version_parse(http_parser_t *hp, const void *buff,
					const size_t len)
{
	const char *ch_buff = (void*)buff;
	const size_t idx1 = sizeof(SEC_WEBSOCKET_VERSION_STR) - 1;
	const size_t idx2 = len - sizeof(CRLF);

	/* Read version */
	char arr[8] = {0};
	const size_t sz = MIN(ARRAY_SIZE(arr),(idx2 - idx1));
	memcpy(arr, ch_buff + idx1, sz);
	hp->sec_websocket.version = atoi(arr);

	if(hp->sec_websocket.version != WEBSOCKET_PROTOCOL_VERSION) {
		/* The version 13 MUST be used
		   (RFC 6455, Section 4.1 on page 18) */
		return -1;  /* ToDo: use proper return code */
	}

	return 0;
}

static int sec_websocket_protocol_parse(http_parser_t *hp, const void *buff,
					 const size_t len)
{
	int ret = 0;
	return ret;
}

static int sec_websocket_extensions_parse(http_parser_t *hp, const void *buff,
					   const size_t len)
{
	int ret = 0;
	return ret;
}

/**
 * @brief Parse the line of HTTP header depending on the field type.
 * 
 * @param hp - pointer to HTTP parser
 * @param ft - expected field type
 * @param buff - buffer with line characters
 * @param len - length of the buffer
 * @return int - parsing status: negative value with error code if fails
 */
static inline int line_parse(http_parser_t *hp, const http_field_type_t ft,
			     const void *buff, const size_t len)
{
	int ret = 0;

	if(!hp) {
		LOG_ERR("Wrong HTTP parser!");
		return -1;  /* ToDo: use proper error code */
	}

	if(!buff) {
		LOG_ERR("Wrong data buffer!");
		return -1;  /* ToDo: use proper error code */
	}

	if(http_fields[ft].parser_cb) {
		ret = http_fields[ft].parser_cb(hp, buff, len);
	}

	return ret;
}

int websocket_server_http_opening_handshake_parse(http_parser_t *hp,
						  const char *buff,
						  const size_t length)
{
	int ret = 0;

	size_t i = 0;
	size_t ln_len = 0;  // line length
	size_t ln_cnt = 0;  // lines counter
	http_field_type_t ft = empty;
	do {
		/* Find line end index */
		ln_len = header_line_length(buff + i, length - i);

		/* Define the field type */
		ft = field_type(buff + i, ln_len);

		/* Parse the line */
		ret = line_parse(hp, ft, buff + i, ln_len);

		/* Bad field check */
		if(ret < 0) {
			LOG_ERR("Bad HTTP field detected!");
			return ret;
		}

		/* Point to the next line */
		i += ln_len;
		ln_cnt++;

		/* Check final sequence */
		if(ln_len == sizeof(CRLF)) {
			hp->is_fin = true;
			break;
		}
	} while(i < length - 1);

	return ret;
}

/**
 * @brief Writes CRLF into dst beginning from the offset
 * 
 * @param dst - the buffer to write into
 * @param offset - the offset
 */
void write_line_end(char *dst, size_t *offset)
{
	const uint16_t crlf = sys_be16_to_cpu(CRLF);
	memcpy(dst + *offset, &crlf, sizeof(CRLF));
	*offset += sizeof(CRLF);
}

/**
 * @brief Writes a line given in src into dst beginnig from offset. Adds CRLF 
 * 	at the end. The offset will be changed accordingly to the number of 
 * 	written symbols.
 * 
 * @param dst - destination buffer
 * @param offset - offset of dst buffer
 * @param src - source buffer (NULL may be used to write an empty line)
 * @param len - number of bytes of source buffer to write
 */
void write_line(char *dst, size_t *offset, const char *src, const size_t len)
{
	if(src) {
		memcpy(dst + *offset, (void*)src, len);
		*offset += len;
	}
	write_line_end(dst, offset);
}

/**
 * @brief Writes a buffer given in src into dst beginnig from offset.
 * 	The offset will be changed accordingly to the number of written symbols.
 * 
 * @param dst - destination buffer
 * @param offset - offset of dst buffer
 * @param src - source buffer
 * @param len - number of bytes of source buffer to write
 */
void write_buff(char *dst, size_t *offset, const char *src, const size_t len)
{
	memcpy(dst + *offset, (void*)src, len);
	*offset += len;
}

int websocket_server_http_opening_handshake_response(http_parser_t *hp,
						     size_t *offset,
						     char *buff,
						     const size_t length)
{
	int ret = 0;

	// Clean the buffer first:
	*offset = 0;
	memset(buff, 0, length);

	// Put "HTTP/x.x 101 Sitching Protocols" into the buffer:
	write_buff(buff, offset, HTTP_STR, sizeof(HTTP_STR)-1);
	ret = snprintf(buff + *offset, length - *offset, "/%d.%d ",
		hp->http_ver.major, hp->http_ver.minor);
	if(ret < 0) {
		LOG_ERR("ERROR: Buffer write failed with code: %d!", ret);
		return ret;
	}
	*offset += 5;

	write_line(buff, offset, HTTP_SWITCHING_PROTOCOLS_STR,
		sizeof(HTTP_SWITCHING_PROTOCOLS_STR)-1);

	write_buff(buff, offset, UPGRADE_STR, sizeof(UPGRADE_STR)-1);
	write_line(buff, offset, WEBSOCKET_STR, sizeof(WEBSOCKET_STR)-1);

	write_buff(buff, offset, CONNECTION_STR, sizeof(CONNECTION_STR)-1);
	write_line(buff, offset, UPGRADE_STR, sizeof(UPGRADE_STR)-3);
	
	// Put sec-websocket-accept intho the header:
	write_buff(buff, offset, SEC_WEBSOCKET_ACCEPT_STR,
		sizeof(SEC_WEBSOCKET_ACCEPT_STR)-1);
	write_line(buff, offset, hp->sec_websocket.accept,
		ARRAY_SIZE(hp->sec_websocket.accept)-1);

	// ToDo: Put optional headers in the buffer:

	// An empty line must close the header:
	write_line(buff, offset, NULL, 0);

	return ret;
}

/**
 * @brief Just reset the parser by setting all it's fileds to zero!
 * 
 * @param hp 
 */
void reset_parser(http_parser_t *hp)
{
	memset(hp, 0, sizeof(http_parser_t));
}

int websocket_server_http_parser_init(http_parser_t *hp)
{
	int ret = 0;
	reset_parser(hp);
	return ret;
}
