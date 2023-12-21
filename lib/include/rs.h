/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (C) 2012-2014, 2018-2022 Intel Corporation
 * Copyright (C) 2017 Intel Deutschland GmbH
 */
#ifndef __iwl_fw_api_rs_h__
#define __iwl_fw_api_rs_h__

/*
 * rate_n_flags bit fields version 1
 *
 * The 32-bit value has different layouts in the low 8 bites depending on the
 * format. There are three formats, HT, VHT and legacy (11abg, with subformats
 * for CCK and OFDM).
 *
 * High-throughput (HT) rate format
 *	bit 8 is 1, bit 26 is 0, bit 9 is 0 (OFDM)
 * Very High-throughput (VHT) rate format
 *	bit 8 is 0, bit 26 is 1, bit 9 is 0 (OFDM)
 * Legacy OFDM rate format for bits 7:0
 *	bit 8 is 0, bit 26 is 0, bit 9 is 0 (OFDM)
 * Legacy CCK rate format for bits 7:0:
 *	bit 8 is 0, bit 26 is 0, bit 9 is 1 (CCK)
 */

/* Bit 8: (1) HT format, (0) legacy or VHT format */
#define RATE_MCS_HT_POS 8
#define RATE_MCS_HT_MSK_V1 BIT(RATE_MCS_HT_POS)

/* Bit 9: (1) CCK, (0) OFDM.  HT (bit 8) must be "0" for this bit to be valid */
#define RATE_MCS_CCK_POS_V1 9
#define RATE_MCS_CCK_MSK_V1 BIT(RATE_MCS_CCK_POS_V1)

/* Bit 26: (1) VHT format, (0) legacy format in bits 8:0 */
#define RATE_MCS_VHT_POS_V1 26
#define RATE_MCS_VHT_MSK_V1 BIT(RATE_MCS_VHT_POS_V1)

/*
 * High-throughput (HT) rate format for bits 7:0
 *
 *  2-0:  MCS rate base
 *        0)   6 Mbps
 *        1)  12 Mbps
 *        2)  18 Mbps
 *        3)  24 Mbps
 *        4)  36 Mbps
 *        5)  48 Mbps
 *        6)  54 Mbps
 *        7)  60 Mbps
 *  4-3:  0)  Single stream (SISO)
 *        1)  Dual stream (MIMO)
 *        2)  Triple stream (MIMO)
 *    5:  Value of 0x20 in bits 7:0 indicates 6 Mbps HT40 duplicate data
 *  (bits 7-6 are zero)
 *
 * Together the low 5 bits work out to the MCS index because we don't
 * support MCSes above 15/23, and 0-7 have one stream, 8-15 have two
 * streams and 16-23 have three streams. We could also support MCS 32
 * which is the duplicate 20 MHz MCS (bit 5 set, all others zero.)
 */
#define RATE_HT_MCS_RATE_CODE_MSK_V1 0x7
#define RATE_HT_MCS_NSS_POS_V1 3
#define RATE_HT_MCS_NSS_MSK_V1 (3 << RATE_HT_MCS_NSS_POS_V1)
#define RATE_HT_MCS_MIMO2_MSK BIT(RATE_HT_MCS_NSS_POS_V1)

/* Bit 10: (1) Use Green Field preamble */
#define RATE_HT_MCS_GF_POS 10
#define RATE_HT_MCS_GF_MSK (1 << RATE_HT_MCS_GF_POS)

#define RATE_HT_MCS_INDEX_MSK_V1 0x3f

/*
 * Very High-throughput (VHT) rate format for bits 7:0
 *
 *  3-0:  VHT MCS (0-9)
 *  5-4:  number of streams - 1:
 *        0)  Single stream (SISO)
 *        1)  Dual stream (MIMO)
 *        2)  Triple stream (MIMO)
 */

/* Bit 4-5: (0) SISO, (1) MIMO2 (2) MIMO3 */
#define RATE_VHT_MCS_RATE_CODE_MSK 0xf

/*
 * Legacy OFDM rate format for bits 7:0
 *
 *  3-0:  0xD)   6 Mbps
 *        0xF)   9 Mbps
 *        0x5)  12 Mbps
 *        0x7)  18 Mbps
 *        0x9)  24 Mbps
 *        0xB)  36 Mbps
 *        0x1)  48 Mbps
 *        0x3)  54 Mbps
 * (bits 7-4 are 0)
 *
 * Legacy CCK rate format for bits 7:0:
 * bit 8 is 0, bit 26 is 0, bit 9 is 1 (CCK):
 *
 *  6-0:   10)  1 Mbps
 *         20)  2 Mbps
 *         55)  5.5 Mbps
 *        110)  11 Mbps
 * (bit 7 is 0)
 */
