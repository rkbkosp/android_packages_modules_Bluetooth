/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// A2DP constants for LHDC codec
//

#ifndef A2DP_VENDOR_LHDCV5_CONSTANTS_H
#define A2DP_VENDOR_LHDCV5_CONSTANTS_H

/* LHDCV5 quality supporting bit rate */
#define A2DP_LHDCV5_QUALITY_MASK       (0xFF)
#define A2DP_LHDCV5_QUALITY_ABR        (0x09)     // Adaptive(Auto) bitrate mode
#define A2DP_LHDCV5_QUALITY_HIGH1      (0x08)     // 1000 kbps
#define A2DP_LHDCV5_QUALITY_HIGH       (0x07)     // 900  kbps
#define A2DP_LHDCV5_QUALITY_MID        (0x06)     // 480/520  kbps
#define A2DP_LHDCV5_QUALITY_LOW        (0x05)     // 380/390  kbps
#define A2DP_LHDCV5_QUALITY_LOW4       (0x04)     // 300/320  kbps
#define A2DP_LHDCV5_QUALITY_LOW3       (0x03)     // 240/260  kbps
#define A2DP_LHDCV5_QUALITY_LOW2       (0x02)     // 180/200  kbps
#define A2DP_LHDCV5_QUALITY_LOW1       (0x01)     // 160  kbps
#define A2DP_LHDCV5_QUALITY_LOW0       (0x00)     // 120/130  kbps

#define A2DP_LHDCV5_QUALITY_CMD_MASK   (0xC000)
#define A2DP_LHDCV5_QUALITY_MAGIC_NUM  (0x4000)   //PS: different from LHDCV3

////////////////////////////////////////////////////////////////////
// LHDCV5 codec info (capabilities) format:
// Total Length: A2DP_LHDCV5_CODEC_LEN + 1(losc)
//  --------------------------------------------------------------------------------------
//  H0   |    H1     |    H2     |  P0-P3   | P4-P5   |
//  losc | mediaType | codecType | vendorId | codecId |
//  --------------------------------------------------------------------------------------
//  P6[7:6]      | P6[5]   | P6[4] | P6[2] | P6[0]  |
//  FLenSelect   | 44.1KHz | 48KHz | 96KHz | 192KHz |
//  --------------------------------------------------------------------------------------
//  P7[7:6]     | P7[5:4]    | P7[2]   | P7[1]   |
//  MinBitRate  | MaxBitRate | 16 bits | 24 bits |
//  --------------------------------------------------------------------------------------
//  P8[7]       | P8[6]      | P8[4]     | P8[3:0]        |
//  FrmDur2p5ms | FrmDur10ms | FrmDur5ms | Version Number |
//  --------------------------------------------------------------------------------------
//  P9[7]       | P9[6]       | P9[5]         | P9[4]         |
//  Lossless    | Low Latency | Lossless24Bit | Lossless96KHz |
//  --------------------------------------------------------------------------------------
//  P10[7]      | P10[6]    | P10[5]    | P10[3:1] |
//  LosslessRaw | newMmBR   | br128kbps | exMBR    |
//  --------------------------------------------------------------------------------------
#define A2DP_LHDCV5_CODEC_LEN (13)

// P0-P3 Vendor ID: A2DP_LHDC_VENDOR_ID (0x0000053a)
// P4-P5 Vendor Specific Codec ID: A2DP_LHDCV5_CODEC_ID (0x4C35)

// P6[7:6] (lossy mode) Frame Length(Duration) Select from sink
#define A2DP_LHDCV5_FRAME_LEN_SELECT_MASK    (0xC0)
#define A2DP_LHDCV5_FRAME_LEN_SELECT_5MS     (0x00)
#define A2DP_LHDCV5_FRAME_LEN_SELECT_2P5MS   (0x40)
#define A2DP_LHDCV5_FRAME_LEN_SELECT_10MS    (0x80)
#define A2DP_LHDCV5_FRAME_LEN_SELECT_NS      (0xC0)

// P6[5:4] + P6[2] + P6[0] Sampling Frequency
#define A2DP_LHDCV5_SAMPLING_FREQ_MASK    (0x35)
#define A2DP_LHDCV5_SAMPLING_FREQ_44100   (0x20)
#define A2DP_LHDCV5_SAMPLING_FREQ_48000   (0x10)
#define A2DP_LHDCV5_SAMPLING_FREQ_96000   (0x04)
#define A2DP_LHDCV5_SAMPLING_FREQ_192000  (0x01)
#define A2DP_LHDCV5_SAMPLING_FREQ_NS      (0x00)

