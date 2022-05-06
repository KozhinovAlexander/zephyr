/** @file
 @brief Websocket Server API

 An API for applications to setup a websocket server connections.
 */
/*
 * Copyright (c) 2022 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_websocket_server_tcp, CONFIG_WEBSOCKET_SRV_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <errno.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>

#include "include/websocket_server_tcp.h"
#include "include/websocket_server_internal.h"

#define PROTOCOL_TYPE_IPv4	0xd4
#define PROTOCOL_TYPE_IPv6	0xd6

/**
 * @brief ToDo: put following construct to KConfig!
 * 
 */
#if IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(CONFIG_NUM_PREEMPT_PRIORITIES - 1)  // ToDo: must be done over KConfig!
#endif

/* ToDo: Use separate Rx/Tx threads with different configurable priority */

#if defined(CONFIG_NET_IPV4)
K_THREAD_STACK_ARRAY_DEFINE(tcp4_handler_stack,
			    CONFIG_WEBSOCKET_SRV_MAX_CLIENTS,
			    CONFIG_WEBSOCKET_SRV_STACK_SIZE);
static struct k_thread tcp4_handler_thread[CONFIG_WEBSOCKET_SRV_MAX_CLIENTS];
static bool tcp4_handler_in_use[CONFIG_WEBSOCKET_SRV_MAX_CLIENTS];  // ToDo: need to be done much simpler

static void tcp4_handler(void);
K_THREAD_DEFINE(tcp4_thread_id, CONFIG_WEBSOCKET_SRV_STACK_SIZE, tcp4_handler,
		NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);  // ToDo: use delay (last param) as KConfig param
#endif

#if defined(CONFIG_NET_IPV6)
K_THREAD_STACK_ARRAY_DEFINE(tcp6_handler_stack,
			    CONFIG_WEBSOCKET_SRV_MAX_CLIENTS,
			    CONFIG_WEBSOCKET_SRV_STACK_SIZE);
static struct k_thread tcp6_handler_thread[CONFIG_WEBSOCKET_SRV_MAX_CLIENTS];
static bool tcp6_handler_in_use[CONFIG_WEBSOCKET_SRV_MAX_CLIENTS];  // ToDo: need to be done much simpler

static void tcp6_handler(void);
K_THREAD_DEFINE(tcp6_thread_id, CONFIG_WEBSOCKET_SRV_STACK_SIZE, tcp6_handler,
		NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);  // ToDo: use delay (last param) as KConfig param
#endif


const char* prot2str(const char p);
static void srv_handler(void *ptr1, void *ptr2, void *ptr3);

/**
 * @brief ToDo: Add description
 * 
 */
struct data_t {
	const char proto;
	int sock;
	atomic_t bytes_received;
	struct {
		int sock;
		char rcv_buffer[CONFIG_WEBSOCKET_SRV_RCV_BUFF_SIZE];  // ToDo: test with smaller buffer sizes!!!
		char snd_buffer[CONFIG_WEBSOCKET_SRV_SND_BUFF_SIZE];  // ToDo: test with smaller buffer sizes!!!
	} accepted[CONFIG_WEBSOCKET_SRV_MAX_CLIENTS];
};

/**
 * @brief ToDo: Add description
 * 
 */
struct configs_t {
	struct data_t ipv4;
	struct data_t ipv6;
};

struct configs_t conf = {
	.ipv4 = {
		.proto = PROTOCOL_TYPE_IPv4,
	},
	.ipv6 = {
		.proto = PROTOCOL_TYPE_IPv6,
	},
};

const char* prot2str(const char p)
{
	switch (p) {
		case PROTOCOL_TYPE_IPv4: return "IPv4"; break;
		case PROTOCOL_TYPE_IPv6: return "IPv6"; break;
		default: return NULL; break;
	}
}

