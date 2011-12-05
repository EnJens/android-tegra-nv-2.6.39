/* OK, info extracted from a running .32 kernel */
/*
 * Copyright (C) 2011 Eduardo José Tagle <ejtagle@tutopia.com>
 * Copyright (C) 2010 Google, Inc.
 *
 * Authors:
 *	Colin Cross <ccross@google.com>
 *	Eduardo José Tagle <ejtagle@tutopia.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/* Tegra2 external memory controller initialization */
 
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>

#include "board-adam.h"
#include "tegra2_emc.h"

#if defined (ADAM_EMC_SAMSUNG)

/* Samsung: manufacturer_id = 0x0101, mem_pid = 0x5454 */
static const struct tegra_emc_table adam_emc_tables[] = {
	{
		.rate = 25000,   /* SDRAM frequency */
		.regs = {
			0x00000002,   /* RC */
			0x00000006,   /* RFC */
			0x00000003,   /* RAS */
			0x00000003,   /* RP */
			0x00000006,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x00000009,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000002,   /* REXT */
			0x00000002,   /* WDV */
			0x00000004,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000008,   /* QSAFE */
			0x0000000b,   /* RDV */
			0x0000004d,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000008,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000a,   /* RW2PDEN */
			0x00000004,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000008,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x00000006,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000068,   /* TREFBW */
			0x00000003,   /* QUSE_EXTRA */
			0x00000003,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000082,   /* FBIO_CFG5 */
			0xa09404ae,   /* CFG_DIG_DLL */
			0x00070000,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000003,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	},
	{
		.rate = 50000,   /* SDRAM frequency */
		.regs = {
			0x00000003,   /* RC */
			0x00000007,   /* RFC */
			0x00000003,   /* RAS */
			0x00000003,   /* RP */
			0x00000006,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x00000009,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000002,   /* REXT */
			0x00000002,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000008,   /* QSAFE */
			0x0000000b,   /* RDV */
			0x0000009f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000008,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000a,   /* RW2PDEN */
			0x00000007,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000008,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x00000006,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x000000d0,   /* TREFBW */
			0x00000004,   /* QUSE_EXTRA */
			0x00000000,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000082,   /* FBIO_CFG5 */
			0xa09404ae,   /* CFG_DIG_DLL */
			0x00070000,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000005,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	},
	{
		.rate = 75000,   /* SDRAM frequency */
		.regs = {
			0x00000005,   /* RC */
			0x0000000a,   /* RFC */
			0x00000004,   /* RAS */
			0x00000003,   /* RP */
			0x00000006,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x00000009,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000002,   /* REXT */
			0x00000002,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000008,   /* QSAFE */
			0x0000000b,   /* RDV */
			0x000000ff,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000008,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000a,   /* RW2PDEN */
			0x0000000b,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000008,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x00000006,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000138,   /* TREFBW */
			0x00000004,   /* QUSE_EXTRA */
			0x00000000,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000082,   /* FBIO_CFG5 */
			0xa09404ae,   /* CFG_DIG_DLL */
			0x00070000,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000007,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	},
	{
		.rate = 150000,   /* SDRAM frequency */
		.regs = {
			0x00000009,   /* RC */
			0x00000014,   /* RFC */
			0x00000007,   /* RAS */
			0x00000003,   /* RP */
			0x00000006,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x00000009,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000002,   /* REXT */
			0x00000002,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000008,   /* QSAFE */
			0x0000000b,   /* RDV */
			0x0000021f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000008,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000a,   /* RW2PDEN */
			0x00000015,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000008,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x00000006,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000270,   /* TREFBW */
			0x00000004,   /* QUSE_EXTRA */
			0x00000001,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000082,   /* FBIO_CFG5 */
			0xa06804ae,   /* CFG_DIG_DLL */
			0x007e0010,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x0000000e,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	},
	{
		.rate = 300000,   /* SDRAM frequency */
		.regs = {
			0x00000012,   /* RC */
			0x00000027,   /* RFC */
			0x0000000d,   /* RAS */
			0x00000006,   /* RP */
			0x00000007,   /* R2W */
			0x00000005,   /* W2R */
			0x00000003,   /* R2P */
			0x00000009,   /* W2P */
			0x00000006,   /* RD_RCD */
			0x00000006,   /* WR_RCD */
			0x00000003,   /* RRD */
			0x00000003,   /* REXT */
			0x00000002,   /* WDV */
			0x00000006,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000c,   /* RDV */
			0x0000045f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000004,   /* PDEX2WR */
			0x00000004,   /* PDEX2RD */
			0x00000006,   /* PCHG2PDEN */
			0x00000008,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000e,   /* RW2PDEN */
			0x0000002a,   /* TXSR */
			0x00000003,   /* TCKE */
			0x0000000f,   /* TFAW */
			0x00000007,   /* TRPAB */
			0x00000005,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x000004e0,   /* TREFBW */
			0x00000005,   /* QUSE_EXTRA */
			0x00000002,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000282,   /* FBIO_CFG5 */
			0xe04e048b,   /* CFG_DIG_DLL */
			0x007e0010,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x0000001b,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}
};

#elif defined (ADAM_EMC_ELPIDA50NM)

/* Elpida 50nm: .mem_manufacturer_id = 0x0303, .mem_pid = 0x5454 */
static const struct tegra_emc_table adam_emc_tables[] = {
	{
		.rate = 25000,   /* SDRAM frequency */
		.regs = {
			0x00000002,   /* RC */
			0x00000006,   /* RFC */
			0x00000003,   /* RAS */
			0x00000003,   /* RP */
			0x00000006,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x00000009,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000002,   /* REXT */
			0x00000002,   /* WDV */
			0x00000004,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000008,   /* QSAFE */
			0x0000000b,   /* RDV */
			0x0000004d,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000008,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000a,   /* RW2PDEN */
			0x00000004,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000008,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x00000006,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000068,   /* TREFBW */
			0x00000003,   /* QUSE_EXTRA */
			0x00000003,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000082,   /* FBIO_CFG5 */
			0xa06a04ae,   /* CFG_DIG_DLL */
			0x00070000,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000003,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	},
	{
		.rate = 50000,   /* SDRAM frequency */
		.regs = {
			0x00000003,   /* RC */
			0x00000007,   /* RFC */
			0x00000003,   /* RAS */
			0x00000003,   /* RP */
			0x00000006,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x00000009,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000002,   /* REXT */
			0x00000002,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000008,   /* QSAFE */
			0x0000000b,   /* RDV */
			0x0000009f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000008,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000a,   /* RW2PDEN */
			0x00000007,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000008,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x00000006,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x000000d0,   /* TREFBW */
			0x00000004,   /* QUSE_EXTRA */
			0x00000000,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000082,   /* FBIO_CFG5 */
			0xa06a04ae,   /* CFG_DIG_DLL */
			0x00070000,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000005,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	},
	{
		.rate = 75000,   /* SDRAM frequency */
		.regs = {
			0x00000005,   /* RC */
			0x0000000a,   /* RFC */
			0x00000004,   /* RAS */
			0x00000003,   /* RP */
			0x00000006,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x00000009,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000002,   /* REXT */
			0x00000002,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000008,   /* QSAFE */
			0x0000000b,   /* RDV */
			0x000000ff,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000008,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000a,   /* RW2PDEN */
			0x0000000b,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000008,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x00000006,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000138,   /* TREFBW */
			0x00000004,   /* QUSE_EXTRA */
			0x00000000,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000082,   /* FBIO_CFG5 */
			0xa06a04ae,   /* CFG_DIG_DLL */
			0x00070000,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000007,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	},
	{
		.rate = 150000,   /* SDRAM frequency */
		.regs = {
			0x00000009,   /* RC */
			0x00000014,   /* RFC */
			0x00000007,   /* RAS */
			0x00000003,   /* RP */
			0x00000006,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x00000009,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000002,   /* REXT */
			0x00000002,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000008,   /* QSAFE */
			0x0000000b,   /* RDV */
			0x0000021f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000008,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000a,   /* RW2PDEN */
			0x00000015,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000008,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x00000006,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000270,   /* TREFBW */
			0x00000004,   /* QUSE_EXTRA */
			0x00000001,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000082,   /* FBIO_CFG5 */
			0xa04c04ae,   /* CFG_DIG_DLL */
			0x007e0010,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x0000000e,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	},
	{
		.rate = 300000,   /* SDRAM frequency */
		.regs = {
			0x00000012,   /* RC */
			0x00000027,   /* RFC */
			0x0000000d,   /* RAS */
			0x00000006,   /* RP */
			0x00000007,   /* R2W */
			0x00000005,   /* W2R */
			0x00000003,   /* R2P */
			0x00000009,   /* W2P */
			0x00000006,   /* RD_RCD */
			0x00000006,   /* WR_RCD */
			0x00000003,   /* RRD */
			0x00000003,   /* REXT */
			0x00000002,   /* WDV */
			0x00000006,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000c,   /* RDV */
			0x0000045f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000004,   /* PDEX2WR */
			0x00000004,   /* PDEX2RD */
			0x00000006,   /* PCHG2PDEN */
			0x00000008,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000e,   /* RW2PDEN */
			0x0000002a,   /* TXSR */
			0x00000003,   /* TCKE */
			0x0000000f,   /* TFAW */
			0x00000007,   /* TRPAB */
			0x00000005,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x000004e0,   /* TREFBW */
			0x00000005,   /* QUSE_EXTRA */
			0x00000002,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000282,   /* FBIO_CFG5 */
			0xe03c048b,   /* CFG_DIG_DLL */
			0x007e0010,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x0000001b,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}
};

#elif defined (ADAM_EMC_ELPIDA40NM)

/* Elpida 40nm : mem_manufacturer_id = 0x0303, mem_revision_id1 = 0x0101, mem_pid = 0x5454 */
static const struct tegra_emc_table adam_emc_tables[] = {
	{
		.rate = 25000,   /* SDRAM frequency */
		.regs = {
			0x00000002,   /* RC */
			0x00000006,   /* RFC */
			0x00000003,   /* RAS */
			0x00000003,   /* RP */
			0x00000006,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x00000009,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000002,   /* REXT */
			0x00000002,   /* WDV */
			0x00000004,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000008,   /* QSAFE */
			0x0000000b,   /* RDV */
			0x0000004d,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000008,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000a,   /* RW2PDEN */
			0x00000004,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000008,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x00000006,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000068,   /* TREFBW */
			0x00000003,   /* QUSE_EXTRA */
			0x00000003,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000082,   /* FBIO_CFG5 */
			0xa0ae04ae,   /* CFG_DIG_DLL */
			0x00070000,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000003,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	},
	{
		.rate = 50000,   /* SDRAM frequency */
		.regs = {
			0x00000003,   /* RC */
			0x00000007,   /* RFC */
			0x00000003,   /* RAS */
			0x00000003,   /* RP */
			0x00000006,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x00000009,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000002,   /* REXT */
			0x00000002,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000008,   /* QSAFE */
			0x0000000b,   /* RDV */
			0x0000009f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000008,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000a,   /* RW2PDEN */
			0x00000007,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000008,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x00000006,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x000000d0,   /* TREFBW */
			0x00000004,   /* QUSE_EXTRA */
			0x00000000,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000082,   /* FBIO_CFG5 */
			0xa0ae04ae,   /* CFG_DIG_DLL */
			0x00070000,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000005,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	},
	{
		.rate = 75000,   /* SDRAM frequency */
		.regs = {
			0x00000005,   /* RC */
			0x0000000a,   /* RFC */
			0x00000004,   /* RAS */
			0x00000003,   /* RP */
			0x00000006,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x00000009,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000002,   /* REXT */
			0x00000002,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000008,   /* QSAFE */
			0x0000000b,   /* RDV */
			0x000000ff,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000008,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000a,   /* RW2PDEN */
			0x0000000b,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000008,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x00000006,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000138,   /* TREFBW */
			0x00000004,   /* QUSE_EXTRA */
			0x00000000,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000082,   /* FBIO_CFG5 */
			0xa0ae04ae,   /* CFG_DIG_DLL */
			0x00070000,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000007,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	},
	{
		.rate = 150000,   /* SDRAM frequency */
		.regs = {
			0x00000009,   /* RC */
			0x00000014,   /* RFC */
			0x00000007,   /* RAS */
			0x00000003,   /* RP */
			0x00000006,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x00000009,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000002,   /* REXT */
			0x00000002,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000008,   /* QSAFE */
			0x0000000b,   /* RDV */
			0x0000021f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000008,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000a,   /* RW2PDEN */
			0x00000015,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000008,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x00000006,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000270,   /* TREFBW */
			0x00000004,   /* QUSE_EXTRA */
			0x00000001,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000082,   /* FBIO_CFG5 */
			0xa07c04ae,   /* CFG_DIG_DLL */
			0x007de010,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x0000000e,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	},
	{
		.rate = 300000,   /* SDRAM frequency */
		.regs = {
			0x00000012,   /* RC */
			0x00000027,   /* RFC */
			0x0000000d,   /* RAS */
			0x00000006,   /* RP */
			0x00000007,   /* R2W */
			0x00000005,   /* W2R */
			0x00000003,   /* R2P */
			0x00000009,   /* W2P */
			0x00000006,   /* RD_RCD */
			0x00000006,   /* WR_RCD */
			0x00000003,   /* RRD */
			0x00000003,   /* REXT */
			0x00000002,   /* WDV */
			0x00000006,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000c,   /* RDV */
			0x0000045f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000004,   /* PDEX2WR */
			0x00000004,   /* PDEX2RD */
			0x00000006,   /* PCHG2PDEN */
			0x00000008,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000e,   /* RW2PDEN */
			0x0000002a,   /* TXSR */
			0x00000003,   /* TCKE */
			0x0000000f,   /* TFAW */
			0x00000007,   /* TRPAB */
			0x00000005,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x000004e0,   /* TREFBW */
			0x00000005,   /* QUSE_EXTRA */
			0x00000002,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000282,   /* FBIO_CFG5 */
			0xe059048b,   /* CFG_DIG_DLL */
			0x007e4010,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x0000001b,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}
};

#elif 1

// This is the table used by nvidia ODM
static const struct tegra_emc_table adam_emc_tables[] = {
	{
		.rate = 166500,   /* SDRAM frequency */
		.regs = {
            0x0000000A,   /* RC */
            0x00000016,   /* RFC */
            0x00000008,   /* RAS */
            0x00000003,   /* RP */
            0x00000004,   /* R2W */
            0x00000004,   /* W2R */
            0x00000002,   /* R2P */
            0x0000000C,   /* W2P */
            0x00000003,   /* RD_RCD */
            0x00000003,   /* WR_RCD */
            0x00000002,   /* RRD */
            0x00000001,   /* REXT */
            0x00000004,   /* WDV */
            0x00000005,   /* QUSE */
            0x00000004,   /* QRST */
            0x00000009,   /* QSAFE */
            0x0000000D,   /* RDV */
            0x000004DF,   /* REFRESH */
            0x00000000,   /* BURST_REFRESH_NUM */
            0x00000003,   /* PDEX2WR */
            0x00000003,   /* PDEX2RD */
            0x00000003,   /* PCHG2PDEN */
            0x00000003,   /* ACT2PDEN */
            0x00000001,   /* AR2PDEN */
            0x0000000A,   /* RW2PDEN */
            0x000000C8,   /* TXSR */
            0x00000003,   /* TCKE */
            0x00000006,   /* TFAW */
            0x00000004,   /* TRPAB */
            0x00000008,   /* TCLKSTABLE */
            0x00000002,   /* TCLKSTOP */
            0x00000000,   /* TREFBW */
            0x00000000,   /* QUSE_EXTRA */
            0x00000002,   /* FBIO_CFG6 */
            0x00000000,   /* ODT_WRITE */
            0x00000000,   /* ODT_READ */
            0x00000083,   /* FBIO_CFG5 */
            0xA04204AE,   /* CFG_DIG_DLL */
            0x007FD010,   /* DLL_XFORM_DQS */
            0x00000000,   /* DLL_XFORM_QUSE */
            0x00000000,   /* ZCAL_REF_CNT */
            0x00000000,   /* ZCAL_WAIT_CNT */
            0x00000000,   /* AUTO_CAL_INTERVAL */
            0x00000000,   /* CFG_CLKTRIM_0 */
            0x00000000,   /* CFG_CLKTRIM_1 */
            0x00000000,   /* CFG_CLKTRIM_2 */
        }
    },
	{
		.rate = 333000,   /* SDRAM frequency */
		.regs = {
            0x00000014,   /* RC */
            0x0000002B,   /* RFC */
            0x0000000F,   /* RAS */
            0x00000005,   /* RP */
            0x00000004,   /* R2W */
            0x00000005,   /* W2R */
            0x00000003,   /* R2P */
            0x0000000C,   /* W2P */
            0x00000005,   /* RD_RCD */
            0x00000005,   /* WR_RCD */
            0x00000003,   /* RRD */
            0x00000001,   /* REXT */
            0x00000004,   /* WDV */
            0x00000005,   /* QUSE */
            0x00000004,   /* QRST */
            0x00000009,   /* QSAFE */
            0x0000000D,   /* RDV */
            0x000009FF,   /* REFRESH */
            0x00000000,   /* BURST_REFRESH_NUM */
            0x00000003,   /* PDEX2WR */
            0x00000003,   /* PDEX2RD */
            0x00000005,   /* PCHG2PDEN */
            0x00000005,   /* ACT2PDEN */
            0x00000001,   /* AR2PDEN */
            0x0000000F,   /* RW2PDEN */
            0x000000C8,   /* TXSR */
            0x00000003,   /* TCKE */
            0x0000000C,   /* TFAW */
            0x00000006,   /* TRPAB */
            0x00000008,   /* TCLKSTABLE */
            0x00000002,   /* TCLKSTOP */
            0x00000000,   /* TREFBW */
            0x00000000,   /* QUSE_EXTRA */
            0x00000002,   /* FBIO_CFG6 */
            0x00000000,   /* ODT_WRITE */
            0x00000000,   /* ODT_READ */
            0x00000083,   /* FBIO_CFG5 */
            0xE034048B,   /* CFG_DIG_DLL */
            0x007FC010,   /* DLL_XFORM_DQS */
            0x00000000,   /* DLL_XFORM_QUSE */
            0x00000000,   /* ZCAL_REF_CNT */
            0x00000000,   /* ZCAL_WAIT_CNT */
            0x00000000,   /* AUTO_CAL_INTERVAL */
            0x00000000,   /* CFG_CLKTRIM_0 */
            0x00000000,   /* CFG_CLKTRIM_1 */
            0x00000000,   /* CFG_CLKTRIM_2 */
        }
    }
};

#elif 0

/* This is the LPDDR2 sdram used in Shuttle: Hynax HY5PS1G831C-S5 

	Tck  = 2.5ns -> DDR2-800
	CL	 = 5 Clk
	tRCD = 5 Clk
	tRP  = 5 Clk

	VDD = 1.8 +/- 0.1V
	VDDQ = 1.8 +/- 0.1V
	
	2 chips x 128Mx8
	
	Programmable CAS latency 3, 4, 5 and 6 supported
	Programmable additive latency 0, 1, 2, 3, 4 and 5 supported
	Programmable burst length 4/8 with both nibble sequential and interleave mode
	Internal eight bank operations with single pulsed RAS
	Auto refresh and self refresh supported
	tRAS lockout supported
	8K refresh cycles /64ms	
	
	Page size = 1kbyte

	*/
	{
		.rate = 400000,   /* SDRAM frequency */
		.regs = {
			// specifies the row cycle time.
			// This is the minimum number of cycles between activate commands to the same bank. 
            0x0000003,    /* RC = tRRD = 7.5nS row cycle time, but tRCD = 5 */
			
			// specifies the auto refresh cycle time.
			// This is the minimum number of cycles between an auto refresh command and a subsequent auto refresh
			// or activate command. 
            0x00000033,   /* RFC = tRFC = 127.5 nS */
			
			// specifies the row active time. 
			// This is the minimum number of cycles between an activate command and a precharge command to the same bank. 
            0x00000012,   /* RAS = tRAS = 45 nS*/
			
			// specifies the row precharge time.
			// This is the minimum number of cycles between a precharge command and an activate command to the same bank. 
            0x00000005,   /* RP  = tRP = 12.5nS */
			
			// specifies the minimum number of cycles from any read command to any write command,
			// irrespective of bank.  This parameter guarantees the read->write turn-around time on the bus.
			// Set to ((CL+1)-WL + R2W_bus_turnaround_clks). If ODT is enabled,  set to ((CL+1)-WL + R2W_bus_turnaround_clks + 1)).
			// Largest programming value is 29 
            0x00000004,   /* R2W */ ??
			
			// specifies the minimum number of cycles from a write command to a read command,
			// irrespective of bank. Set to ((WL+1) + tWTR).
			// Largest programming value is 29 
            0x00000003,   /* W2R = tWTR = 7.5nS = write to read delay */
			
			// specifies the minimum number of cycles from a read command to
			// a precharge command for the same bank.  Set to 1 clock. 
            0x00000003,   /* R2P = tRTP = 7.5nS*/
			
			// specifies the minimum number of cycles from a write command to
			// a precharge command for the same bank. Set to ((WL+1) + tWR). 
            0x00000006,   /* W2P = tWR  = 15nS */
			
			// specifies the ras to cas delay.
			// RD_RCD is the minimum number of cycles between an activate command and a read command to the same bank. 
            0x00000005,   /* RD_RCD = tRCD = 5*/
			
			// minimum number of cycles between an activate command and a
			// write command to the same bank. 
            0x00000005,   /* WR_RCD = tRCD = 5*/
			
			// specifies the Bank X Act to Bank Y Act command delay. 
            0x00000003,   /* RRD = row to row delay */ ??
			
			// specifies the read to read delay for reads when 
			// multiple physical devices are present. 
            0x00000001,   /* REXT */ ??
			
			// the number of cycles to post (delay) write data from being asserted
			// to the rams. Set to 0 for DDR1 operation. For DDR1, the delay obtained is the programmed value + 1. 
            0x00000004,   /* WDV */ ??
			
			// tells the chip when to look for read return data. 
            0x00000005,   /* QUSE */ ??
			
			// time from expiration of QSAFE until reset is issued 
            0x00000004,   /* QRST */ ??
			
			// time from a read command to when it is safe to issue a QRST (delayed by the QRST parameter). 
            0x00000009,   /* QSAFE */ ??
			
			// time from read command to latching the read data from the pad macros. 
            0x0000000D,   /* RDV */ ??
			
			// specifies the interval between refresh requests. 
            0x000009FF,   /* REFRESH */ ??
			
			// specify the refresh burst count. 
            0x00000000,   /* BURST_REFRESH_NUM */
			
			// specify the timing delay from exit of powerdown mode to a write command.
			// Largest allowed value is 14 
            0x00000003,   /* PDEX2WR */
			
			// specify the timing delay from exit of powerdown mode to a read command.
			// Largest allowed value is 14 
            0x00000003,   /* PDEX2RD */
			
			//  specify the timing delay from a precharge command to powerdown entry. 
            0x00000005,   /* PCHG2PDEN */
			
			// specify the timing delay from an activate, mrs or emrs command to powerdown entry. 
            0x00000005,   /* ACT2PDEN */
			
			// specify the timing delay from an autorefresh command to powerdown entry. 
            0x00000001,   /* AR2PDEN */
			
			// specify the timing delay from a read/write command to powerdown entry.
			//  Auto-precharge timing must be taken into account when programming this field (affects lpddr & lpddr2/ddr2 differently). 
            0x0000000F,   /* RW2PDEN */
			
			// cycles between self-refresh exit & first DRAM command
			// Largest allowed value is 0xffe 
            0x000000C8,   /* TXSR */
			
			// specify minimum CKE pulse width. 
            0x00000003,   /* TCKE */
			
			// specify the width of the FAW (four-activate window) for 8-bank devices.
			// Set to 0 to disable this timing check. Only 4 activates may occur withing the rolling window.  
            0x0000000C,   /* TFAW */
			
			// specify precharge-all tRP allowance for 8-bank devices.
			// Setting this field to 0 will cause EMC to use TRP.TRP for precharge-all. 
            0x00000006,   /* TRPAB */
			
			// specify minimum number of cycles of a stable clock period
			// prior to exiting powerdown or self-refresh modes. 
            0x00000008,   /* TCLKSTABLE */
			
			// delay from last command to stopping the external clock to DRAM devices. 
            0x00000002,   /* TCLKSTOP */
			
			// specify the width of the burst-refresh window.
			// If set to a non-zero value, only 8 refreshes will occur in this rolling window.
			// Set to 0 to disable this timing check.  
            0x00000000,   /* TREFBW */
			
            0x00000000,   /* QUSE_EXTRA */
            0x00000002,   /* FBIO_CFG6 -> Late default*/
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
            0x00000082,   /* FBIO_CFG5 -> 3=DDR2, 2=LPDDR2 */
            0xE034048B,   /* CFG_DIG_DLL */
            0x007FC010,   /* DLL_XFORM_DQS */
            0x00000000,   /* DLL_XFORM_QUSE */
            0x00000000,   /* ZCAL_REF_CNT */
            0x00000000,   /* ZCAL_WAIT_CNT */
            0x00000000,   /* AUTO_CAL_INTERVAL */
            0x00000000,   /* CFG_CLKTRIM_0 */
            0x00000000,   /* CFG_CLKTRIM_1 */
            0x00000000,   /* CFG_CLKTRIM_2 */
        }
    }


#else

// This is a Hynix compatible memory information table
static const struct tegra_emc_table adam_emc_tables[] = {
	{
		.rate =  18000,   /* SDRAM frequency : 950mV emc core voltage*/
		.regs = {
            0x00000002,   /* RC */
            0x00000006,   /* RFC */
            0x00000003,   /* RAS */
            0x00000003,   /* RP */
            0x00000006,   /* R2W */
            0x00000004,   /* W2R */
            0x00000002,   /* R2P */
            0x0000000B,   /* W2P */
            0x00000003,   /* RD_RCD */
            0x00000003,   /* WR_RCD */
            0x00000002,   /* RRD */
            0x00000002,   /* REXT */
            0x00000003,   /* WDV */
            0x00000005,   /* QUSE */
            0x00000004,   /* QRST */
            0x00000008,   /* QSAFE */
            0x0000000C,   /* RDV */
            0x00000070,   /* REFRESH */
            0x00000000,   /* BURST_REFRESH_NUM */
            0x00000003,   /* PDEX2WR */
            0x00000003,   /* PDEX2RD */
            0x00000003,   /* PCHG2PDEN */
            0x00000003,   /* ACT2PDEN */
            0x00000001,   /* AR2PDEN */
            0x0000000B,   /* RW2PDEN */
            0x00000003,   /* TXSR */
            0x00000003,   /* TCKE */
            0x00000008,   /* TFAW */
            0x00000004,   /* TRPAB */
            0x00000008,   /* TCLKSTABLE */
            0x00000002,   /* TCLKSTOP */
            0x0000004B,   /* TREFBW */
            0x00000000,   /* QUSE_EXTRA */
            0x00000003,   /* FBIO_CFG6 */
            0x00000000,   /* ODT_WRITE */
            0x00000000,   /* ODT_READ */
            0x00000082,   /* FBIO_CFG5 */
            0x00780006,   /* CFG_DIG_DLL */
            0x00000010,   /* DLL_XFORM_DQS */
            0x00000008,   /* DLL_XFORM_QUSE */
            0x00000000,   /* ZCAL_REF_CNT */
            0x00000002,   /* ZCAL_WAIT_CNT */
            0x00000000,   /* AUTO_CAL_INTERVAL */
            0x00000000,   /* CFG_CLKTRIM_0 */
            0x00000000,   /* CFG_CLKTRIM_1 */
            0x00000000,   /* CFG_CLKTRIM_2 */
        }
    },
    {
		.rate =  27000,   /* SDRAM frequency : 950mV emc core voltage*/
		.regs = {
            0x00000002,   /* RC */
            0x00000006,   /* RFC */
            0x00000003,   /* RAS */
            0x00000003,   /* RP */
            0x00000006,   /* R2W */
            0x00000004,   /* W2R */
            0x00000002,   /* R2P */
            0x0000000B,   /* W2P */
            0x00000003,   /* RD_RCD */
            0x00000003,   /* WR_RCD */
            0x00000002,   /* RRD */
            0x00000002,   /* REXT */
            0x00000003,   /* WDV */
            0x00000005,   /* QUSE */
            0x00000004,   /* QRST */
            0x00000008,   /* QSAFE */
            0x0000000C,   /* RDV */
            0x000000A8,   /* REFRESH */
            0x00000000,   /* BURST_REFRESH_NUM */
            0x00000003,   /* PDEX2WR */
            0x00000003,   /* PDEX2RD */
            0x00000003,   /* PCHG2PDEN */
            0x00000003,   /* ACT2PDEN */
            0x00000001,   /* AR2PDEN */
            0x0000000B,   /* RW2PDEN */
            0x00000004,   /* TXSR */
            0x00000003,   /* TCKE */
            0x00000008,   /* TFAW */
            0x00000004,   /* TRPAB */
            0x00000008,   /* TCLKSTABLE */
            0x00000002,   /* TCLKSTOP */
            0x00000071,   /* TREFBW */
            0x00000000,   /* QUSE_EXTRA */
            0x00000003,   /* FBIO_CFG6 */
            0x00000000,   /* ODT_WRITE */
            0x00000000,   /* ODT_READ */
            0x00000082,   /* FBIO_CFG5 */
            0x00780006,   /* CFG_DIG_DLL */
            0x00000010,   /* DLL_XFORM_DQS */
            0x00000008,   /* DLL_XFORM_QUSE */
            0x00000000,   /* ZCAL_REF_CNT */
            0x00000003,   /* ZCAL_WAIT_CNT */
            0x00000000,   /* AUTO_CAL_INTERVAL */
            0x00000000,   /* CFG_CLKTRIM_0 */
            0x00000000,   /* CFG_CLKTRIM_1 */
            0x00000000,   /* CFG_CLKTRIM_2 */
        }
    },
    {
		.rate =  54000,   /* SDRAM frequency : 1000mV emc core voltage*/
		.regs = {
            0x00000004,   /* RC */
            0x00000008,   /* RFC */
            0x00000003,   /* RAS */
            0x00000003,   /* RP */
            0x00000006,   /* R2W */
            0x00000004,   /* W2R */
            0x00000002,   /* R2P */
            0x0000000B,   /* W2P */
            0x00000003,   /* RD_RCD */
            0x00000003,   /* WR_RCD */
            0x00000002,   /* RRD */
            0x00000002,   /* REXT */
            0x00000003,   /* WDV */
            0x00000006,   /* QUSE */
            0x00000004,   /* QRST */
            0x00000008,   /* QSAFE */
            0x0000000C,   /* RDV */
            0x0000017F,   /* REFRESH */
            0x00000000,   /* BURST_REFRESH_NUM */
            0x00000003,   /* PDEX2WR */
            0x00000003,   /* PDEX2RD */
            0x00000003,   /* PCHG2PDEN */
            0x00000003,   /* ACT2PDEN */
            0x00000001,   /* AR2PDEN */
            0x0000000B,   /* RW2PDEN */
            0x00000008,   /* TXSR */
            0x00000003,   /* TCKE */
            0x00000008,   /* TFAW */
            0x00000004,   /* TRPAB */
            0x00000008,   /* TCLKSTABLE */
            0x00000002,   /* TCLKSTOP */
            0x000000E1,   /* TREFBW */
            0x00000000,   /* QUSE_EXTRA */
            0x00000000,   /* FBIO_CFG6 */
            0x00000000,   /* ODT_WRITE */
            0x00000000,   /* ODT_READ */
            0x00000082,   /* FBIO_CFG5 */
            0x00780006,   /* CFG_DIG_DLL */
            0x00000010,   /* DLL_XFORM_DQS */
            0x00000008,   /* DLL_XFORM_QUSE */
            0x00000000,   /* ZCAL_REF_CNT */
            0x00000005,   /* ZCAL_WAIT_CNT */
            0x00000000,   /* AUTO_CAL_INTERVAL */
            0x00000000,   /* CFG_CLKTRIM_0 */
            0x00000000,   /* CFG_CLKTRIM_1 */
            0x00000000,   /* CFG_CLKTRIM_2 */
        }
    },
    {
		.rate = 108000,   /* SDRAM frequency : 1000mV emc core voltage*/
		.regs = {
            0x00000007,   /* RC */
            0x0000000F,   /* RFC */
            0x00000005,   /* RAS */
            0x00000003,   /* RP */
            0x00000006,   /* R2W */
            0x00000004,   /* W2R */
            0x00000002,   /* R2P */
            0x0000000B,   /* W2P */
            0x00000003,   /* RD_RCD */
            0x00000003,   /* WR_RCD */
            0x00000002,   /* RRD */
            0x00000002,   /* REXT */
            0x00000003,   /* WDV */
            0x00000006,   /* QUSE */
            0x00000004,   /* QRST */
            0x00000008,   /* QSAFE */
            0x0000000C,   /* RDV */
            0x0000031F,   /* REFRESH */
            0x00000000,   /* BURST_REFRESH_NUM */
            0x00000003,   /* PDEX2WR */
            0x00000003,   /* PDEX2RD */
            0x00000003,   /* PCHG2PDEN */
            0x00000003,   /* ACT2PDEN */
            0x00000001,   /* AR2PDEN */
            0x0000000B,   /* RW2PDEN */
            0x00000010,   /* TXSR */
            0x00000003,   /* TCKE */
            0x00000008,   /* TFAW */
            0x00000004,   /* TRPAB */
            0x00000008,   /* TCLKSTABLE */
            0x00000002,   /* TCLKSTOP */
            0x000001C2,   /* TREFBW */
            0x00000000,   /* QUSE_EXTRA */
            0x00000000,   /* FBIO_CFG6 */
            0x00000000,   /* ODT_WRITE */
            0x00000000,   /* ODT_READ */
            0x00000082,   /* FBIO_CFG5 */
            0xD0780323,   /* CFG_DIG_DLL */
            0x007FD010,   /* DLL_XFORM_DQS */
            0x00000010,   /* DLL_XFORM_QUSE */
            0x00000000,   /* ZCAL_REF_CNT */
            0x0000000A,   /* ZCAL_WAIT_CNT */
            0x00000000,   /* AUTO_CAL_INTERVAL */
            0x00000000,   /* CFG_CLKTRIM_0 */
            0x00000000,   /* CFG_CLKTRIM_1 */
            0x00000000,   /* CFG_CLKTRIM_2 */
        }
    },
    {
		.rate = 150000,   /* SDRAM frequency : 1000mV emc core voltage*/
		.regs = {
            0x00000009,   /* RC */
            0x00000014,   /* RFC */
            0x00000007,   /* RAS */
            0x00000003,   /* RP */
            0x00000006,   /* R2W */
            0x00000004,   /* W2R */
            0x00000002,   /* R2P */
            0x0000000B,   /* W2P */
            0x00000003,   /* RD_RCD */
            0x00000003,   /* WR_RCD */
            0x00000002,   /* RRD */
            0x00000002,   /* REXT */
            0x00000003,   /* WDV */
            0x00000006,   /* QUSE */
            0x00000004,   /* QRST */
            0x00000008,   /* QSAFE */
            0x0000000C,   /* RDV */
            0x0000045F,   /* REFRESH */
            0x00000000,   /* BURST_REFRESH_NUM */
            0x00000003,   /* PDEX2WR */
            0x00000003,   /* PDEX2RD */
            0x00000003,   /* PCHG2PDEN */
            0x00000003,   /* ACT2PDEN */
            0x00000001,   /* AR2PDEN */
            0x0000000B,   /* RW2PDEN */
            0x00000015,   /* TXSR */
            0x00000003,   /* TCKE */
            0x00000008,   /* TFAW */
            0x00000004,   /* TRPAB */
            0x00000008,   /* TCLKSTABLE */
            0x00000002,   /* TCLKSTOP */
            0x00000270,   /* TREFBW */
            0x00000000,   /* QUSE_EXTRA */
            0x00000001,   /* FBIO_CFG6 */
            0x00000000,   /* ODT_WRITE */
            0x00000000,   /* ODT_READ */
            0x00000082,   /* FBIO_CFG5 */
            0xD05E0323,   /* CFG_DIG_DLL */
            0x007FD010,   /* DLL_XFORM_DQS */
            0x00000010,   /* DLL_XFORM_QUSE */
            0x00000000,   /* ZCAL_REF_CNT */
            0x0000000E,   /* ZCAL_WAIT_CNT */
            0x00000000,   /* AUTO_CAL_INTERVAL */
            0x00000000,   /* CFG_CLKTRIM_0 */
            0x00000000,   /* CFG_CLKTRIM_1 */
            0x00000000,   /* CFG_CLKTRIM_2 */
        }
    },
    {
		.rate = 166500,   /* SDRAM frequency : 1000mV emc core voltage*/
		.regs = {
            0x0000000A,   /* RC */
            0x00000017,   /* RFC */
            0x00000008,   /* RAS */
            0x00000004,   /* RP */
            0x00000007,   /* R2W */
            0x00000005,   /* W2R */
            0x00000003,   /* R2P */
            0x0000000D,   /* W2P */
            0x00000004,   /* RD_RCD */
            0x00000004,   /* WR_RCD */
            0x00000003,   /* RRD */
            0x00000003,   /* REXT */
            0x00000004,   /* WDV */
            0x00000007,   /* QUSE */
            0x00000005,   /* QRST */
            0x00000009,   /* QSAFE */
            0x0000000E,   /* RDV */
            0x0000045F,   /* REFRESH */
            0x00000000,   /* BURST_REFRESH_NUM */
            0x00000004,   /* PDEX2WR */
            0x00000004,   /* PDEX2RD */
            0x00000004,   /* PCHG2PDEN */
            0x00000004,   /* ACT2PDEN */
            0x00000002,   /* AR2PDEN */
            0x0000000D,   /* RW2PDEN */
            0x00000017,   /* TXSR */
            0x00000004,   /* TCKE */
            0x00000009,   /* TFAW */
            0x00000005,   /* TRPAB */
            0x00000009,   /* TCLKSTABLE */
            0x00000003,   /* TCLKSTOP */
            0x00000270,   /* TREFBW */
            0x00000000,   /* QUSE_EXTRA */
            0x00000001,   /* FBIO_CFG6 */
            0x00000000,   /* ODT_WRITE */
            0x00000000,   /* ODT_READ */
            0x00000082,   /* FBIO_CFG5 */
            0xD05E0323,   /* CFG_DIG_DLL */
            0x007FD010,   /* DLL_XFORM_DQS */
            0x00000010,   /* DLL_XFORM_QUSE */
            0x00000000,   /* ZCAL_REF_CNT */
            0x0000000E,   /* ZCAL_WAIT_CNT */
            0x00000000,   /* AUTO_CAL_INTERVAL */
            0x00000000,   /* CFG_CLKTRIM_0 */
            0x00000000,   /* CFG_CLKTRIM_1 */
            0x00000000,   /* CFG_CLKTRIM_2 */
        }
    },
    {
		.rate = 300000,   /* SDRAM frequency : 1100mV emc core voltage*/
		.regs = {
            0x00000012,   /* RC */
            0x00000027,   /* RFC */
            0x0000000D,   /* RAS */
            0x00000006,   /* RP */
            0x00000007,   /* R2W */
            0x00000005,   /* W2R */
            0x00000003,   /* R2P */
            0x0000000B,   /* W2P */
            0x00000006,   /* RD_RCD */
            0x00000006,   /* WR_RCD */
            0x00000003,   /* RRD */
            0x00000003,   /* REXT */
            0x00000003,   /* WDV */
            0x00000006,   /* QUSE */
            0x00000004,   /* QRST */
            0x00000009,   /* QSAFE */
            0x0000000D,   /* RDV */
            0x000008FF,   /* REFRESH */
            0x00000000,   /* BURST_REFRESH_NUM */
            0x00000004,   /* PDEX2WR */
            0x00000004,   /* PDEX2RD */
            0x00000006,   /* PCHG2PDEN */
            0x00000006,   /* ACT2PDEN */
            0x00000001,   /* AR2PDEN */
            0x0000000F,   /* RW2PDEN */
            0x0000002A,   /* TXSR */
            0x00000003,   /* TCKE */
            0x0000000F,   /* TFAW */
            0x00000007,   /* TRPAB */
            0x00000007,   /* TCLKSTABLE */
            0x00000002,   /* TCLKSTOP */
            0x000004E0,   /* TREFBW */
            0x00000000,   /* QUSE_EXTRA */
            0x00000002,   /* FBIO_CFG6 */
            0x00000000,   /* ODT_WRITE */
            0x00000000,   /* ODT_READ */
            0x00000282,   /* FBIO_CFG5 */
            0xE03A0303,   /* CFG_DIG_DLL */
            0x007FD010,   /* DLL_XFORM_DQS */
            0x00000010,   /* DLL_XFORM_QUSE */
            0x00000000,   /* ZCAL_REF_CNT */
            0x0000001B,   /* ZCAL_WAIT_CNT */
            0x00000000,   /* AUTO_CAL_INTERVAL */
            0x00000000,   /* CFG_CLKTRIM_0 */
            0x00000000,   /* CFG_CLKTRIM_1 */
            0x00000000,   /* CFG_CLKTRIM_2 */
        }
    },
    {
		.rate = 333000,   /* SDRAM frequency : 1100mV emc core voltage*/
		.regs = {
            0x00000014,   /* RC */
            0x0000002B,   /* RFC */
            0x0000000F,   /* RAS */
            0x00000007,   /* RP */
            0x00000008,   /* R2W */
            0x00000006,   /* W2R */
            0x00000004,   /* R2P */
            0x0000000D,   /* W2P */
            0x00000007,   /* RD_RCD */
            0x00000007,   /* WR_RCD */
            0x00000004,   /* RRD */
            0x00000004,   /* REXT */
            0x00000004,   /* WDV */
            0x00000007,   /* QUSE */
            0x00000005,   /* QRST */
            0x0000000A,   /* QSAFE */
            0x0000000F,   /* RDV */
            0x000008FF,   /* REFRESH */
            0x00000000,   /* BURST_REFRESH_NUM */
            0x00000005,   /* PDEX2WR */
            0x00000005,   /* PDEX2RD */
            0x00000007,   /* PCHG2PDEN */
            0x00000007,   /* ACT2PDEN */
            0x00000002,   /* AR2PDEN */
            0x00000011,   /* RW2PDEN */
            0x0000002E,   /* TXSR */
            0x00000004,   /* TCKE */
            0x00000011,   /* TFAW */
            0x00000008,   /* TRPAB */
            0x00000008,   /* TCLKSTABLE */
            0x00000003,   /* TCLKSTOP */
            0x000004E0,   /* TREFBW */
            0x00000000,   /* QUSE_EXTRA */
            0x00000002,   /* FBIO_CFG6 */
            0x00000000,   /* ODT_WRITE */
            0x00000000,   /* ODT_READ */
            0x00000282,   /* FBIO_CFG5 */
            0xE03A0303,   /* CFG_DIG_DLL */
            0x007FD010,   /* DLL_XFORM_DQS */
            0x00000010,   /* DLL_XFORM_QUSE */
            0x00000000,   /* ZCAL_REF_CNT */
            0x0000001B,   /* ZCAL_WAIT_CNT */
            0x00000000,   /* AUTO_CAL_INTERVAL */
            0x00000000,   /* CFG_CLKTRIM_0 */
            0x00000000,   /* CFG_CLKTRIM_1 */
            0x00000000,   /* CFG_CLKTRIM_2 */
        }
    } 
};

#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38) || LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,39) 
static const struct tegra_emc_chip adam_emc_chips[] = {
	{
		.description = "Shuttle memory",
		.mem_manufacturer_id = -1,
		.mem_revision_id1 = -1,
		.mem_revision_id2 = -1,
		.mem_pid = -1,
		.table = adam_emc_tables,
		.table_size = ARRAY_SIZE(adam_emc_tables),
	},
};
#endif

void __init adam_init_emc(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38) || LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,39) 
	tegra_init_emc(adam_emc_chips, ARRAY_SIZE(adam_emc_chips));
#else
	tegra_init_emc(adam_emc_tables, ARRAY_SIZE(adam_emc_tables));
#endif
}