#define RATE_LEGACY_RATE_MSK_V1 0xff

/* Bit 10 - OFDM HE */
#define RATE_MCS_HE_POS_V1 10
#define RATE_MCS_HE_MSK_V1 BIT(RATE_MCS_HE_POS_V1)

/*
 * Bit 11-12: (0) 20MHz, (1) 40MHz, (2) 80MHz, (3) 160MHz
 * 0 and 1 are valid for HT and VHT, 2 and 3 only for VHT
 */
#define RATE_MCS_CHAN_WIDTH_POS 11
#define RATE_MCS_CHAN_WIDTH_MSK_V1 (3 << RATE_MCS_CHAN_WIDTH_POS)

/* Bit 13: (1) Short guard interval (0.4 usec), (0) normal GI (0.8 usec) */
#define RATE_MCS_SGI_POS_V1 13
#define RATE_MCS_SGI_MSK_V1 BIT(RATE_MCS_SGI_POS_V1)

/* Bit 14-16: Antenna selection (1) Ant A, (2) Ant B, (4) Ant C */
#define RATE_MCS_ANT_POS 14
#define RATE_MCS_ANT_A_MSK (1 << RATE_MCS_ANT_POS)
#define RATE_MCS_ANT_B_MSK (2 << RATE_MCS_ANT_POS)
#define RATE_MCS_ANT_AB_MSK (RATE_MCS_ANT_A_MSK | \
							 RATE_MCS_ANT_B_MSK)
#define RATE_MCS_ANT_MSK RATE_MCS_ANT_AB_MSK

/* Bit 17: (0) SS, (1) SS*2 */
#define RATE_MCS_STBC_POS 17
#define RATE_MCS_STBC_MSK BIT(RATE_MCS_STBC_POS)

/* Bit 18: OFDM-HE dual carrier mode */
#define RATE_HE_DUAL_CARRIER_MODE 18
#define RATE_HE_DUAL_CARRIER_MODE_MSK BIT(RATE_HE_DUAL_CARRIER_MODE)

/* Bit 19: (0) Beamforming is off, (1) Beamforming is on */
#define RATE_MCS_BF_POS 19
#define RATE_MCS_BF_MSK (1 << RATE_MCS_BF_POS)

/*
 * Bit 20-21: HE LTF type and guard interval
 * HE (ext) SU:
 *	0			1xLTF+0.8us
 *	1			2xLTF+0.8us
 *	2			2xLTF+1.6us
 *	3 & SGI (bit 13) clear	4xLTF+3.2us
 *	3 & SGI (bit 13) set	4xLTF+0.8us
 * HE MU:
 *	0			4xLTF+0.8us
 *	1			2xLTF+0.8us
 *	2			2xLTF+1.6us
 *	3			4xLTF+3.2us
 * HE-EHT TRIG:
 *	0			1xLTF+1.6us
 *	1			2xLTF+1.6us
 *	2			4xLTF+3.2us
 *	3			(does not occur)
 * EHT MU:
 *	0			2xLTF+0.8us
 *	1			2xLTF+1.6us
 *	2			4xLTF+0.8us
 *	3			4xLTF+3.2us
 */
#define RATE_MCS_HE_GI_LTF_POS 20
#define RATE_MCS_HE_GI_LTF_MSK_V1 (3 << RATE_MCS_HE_GI_LTF_POS)

/* Bit 22-23: HE type. (0) SU, (1) SU_EXT, (2) MU, (3) trigger based */
#define RATE_MCS_HE_TYPE_POS_V1 22
#define RATE_MCS_HE_TYPE_SU_V1 (0 << RATE_MCS_HE_TYPE_POS_V1)
#define RATE_MCS_HE_TYPE_EXT_SU_V1 BIT(RATE_MCS_HE_TYPE_POS_V1)
#define RATE_MCS_HE_TYPE_MU_V1 (2 << RATE_MCS_HE_TYPE_POS_V1)
#define RATE_MCS_HE_TYPE_TRIG_V1 (3 << RATE_MCS_HE_TYPE_POS_V1)
#define RATE_MCS_HE_TYPE_MSK_V1 (3 << RATE_MCS_HE_TYPE_POS_V1)

/* Bit 24-25: (0) 20MHz (no dup), (1) 2x20MHz, (2) 4x20MHz, 3 8x20MHz */
#define RATE_MCS_DUP_POS_V1 24
#define RATE_MCS_DUP_MSK_V1 (3 << RATE_MCS_DUP_POS_V1)

