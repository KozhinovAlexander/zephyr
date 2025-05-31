/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_COMPARATOR_STM32H7_COMP_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_COMPARATOR_STM32H7_COMP_H_

#include <zephyr/dt-bindings/comparator/stm32_comp.h>

/* STM32 ADC resolution register for H7 and similar */
#define STM32_ADC_RES_REG	0x0C
#define STM32_ADC_RES_SHIFT	2
#define STM32_ADC_RES_MASK	BIT_MASK(3)

/*
 * For STM32H72x & H73x, ADC3 is different and has a 12 to 6-bit resolution.
 * The offset and the width of the resolution field need to be redefined.
 */
#define STM32H72X_ADC3_RES_SHIFT	3
#define STM32H72X_ADC3_RES_MASK		0x03

#define STM32H72X_ADC3_RES(resolution, reg_val)	\
	STM32_ADC(resolution, reg_val, STM32H72X_ADC3_RES_MASK, \
		  STM32H72X_ADC3_RES_SHIFT, STM32_ADC_RES_REG)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_COMPARATOR_STM32H7_COMP_H_ */
