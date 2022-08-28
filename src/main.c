/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(app);

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

void main(void)
{
	while (1) {
		printk("Tere\n");
		LOG_INF("AAAA");
		LOG_DBG("BBBB");
		LOG_WRN("AAAA");
		LOG_ERR("AAAA");
		k_sleep(K_MSEC(1000));
	}
}