// P7[2:0] Bit depth
#define A2DP_LHDCV5_BIT_FMT_MASK  (0x07)
#define A2DP_LHDCV5_BIT_FMT_16    (0x04)
#define A2DP_LHDCV5_BIT_FMT_24    (0x02)
#define A2DP_LHDCV5_BIT_FMT_ERR   (0x01)
#define A2DP_LHDCV5_BIT_FMT_NS    (0x00)

// P7[5:4] Max Bit Rate Type
#define A2DP_LHDCV5_MAX_BIT_RATE_MASK   (0x30)
#define A2DP_LHDCV5_MAX_BIT_RATE_00     (0x00)
#define A2DP_LHDCV5_MAX_BIT_RATE_11     (0x30)
#define A2DP_LHDCV5_MAX_BIT_RATE_10     (0x20)
#define A2DP_LHDCV5_MAX_BIT_RATE_01     (0x10)
#define A2DP_LHDCV5_MAX_BIT_RATE_ALL    (0x30)

// P7[7:6] Min Bit Rate Type
#define A2DP_LHDCV5_MIN_BIT_RATE_MASK   (0xC0)
#define A2DP_LHDCV5_MIN_BIT_RATE_11     (0xC0)
#define A2DP_LHDCV5_MIN_BIT_RATE_10     (0x80)
#define A2DP_LHDCV5_MIN_BIT_RATE_01     (0x40)
#define A2DP_LHDCV5_MIN_BIT_RATE_00     (0x00)
#define A2DP_LHDCV5_MIN_BIT_RATE_ALL    (0xC0)

// P8[3:0] Codec SubVersion
#define A2DP_LHDCV5_VERSION_MASK    (0x0F)
#define A2DP_LHDCV5_VER_1           (0x01)
#define A2DP_LHDCV5_VER_NS          (0x00)

// P8[7:6] + P8[4] Frame Length(Duration) Type
#define A2DP_LHDCV5_FRAME_LEN_MASK  (0xD0)
#define A2DP_LHDCV5_FRAME_LEN_5MS   (0x10)
#define A2DP_LHDCV5_FRAME_LEN_10MS  (0x40)
#define A2DP_LHDCV5_FRAME_LEN_2P5MS (0x80)
#define A2DP_LHDCV5_FRAME_LEN_NS    (0x00)

// P9[4] Lossless96K
// P9[5] Lossless24Bit
// P9[6] LowLatency
// P9[7] Lossless48K
#define A2DP_LHDCV5_FEATURE_LLESS48K    (0x80)
#define A2DP_LHDCV5_FEATURE_LL          (0x40)
#define A2DP_LHDCV5_FEATURE_LLESS24BIT  (0x20)
#define A2DP_LHDCV5_FEATURE_LLESS96K    (0x10)

// P10[7] LosslessRaw48K
// P10[6] newMmBR
// P10[5] br128kbps (improved low bitrate)
#define A2DP_LHDCV5_FEATURE_LLESS_RAW   (0x80)
#define A2DP_LHDCV5_FEATURE_newMmBR     (0x40)
#define A2DP_LHDCV5_FEATURE_128KBPS     (0x20)
// P10[3:1] exMBR
#define A2DP_LHDCV5_EXMBR_MASK          (0x0E)
#define A2DP_LHDCV5_EXMBR_ALL           (0x0E)
#define A2DP_LHDCV5_EXMBR_DISABLE       (0x00)
#define A2DP_LHDCV5_EXMBR_001           (0x02)  //128 (b0010)
#define A2DP_LHDCV5_EXMBR_010           (0x04)  //160 (b0100)
#define A2DP_LHDCV5_EXMBR_011           (0x06)  //192 (b0110)
#define A2DP_LHDCV5_EXMBR_100           (0x08)  //256 (b1000)
#define A2DP_LHDCV5_EXMBR_101           (0x0A)  //320 (b1010)

////////////////////////////////////////////////////////////////////

#define A2DP_LHDCV5_UI_MAX_BITRATE_MASK        (0xFF0000)
#define A2DP_LHDCV5_UI_MAX_BITRATE_SHIFT_BIT   (16)
#define A2DP_LHDCV5_UI_MIN_BITRATE_MASK        (0xFF000000)
#define A2DP_LHDCV5_UI_MIN_BITRATE_SHIFT_BIT   (24)

