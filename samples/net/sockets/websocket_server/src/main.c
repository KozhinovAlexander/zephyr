/*
 * Copyright (c) 2022 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(websocket_server_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <errno.h>

#include <zephyr/net/websocket_server.h>

#include "certificate.h"

static struct k_sem quit_lock;

static websocket_server_hnd_t hnd;

void main(void)
{
	int ret;
	
	k_sem_init(&quit_lock, 0, K_SEM_MAX_LIMIT);

	ret = websocket_server_init(&hnd);

	k_sem_take(&quit_lock, K_FOREVER);

	LOG_DBG("*** EXIT STATUS: %d***", ret);
}
