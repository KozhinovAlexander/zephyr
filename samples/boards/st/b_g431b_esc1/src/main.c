/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/comparator.h>

#define SLEEP_TIME_MS		2500
#define SEQUENCE_SAMPLES	3

#define THREAD_ADC_STACK_SIZE	1024
#define THREAD_ADC_PRIORITY		7

#define THREAD_SENSOR_STACK_SIZE	1024
#define THREAD_SENSOR_PRIORITY		8

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

/* ADC nodes from the devicetree. */
#define ADC_1_NODE DT_ALIAS(adc1)
#if DT_NODE_HAS_STATUS(ADC_1_NODE, okay)
static const struct device *adc1_dev = DEVICE_DT_GET(ADC_1_NODE);
#else
#error "ADC1 node is disabled"
#endif

#define ADC_2_NODE DT_ALIAS(adc2)
#if DT_NODE_HAS_STATUS(ADC_2_NODE, okay)
static const struct device *adc2_dev = DEVICE_DT_GET(ADC_2_NODE);
#else
#error "ADC2 node is disabled"
#endif

/* COMP node from device tree */
#define COMP_1_NODE DT_ALIAS(comp1)
#if DT_NODE_HAS_STATUS(COMP_1_NODE, okay)
static const struct device *comp1_dev = DEVICE_DT_GET(COMP_1_NODE);
#else
#error "COMP1 node is disabled"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)
};

#define VOLT_SENSOR_ALIAS(i) DT_ALIAS(_CONCAT(sensor, i))
#define VOLT_SENSOR(i, _)                                                      \
	IF_ENABLED(DT_NODE_EXISTS(VOLT_SENSOR_ALIAS(i)), (DEVICE_DT_GET(VOLT_SENSOR_ALIAS(i)),))

/* support up to 4 voltage sensors */
static const struct device *const sensors[] = {LISTIFY(4, VOLT_SENSOR, ())};

/**
 * Convert ADC raw value to temperature
 *
 * Conversion equation:
 * T = (1/m)*(V - V0) + T0
 *
 * V0 = 1400mV - a voltage @ T0
 * T0 = 25 °C
 * m = 19 mV/°C - slope
 * T - temperature to be calculated
 * V - ADC raw voltage
 *
 * @param adc_val - a measured ADC raw value
 * @return a floating value in celsius
 */
static float adc_raw_val2temp(uint16_t adc_val)
{
	const float slope_inverse = 0.05263158;  /* °C/mV (corresponds to 19 mV/°C) */
	const float T0 = 25;  /* °C */
	const uint16_t V0 = 1400;  /* mV */
	return (slope_inverse * ((float)(adc_val - V0))) + T0;
}

/* thread_adc_measuremet is a static thread that is spawned automatically */
int thread_adc_measuremet(void *, void *, void *)
{
	int err = 0;
	uint32_t cnt = 0;
	uint16_t channel_reading[SEQUENCE_SAMPLES][ARRAY_SIZE(adc_channels)];

	/* Options for the sequence sampling. */
	const struct adc_sequence_options options = {
		.extra_samplings = SEQUENCE_SAMPLES - 1,
		.interval_us = 0,
	};

	/* Configure the sampling sequence to be made. */
	struct adc_sequence sequence = {
		.buffer = channel_reading,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(channel_reading),
		.options = &options,
		.calibrate = true,
	};

	/* Configure channels individually prior to sampling. */
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		err = adc_is_ready_dt(&adc_channels[i]);
		if (!err) {
			printf("ADC controller device %s not ready\n",
				adc_channels[i].dev->name);
			return err;
		}

		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			printf("Could not setup channel #%d (%d)\n",
				adc_channels[i].channel_cfg.channel_id, err);
			return err;
		}

		sequence.channels |= BIT(adc_channels[i].channel_cfg.channel_id);
		sequence.resolution = adc_channels[i].resolution;  /* shall be same for all */
	}


