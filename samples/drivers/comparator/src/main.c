/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/comparator.h>

#define SLEEP_TIME_MS		500

/* COMP node from device tree */
#define COMP_SENSOR_NODE DT_ALIAS(compsensor)
#if DT_NODE_HAS_STATUS(COMP_SENSOR_NODE, okay)
static const struct device *comp_sens_dev = DEVICE_DT_GET(COMP_SENSOR_NODE);
#else
#error "COMP1 node is disabled"
#endif

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void comp_trigger_cb(const struct device *dev, void *user_data)
{
	const int ret = comparator_get_output(dev);

	printf("- %s: %s output: %d\n", __func__, dev->name, ret);
}

int main(void)
{
	int ret = 0;
	size_t cnt = 0;

	if (!gpio_is_ready_dt(&led)) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = device_is_ready(comp_sens_dev);
	if (!ret) {
		printk("Device not ready: comp1 (err: %d)\n", ret);
		return ret;
	}

	ret = comparator_set_trigger_callback(comp_sens_dev, &comp_trigger_cb, NULL);
	if (ret != 0) {
		printk("Failed to set trigger callback: %s (err: %d)\n",
				comp_sens_dev->name, ret);
		return ret;
	}

	ret = comparator_set_trigger(comp_sens_dev, COMPARATOR_TRIGGER_BOTH_EDGES);
	if (ret != 0) {
		printk("Failed to set trigger: %s (err: %d)\n",
				comp_sens_dev->name, ret);
		return ret;
	}

#ifndef CONFIG_COVERAGE
	while (1) {
#else
	for (int k = 0; k < 10; k++) {
#endif
		k_msleep(SLEEP_TIME_MS);

		if (cnt % 3 == 0) {
			ret = gpio_pin_toggle_dt(&led);
			if (ret < 0) {
				printk("can't toggle led: exit (err: %d)\n", ret);
				return ret;
			}
		}

		ret = comparator_get_output(comp_sens_dev);
		printf("- %s output: %d\n", comp_sens_dev->name, ret);

		cnt++;
	}

	return 0;
}