////////////////////////////////////////////////////////////////////
//  attributes which not in codec info format
//    channel mode
//    channel separation mode
////////////////////////////////////////////////////////////////////
// channel mode:
#define A2DP_LHDCV5_CHANNEL_MODE_MASK   (0x07)
#define A2DP_LHDCV5_CHANNEL_MODE_MONO   (0x04)
#define A2DP_LHDCV5_CHANNEL_MODE_DUAL   (0x02)
#define A2DP_LHDCV5_CHANNEL_MODE_STEREO (0x01)
#define A2DP_LHDCV5_CHANNEL_MODE_NS     (0x00)
////////////////////////////////////////////////////////////////////

/************************************************
 * LHDC Feature Capabilities on A2DP specifics:
   * feature id:                          (1 byte)
   * target specific index:               (2 bits)
   * target bit index on a specific:      (decimal: 0~63)
************************************************/
#define A2DP_LHDCV5_VENDOR_FEATURE_MASK    (0xFF000000)
#define A2DP_LHDCV5_FEATURE_MAGIC_NUM      (0x5C000000)

// feature code:
#define LHDCV5_FEATURE_CODE_MASK     (0xFF)
#define LHDCV5_FEATURE_CODE_NA       (0x00)
#define LHDCV5_FEATURE_CODE_LL       (0x08)
#define LHDCV5_FEATURE_CODE_LLESS    (0x09)
#define LHDCV5_FEATURE_CODE_LLESS_RAW (0x0A)

// target specific index:
#define LHDCV5_FEATURE_ON_A2DP_SPECIFIC_1    (0x00)     //2-bit:00
#define LHDCV5_FEATURE_ON_A2DP_SPECIFIC_2    (0x40)     //2-bit:01
#define LHDCV5_FEATURE_ON_A2DP_SPECIFIC_3    (0x80)     //2-bit:10
#define LHDCV5_FEATURE_ON_A2DP_SPECIFIC_4    (0xC0)     //2-bit:11

// target bit index on the specific:
//  specific@1
#define LHDCV5_FEATURE_QM_SPEC_BIT_POS        (0x00)
//  specific@2
#define LHDCV5_FEATURE_LL_SPEC_BIT_POS        (0x00)
//  specific@3
#define LHDCV5_FEATURE_JAS_SPEC_BIT_POS       (0x00)
#define LHDCV5_FEATURE_AR_SPEC_BIT_POS        (0x01)
#define LHDCV5_FEATURE_META_SPEC_BIT_POS      (0x02)
#define LHDCV5_FEATURE_LLESS_SPEC_BIT_POS     (0x07)
#define LHDCV5_FEATURE_LLESS_RAW_SPEC_BIT_POS (0x08)
// Notice: the highest bit position is limited by A2DP_LHDC_FEATURE_MAGIC_NUM(0x4C000000)
//  ie., available range in a specific: int64[24:0]
#define LHDCV5_FEATURE_MAX_SPEC_BIT_POS       (0x19)

#define LHDC_SETUP_A2DP_SPEC(cfg, spec, has, value)  do{   \
  if( spec == LHDCV5_FEATURE_ON_A2DP_SPECIFIC_1 ) \
    (has) ? (cfg->codec_specific_1 |= value) : (cfg->codec_specific_1 &= ~value);   \
  if( spec == LHDCV5_FEATURE_ON_A2DP_SPECIFIC_2 ) \
    (has) ? (cfg->codec_specific_2 |= value) : (cfg->codec_specific_2 &= ~value);   \
  if( spec == LHDCV5_FEATURE_ON_A2DP_SPECIFIC_3 ) \
    (has) ? (cfg->codec_specific_3 |= value) : (cfg->codec_specific_3 &= ~value);   \
  if( spec == LHDCV5_FEATURE_ON_A2DP_SPECIFIC_4 ) \
    (has) ? (cfg->codec_specific_4 |= value) : (cfg->codec_specific_4 &= ~value);   \
} while(0)

#define LHDCV5_CHECK_IN_A2DP_SPEC(cfg, spec, value)  ({ \
  bool marco_ret = false; \
  do{   \
    if( spec == LHDCV5_FEATURE_ON_A2DP_SPECIFIC_1 ) \
      marco_ret = (cfg->codec_specific_1 & value);   \
    if( spec == LHDCV5_FEATURE_ON_A2DP_SPECIFIC_2 ) \
      marco_ret = (cfg->codec_specific_2 & value);   \
    if( spec == LHDCV5_FEATURE_ON_A2DP_SPECIFIC_3 ) \
      marco_ret = (cfg->codec_specific_3 & value);   \
    if( spec == LHDCV5_FEATURE_ON_A2DP_SPECIFIC_4 ) \
      marco_ret = (cfg->codec_specific_4 & value);   \
    } while(0);  \
  marco_ret;   \
})

#endif  // A2DP_VENDOR_LHDCV5_CONSTANTS_H