#ifndef CONFIG_COVERAGE
	while (1) {
#else
	for (int k = 0; k < 10; k++) {
#endif
		k_msleep(SLEEP_TIME_MS);

		err = adc_read(adc1_dev, &sequence);
		if (err < 0) {
			printf("Could not read (%d)\n", err);
			continue;
		}

		printf("ADC sequence reading [%u]:\n", cnt++);
		for (size_t ch_idx = 0U; ch_idx < ARRAY_SIZE(adc_channels); ch_idx++) {
			printf("- %s, channel %d, %d sequence samples:\n",
				adc_channels[ch_idx].dev->name,
				adc_channels[ch_idx].channel_cfg.channel_id,
				SEQUENCE_SAMPLES);

			int32_t val_mv = 0;
			for (size_t idx = 0U; idx < SEQUENCE_SAMPLES; idx++) {
				val_mv = channel_reading[idx][ch_idx];
				switch(adc_channels[ch_idx].channel_cfg.channel_id)
				{
					case 5:  /* handling NTC temperature channel */
						const float temp = adc_raw_val2temp(val_mv);
						printf("- - %d = %.2f °C\n", val_mv, (double)temp);
						break;
					default:
						printf("- - %d", val_mv);
						err = adc_raw_to_millivolts_dt(&adc_channels[ch_idx],
								&val_mv);
						/* Conversion to mV may not be supported, skip if not */
						if (err < 0) {
							printf(" (value in mV not available)\n");
						} else {
							printf(" = %d mV\n", val_mv);
						}
						break;
				}
			}
		}
	}
	return 0;
}

/* thread_sensor_measuremet is a static thread that is spawned automatically */
int thread_sensor_measuremet(void *, void *, void *)
{
	int rc = 0;

	for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
		if (!device_is_ready(sensors[i])) {
			printf("sensor: device %s not ready.\n", sensors[i]->name);
			return rc;
		}
	}

#ifndef CONFIG_COVERAGE
	while (1) {
#else
	for (int k = 0; k < 10; k++) {
#endif
		k_msleep(SLEEP_TIME_MS);

		for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
			struct sensor_value val;

			/* fetch sensor samples */
			rc = sensor_sample_fetch(sensors[i]);
			if (rc) {
				printf("Failed to fetch sample (%d)\n", rc);
				return rc;
			}

			const int16_t sens_ch = i != 0 ? SENSOR_CHAN_VOLTAGE : SENSOR_CHAN_DIE_TEMP;
			rc = sensor_channel_get(sensors[i], sens_ch, &val);
			if (rc) {
				printf("Failed to get data (%d)\n", rc);
				return rc;
			}

			printf("Sensor voltage[%s]: %.2f units\n", sensors[i]->name,
					sensor_value_to_double(&val));
		}
	}

	return 0;
}

int main(void)
{
	int ret = 0;

	ret = device_is_ready(comp1_dev);
	if (!ret) {
		printk("Device not ready: comp1 (err: %d)\n", ret);
		return ret;
	}

	/* failing on lock is okay here */
	(void)comparator_set_trigger(comp1_dev, COMPARATOR_TRIGGER_BOTH_EDGES);

#ifndef CONFIG_COVERAGE
	while (1) {
#else
	for (int k = 0; k < 10; k++) {
#endif
		k_msleep(SLEEP_TIME_MS/4);
		ret = comparator_get_output(comp1_dev);
		printf("- comp_val: %d\n", ret);
	}

	return 0;
}

K_THREAD_DEFINE(threadADCMeasuremet, THREAD_ADC_STACK_SIZE,
	thread_adc_measuremet, NULL, NULL, NULL,
	THREAD_ADC_PRIORITY, 0, 0);
K_THREAD_DEFINE(threadSensorMeasuremet, THREAD_SENSOR_STACK_SIZE,
	thread_sensor_measuremet, NULL, NULL, NULL,
	THREAD_SENSOR_PRIORITY, 0, 0);