/* Bit 27: (1) LDPC enabled, (0) LDPC disabled */
#define RATE_MCS_LDPC_POS_V1 27
#define RATE_MCS_LDPC_MSK_V1 BIT(RATE_MCS_LDPC_POS_V1)

/* Bit 28: (1) 106-tone RX (8 MHz RU), (0) normal bandwidth */
#define RATE_MCS_HE_106T_POS_V1 28
#define RATE_MCS_HE_106T_MSK_V1 BIT(RATE_MCS_HE_106T_POS_V1)

/* Bit 30-31: (1) RTS, (2) CTS */
#define RATE_MCS_RTS_REQUIRED_POS (30)
#define RATE_MCS_RTS_REQUIRED_MSK (0x1 << RATE_MCS_RTS_REQUIRED_POS)

#define RATE_MCS_CTS_REQUIRED_POS (31)
#define RATE_MCS_CTS_REQUIRED_MSK (0x1 << RATE_MCS_CTS_REQUIRED_POS)

/* rate_n_flags bit field version 2
 *
 * The 32-bit value has different layouts in the low 8 bits depending on the
 * format. There are three formats, HT, VHT and legacy (11abg, with subformats
 * for CCK and OFDM).
 *
 */

/* Bits 10-8: rate format
 * (0) Legacy CCK (1) Legacy OFDM (2) High-throughput (HT)
 * (3) Very High-throughput (VHT) (4) High-efficiency (HE)
 * (5) Extremely High-throughput (EHT)
 */
#define RATE_MCS_MOD_TYPE_POS 8
#define RATE_MCS_MOD_TYPE_MSK (0x7 << RATE_MCS_MOD_TYPE_POS)
#define RATE_MCS_CCK_MSK (0 << RATE_MCS_MOD_TYPE_POS)
#define RATE_MCS_LEGACY_OFDM_MSK (1 << RATE_MCS_MOD_TYPE_POS)
#define RATE_MCS_HT_MSK (2 << RATE_MCS_MOD_TYPE_POS)
#define RATE_MCS_VHT_MSK (3 << RATE_MCS_MOD_TYPE_POS)
#define RATE_MCS_HE_MSK (4 << RATE_MCS_MOD_TYPE_POS)
#define RATE_MCS_EHT_MSK (5 << RATE_MCS_MOD_TYPE_POS)

/*
 * Legacy CCK rate format for bits 0:3:
 *
 * (0) 0xa - 1 Mbps
 * (1) 0x14 - 2 Mbps
 * (2) 0x37 - 5.5 Mbps
 * (3) 0x6e - 11 nbps
 *
 * Legacy OFDM rate format for bis 3:0:
 *
 * (0) 6 Mbps
 * (1) 9 Mbps
 * (2) 12 Mbps
 * (3) 18 Mbps
 * (4) 24 Mbps
 * (5) 36 Mbps
 * (6) 48 Mbps
 * (7) 54 Mbps
 *
 */
#define RATE_LEGACY_RATE_MSK 0x7

/*
 * HT, VHT, HE, EHT rate format for bits 3:0
 * 3-0: MCS
 *
 */
#define RATE_HT_MCS_CODE_MSK 0x7
#define RATE_MCS_NSS_POS 4
#define RATE_MCS_NSS_MSK (1 << RATE_MCS_NSS_POS)
#define RATE_MCS_CODE_MSK 0xf
#define RATE_HT_MCS_INDEX(r) ((((r)&RATE_MCS_NSS_MSK) >> 1) | \
							  ((r)&RATE_HT_MCS_CODE_MSK))

/* Bits 7-5: reserved */

/*
 * Bits 13-11: (0) 20MHz, (1) 40MHz, (2) 80MHz, (3) 160MHz, (4) 320MHz
 */
#define RATE_MCS_CHAN_WIDTH_MSK (0x7 << RATE_MCS_CHAN_WIDTH_POS)
#define RATE_MCS_CHAN_WIDTH_20_VAL 0
#define RATE_MCS_CHAN_WIDTH_20 (RATE_MCS_CHAN_WIDTH_20_VAL << RATE_MCS_CHAN_WIDTH_POS)
#define RATE_MCS_CHAN_WIDTH_40_VAL 1
#define RATE_MCS_CHAN_WIDTH_40 (RATE_MCS_CHAN_WIDTH_40_VAL << RATE_MCS_CHAN_WIDTH_POS)
#define RATE_MCS_CHAN_WIDTH_80_VAL 2
#define RATE_MCS_CHAN_WIDTH_80 (RATE_MCS_CHAN_WIDTH_80_VAL << RATE_MCS_CHAN_WIDTH_POS)
#define RATE_MCS_CHAN_WIDTH_160_VAL 3
#define RATE_MCS_CHAN_WIDTH_160 (RATE_MCS_CHAN_WIDTH_160_VAL << RATE_MCS_CHAN_WIDTH_POS)
#define RATE_MCS_CHAN_WIDTH_320_VAL 4
#define RATE_MCS_CHAN_WIDTH_320 (RATE_MCS_CHAN_WIDTH_320_VAL << RATE_MCS_CHAN_WIDTH_POS)