static int get_free_slot(struct data_t *data)
{
	int i;

	for (i = 0; i < CONFIG_WEBSOCKET_SRV_MAX_CLIENTS; i++) {
		if (data->accepted[i].sock < 0) {
			return i;
		}
	}

	return -1;  /* ToDo: must be proper error code! */
}

static ssize_t sendall(int sock, const void *buf, size_t len)
{
	while (len) {
		ssize_t out_len = zsock_send(sock, buf, len, 0);

		if (out_len < 0) {
			return out_len;
		}
		buf = (const char *)buf + out_len;
		len -= out_len;
	}

	return 0;
}

/**
 * @brief Initialize TCP protocol and set listen state in tne end
 * 
 * @param data 
 * @param bind_addr 
 * @param bind_addrlen 
 * @return int
 */
static int tcp_init(struct data_t *data, struct sockaddr *addr,
		    socklen_t addr_len)
{
	int ret;

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	data->sock = zsock_socket(addr->sa_family, SOCK_STREAM,
					IPPROTO_TLS_1_2);
#else
	data->sock = zsock_socket(addr->sa_family, SOCK_STREAM,
					IPPROTO_TCP);
#endif
	if (data->sock < 0) {
		LOG_ERR("Failed to create TCP socket (%s): %d",
			prot2str(data->proto), errno);
		return -errno;
	}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	sec_tag_t sec_tag_list[] = {
		1,  // SERVER_CERTIFICATE_TAG,
#if defined(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
		PSK_TAG,
#endif
	};

	ret = zsock_setsockopt(data->sock, SOL_TLS, TLS_SEC_TAG_LIST,
				sec_tag_list, sizeof(sec_tag_list));
	if (ret < 0) {
		LOG_ERR("Failed to set TCP secure option (%s): %d",
			prot2str(data->proto), errno);
		ret = -errno;
	}
#endif

	ret = zsock_bind(data->sock, addr, addr_len);
	if (ret < 0) {
		LOG_ERR("Failed to bind TCP socket (%s): %d",
			prot2str(data->proto), errno);
		return -errno;
	}

	ret = zsock_listen(data->sock, CONFIG_WEBSOCKET_SRV_LISTEN_QUEUE_LEN);
	if (ret < 0) {
		LOG_ERR("Failed to listen on TCP socket (%s): %d",
			prot2str(data->proto), errno);
		ret = -errno;
	}

	return ret;
}

/**
 * @brief ToDo: Must be used as WebSocket Fail and also renamed!
 * 
 */
void quit(void)
{
	LOG_INF("WebSocket Server quit...");
}

/**
 * @brief ToDo: add description
 * 
 * @param data 
 * @return int 
 */
static int tcp_handler(struct data_t *data)
{
	int client;
	int slot;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	LOG_INF("Waiting for TCP connection on port %d (%s)...",
		CONFIG_WEBSOCKET_SRV_PORT, prot2str(data->proto));

	client = zsock_accept(data->sock, (struct sockaddr *)&client_addr,
			      &client_addr_len);
	if (client < 0) {
		LOG_ERR("%s accept error (%d)", prot2str(data->proto), -errno);
		return 0;
	}

	slot = get_free_slot(data);
	if (slot < 0) {
		LOG_ERR("Cannot accept more connections");
		zsock_close(client);
		return 0;
	}

	data->accepted[slot].sock = client;

	LOG_INF("TCP (%s): Accepted connection", prot2str(data->proto));

#define MAX_NAME_LEN sizeof("tcp6[0]")

#if defined(CONFIG_NET_IPV6)
	if (client_addr.sin_family == AF_INET6) {
		tcp6_handler_in_use[slot] = true;

		k_thread_create(
			&tcp6_handler_thread[slot],
			tcp6_handler_stack[slot],
			K_THREAD_STACK_SIZEOF(tcp6_handler_stack[slot]),
			(k_thread_entry_t)srv_handler,
			INT_TO_POINTER(slot), data, &tcp6_handler_in_use[slot],
			THREAD_PRIORITY, 0, K_NO_WAIT);

		if (IS_ENABLED(CONFIG_THREAD_NAME)) {
			char name[MAX_NAME_LEN];
			snprintk(name, sizeof(name), "tcp6[%d]", slot);
			k_thread_name_set(&tcp6_handler_thread[slot], name);
		}
	}
#endif

#if defined(CONFIG_NET_IPV4)
	if (client_addr.sin_family == AF_INET) {
		tcp4_handler_in_use[slot] = true;

		k_thread_create(
			&tcp4_handler_thread[slot],
			tcp4_handler_stack[slot],
			K_THREAD_STACK_SIZEOF(tcp4_handler_stack[slot]),
			(k_thread_entry_t)srv_handler,
			INT_TO_POINTER(slot), data, &tcp4_handler_in_use[slot],
			THREAD_PRIORITY, 0, K_NO_WAIT);

		if (IS_ENABLED(CONFIG_THREAD_NAME)) {
			char name[MAX_NAME_LEN];
			snprintk(name, sizeof(name), "tcp4[%d]", slot);
			k_thread_name_set(&tcp4_handler_thread[slot], name);
		}
	}
#endif

	return 0;
}

