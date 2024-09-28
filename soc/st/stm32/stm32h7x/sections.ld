/*
 * Copyright (c) 2020 Mario Jaun
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sram3), okay)
#define sram_region_name	sram3
#else
#define sram_region_name	sram2
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(mac), okay)

SECTION_DATA_PROLOGUE(eth_stm32,(NOLOAD),)
{
	. = ABSOLUTE(DT_REG_ADDR(DT_NODELABEL(eth_stm32_sram)));
	*(.eth_stm32_desc)
	. = ABSOLUTE(DT_REG_ADDR(DT_NODELABEL(eth_stm32_sram))) + 256;
	*(.eth_stm32_buf)
	. = ABSOLUTE(DT_REG_ADDR(DT_NODELABEL(eth_stm32_sram))) + 16K;
} GROUP_DATA_LINK_IN(LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(eth_stm32_sram)),
					 LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(eth_stm32_sram)))

#endif