/* Bit 15-14: Antenna selection:
 * Bit 14: Ant A active
 * Bit 15: Ant B active
 *
 * All relevant definitions are same as in v1
 */

/* Bit 16 (1) LDPC enables, (0) LDPC disabled */
#define RATE_MCS_LDPC_POS 16
#define RATE_MCS_LDPC_MSK (1 << RATE_MCS_LDPC_POS)

/* Bit 17: (0) SS, (1) SS*2 (same as v1) */

/* Bit 18: OFDM-HE dual carrier mode (same as v1) */

/* Bit 19: (0) Beamforming is off, (1) Beamforming is on (same as v1) */

/*
 * Bit 22-20: HE LTF type and guard interval
 * CCK:
 *	0			long preamble
 *	1			short preamble
 * HT/VHT:
 *	0			0.8us
 *	1			0.4us
 * HE (ext) SU:
 *	0			1xLTF+0.8us
 *	1			2xLTF+0.8us
 *	2			2xLTF+1.6us
 *	3			4xLTF+3.2us
 *	4			4xLTF+0.8us
 * HE MU:
 *	0			4xLTF+0.8us
 *	1			2xLTF+0.8us
 *	2			2xLTF+1.6us
 *	3			4xLTF+3.2us
 * HE TRIG:
 *	0			1xLTF+1.6us
 *	1			2xLTF+1.6us
 *	2			4xLTF+3.2us
 * */
#define RATE_MCS_HE_GI_LTF_MSK (0x7 << RATE_MCS_HE_GI_LTF_POS)
#define RATE_MCS_SGI_POS RATE_MCS_HE_GI_LTF_POS
#define RATE_MCS_SGI_MSK (1 << RATE_MCS_SGI_POS)
#define RATE_MCS_HE_SU_4_LTF 3
#define RATE_MCS_HE_SU_4_LTF_08_GI 4

/* Bit 24-23: HE type. (0) SU, (1) SU_EXT, (2) MU, (3) trigger based */
#define RATE_MCS_HE_TYPE_POS 23
#define RATE_MCS_HE_TYPE_SU (0 << RATE_MCS_HE_TYPE_POS)
#define RATE_MCS_HE_TYPE_EXT_SU (1 << RATE_MCS_HE_TYPE_POS)
#define RATE_MCS_HE_TYPE_MU (2 << RATE_MCS_HE_TYPE_POS)
#define RATE_MCS_HE_TYPE_TRIG (3 << RATE_MCS_HE_TYPE_POS)
#define RATE_MCS_HE_TYPE_MSK (3 << RATE_MCS_HE_TYPE_POS)

/* Bit 25: duplicate channel enabled
 *
 * if this bit is set, duplicate is according to BW (bits 11-13):
 *
 * CCK:  2x 20MHz
 * OFDM Legacy: N x 20Mhz, (N = BW \ 2 , either 2, 4, 8, 16)
 * EHT: 2 x BW/2, (80 - 2x40, 160 - 2x80, 320 - 2x160)
 * */
#define RATE_MCS_DUP_POS 25
#define RATE_MCS_DUP_MSK (1 << RATE_MCS_DUP_POS)

/* Bit 26: (1) 106-tone RX (8 MHz RU), (0) normal bandwidth */
#define RATE_MCS_HE_106T_POS 26
#define RATE_MCS_HE_106T_MSK (1 << RATE_MCS_HE_106T_POS)

/* Bit 27: EHT extra LTF:
 * instead of 1 LTF for SISO use 2 LTFs,
 * instead of 2 LTFs for NSTS=2 use 4 LTFs*/
#define RATE_MCS_EHT_EXTRA_LTF_POS 27
#define RATE_MCS_EHT_EXTRA_LTF_MSK (1 << RATE_MCS_EHT_EXTRA_LTF_POS)

/* Bit 31-28: reserved */

#endif /* __iwl_fw_api_rs_h__ */