#if defined(CONFIG_NET_IPV4)
static void tcp4_handler(void)
{
	int ret;
	struct sockaddr_in addr4;

	(void)memset(&addr4, 0, sizeof(addr4));
	addr4.sin_family = AF_INET;
	addr4.sin_port = htons(CONFIG_WEBSOCKET_SRV_PORT);

	ret = tcp_init(&conf.ipv4, (struct sockaddr *)&addr4, sizeof(addr4));
	if (ret < 0) {
		quit();
		return;
	}

	while (ret == 0) {
		ret = tcp_handler(&conf.ipv4);
		if (ret < 0) {
			break;
		}
	}

	quit();
}
#endif

#if defined(CONFIG_NET_IPV6)
static void tcp6_handler(void)
{
	int ret;
	struct sockaddr_in6 addr6;

	(void)memset(&addr6, 0, sizeof(addr6));
	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(CONFIG_WEBSOCKET_SRV_PORT);

	ret = tcp_init(&conf.ipv6, (struct sockaddr *)&addr6, sizeof(addr6));
	if (ret < 0) {
		quit();
		return;
	}

	while (ret == 0) {
		ret = tcp_handler(&conf.ipv6);
		if (ret != 0) {
			break;
		}
	}

	// ToDo: use above defined socket for a proper quit called from stop_tcp() function
	quit();
}
#endif

static void srv_handler(void *ptr1, void *ptr2, void *ptr3)
{
	int slot = POINTER_TO_INT(ptr1);
	struct data_t *data = ptr2;
	bool *in_use = ptr3;
	int offset = 0;
	int received;
	int client;
	int ret;

	client = data->accepted[slot].sock;
	do {
		// ToDo: handle recv if not full buffer will be recived!
		received = zsock_recv(client,
				data->accepted[slot].rcv_buffer + offset,
				sizeof(data->accepted[slot].rcv_buffer) - offset,
				0);

		LOG_INF("received: %i", received);
		if (received == 0) {
			/* Connection closed */
			// LOG_INF("TCP (%s): Connection closed", prot2str(data->proto));
			ret = 0;
			// continue;
		} else if (received < 0) {
			/* Socket error */
			LOG_ERR("TCP (%s): Connection error %d",
				prot2str(data->proto), errno);
			ret = -errno;
			break;
		} else {
			atomic_add(&data->bytes_received, received);

			ret = websocket_server_rcv_hnd(
				data->accepted[slot].rcv_buffer + offset,
				received);

			if (ret < 0) {
				LOG_ERR("TCP (%s): WebSocket rcv handler failed,"
					" closing socket", prot2str(data->proto));
				ret = 0;
				break;
			}
		}

		offset += received;

#if !defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
		// To prevent fragmentation of the response, reply only if
		// buffer is full or there is no more data to read
		if (/*offset == sizeof(data->accepted[slot].rcv_buffer) || */
		    (zsock_recv(client,
			  data->accepted[slot].rcv_buffer + offset,
			  sizeof(data->accepted[slot].rcv_buffer) - offset,
			  ZSOCK_MSG_PEEK | ZSOCK_MSG_DONTWAIT) < 0 &&
		     (errno == EAGAIN || errno == EWOULDBLOCK))
		   ) {
#endif
			ret = websocket_server_snd_hnd(
				data->accepted[slot].snd_buffer,
				ARRAY_SIZE(data->accepted[slot].snd_buffer),
				&offset);
			if (ret < 0) {
				LOG_ERR("TCP (%s): WebSocket snd handler failed,"
					" closing socket", prot2str(data->proto));
				ret = 0;
				break;
			}

			ret = sendall(client, data->accepted[slot].snd_buffer,
				      offset);
			if (ret < 0) {
				LOG_ERR("TCP (%s): Failed to send, closing "
					"socket", prot2str(data->proto));
				ret = 0;
				break;
			}

			// LOG_HEXDUMP_INF(data->accepted[slot].snd_buffer,
			// 		offset, "snd_buffer:");

			offset = 0;
#if !defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
		}
#endif
	} while (true);

	LOG_ERR("ERROR: Closed ---> must not end up here!");
	// *in_use = false;
	// (void)zsock_close(client);  // ToDo: handle error code properly
	// data->accepted[slot].sock = -1;
}

void websocket_server_tcp_start(void)
{
	int i;

	for (i = 0; i < CONFIG_WEBSOCKET_SRV_MAX_CLIENTS; i++) {
		conf.ipv6.accepted[i].sock = -1;
		conf.ipv4.accepted[i].sock = -1;

#if defined(CONFIG_NET_IPV4)
		tcp4_handler_in_use[i] = false;
#endif

#if defined(CONFIG_NET_IPV6)
		tcp6_handler_in_use[i] = false;
#endif
	}

#if defined(CONFIG_NET_IPV6)
	k_thread_start(tcp6_thread_id);
#endif

#if defined(CONFIG_NET_IPV4)
	k_thread_start(tcp4_thread_id);
#endif
}

void websocket_server_tcp_stop(void)
{
	int i;

	/* Not very graceful way to close a thread, but as we may be blocked
	 * in accept or recv call it seems to be necessary
	 */
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		k_thread_abort(tcp6_thread_id);  // ToDo: use proper thread closing
		if (conf.ipv6.sock >= 0) {
			(void)zsock_close(conf.ipv6.sock);  // ToDo: do not ignore return value!
		}

		for (i = 0; i < CONFIG_WEBSOCKET_SRV_MAX_CLIENTS; i++) {
#if defined(CONFIG_NET_IPV6)
			if (tcp6_handler_in_use[i] == true) {
				k_thread_abort(&tcp6_handler_thread[i]);  // ToDo: use proper thread closing
			}
#endif
			if (conf.ipv6.accepted[i].sock >= 0) {
				(void)zsock_close(conf.ipv6.accepted[i].sock);
			}
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		k_thread_abort(tcp4_thread_id);  // ToDo: use proper thread closing
		if (conf.ipv4.sock >= 0) {
			(void)zsock_close(conf.ipv4.sock);
		}

		for (i = 0; i < CONFIG_WEBSOCKET_SRV_MAX_CLIENTS; i++) {
#if defined(CONFIG_NET_IPV4)
			if (tcp4_handler_in_use[i] == true) {
				k_thread_abort(&tcp4_handler_thread[i]);  // ToDo: use proper thread closing
			}
#endif
			if (conf.ipv4.accepted[i].sock >= 0) {
				(void)zsock_close(conf.ipv4.accepted[i].sock);
			}
		}
	}
}