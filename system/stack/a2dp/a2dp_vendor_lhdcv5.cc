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

/******************************************************************************
 *
 *  Utility functions to help build and parse the LHDCV5 Codec Information
 *  Element and Media Payload.
 *
 ******************************************************************************/


#define LOG_TAG "a2dp_vendor_lhdcv5"

#include "a2dp_vendor_lhdcv5.h"

#include <bluetooth/log.h>
#include <string.h>

#include "a2dp_vendor.h"
#include "a2dp_vendor_lhdcv5_encoder.h"

#include "btif/include/btif_av_co.h"
#include "internal_include/bt_trace.h"

#include "osi/include/osi.h"
#include "stack/include/bt_hdr.h"


using namespace bluetooth;


// data type for the LHDC Codec Information Element
typedef struct {
  uint32_t vendorId;                                    /* Vendor ID */
  uint16_t codecId;                                     /* Codec ID */
  uint8_t sampleRate;                                   /* Sampling Frequency Type */
  uint8_t bitsPerSample;                                /* Bits Per Sample Type */
  uint8_t channelMode;                                  /* Channel Mode */
  uint8_t version;                                      /* Codec SubVersion Number */
  uint8_t frameLenType;                                 /* Frame Length Type */
  uint8_t frameLenSelect;                               /* Frame Length Select */
  uint8_t maxTargetBitrate;                             /* Max Target Bit Rate Type */
  uint8_t minTargetBitrate;                             /* Min Target Bit Rate Type */
  uint8_t exMBR;                                        /* FeatureSupported: Extended MaxBitrate */
  bool hasFeatureLL;                                    /* FeatureSupported: Low Latency */
  bool hasFeatureLLESS;                                 /* FeatureSupported: Lossless enable/disable (standard 48 KHz) */
  bool hasFeatureLLESS24Bit;                            /* Lossless extended configurable: 24 bit-per-sample */
  bool hasFeatureLLESS96K;                              /* Lossless extended configurable: 96 KHz */
  bool hasFeatureLLESSRaw;                              /* FeatureSupported: Lossless Raw mode (standard 48 KHz) */
  bool hasFeatureNewMmBR;                               /* FeatureSupported: new max/min bitrate settings */
  bool hasFeaturebr128kbps;                             /* FeatureSupported: improved quality bitrate (128kbps) */
} tA2DP_LHDCV5_CIE;

// source capabilities
static const tA2DP_LHDCV5_CIE a2dp_lhdcv5_source_caps = {
    A2DP_LHDC_VENDOR_ID,  // vendorId
    A2DP_LHDCV5_CODEC_ID, // codecId
    // Sampling Frequency
    (A2DP_LHDCV5_SAMPLING_FREQ_44100 | A2DP_LHDCV5_SAMPLING_FREQ_48000),
    // Bits Per Sample
    (A2DP_LHDCV5_BIT_FMT_16 | A2DP_LHDCV5_BIT_FMT_24),
    // Channel Mode
    A2DP_LHDCV5_CHANNEL_MODE_STEREO,
    // Codec SubVersion Number
    A2DP_LHDCV5_VER_1,
    // Encoded Frame Length type
    A2DP_LHDCV5_FRAME_LEN_5MS,
    // Frame Length select
    A2DP_LHDCV5_FRAME_LEN_SELECT_5MS,
    // Max Target Bit Rate Type
    A2DP_LHDCV5_MAX_BIT_RATE_00,
    // Min Target Bit Rate Type
    A2DP_LHDCV5_MIN_BIT_RATE_00,
    // Extended MaxBitrate mode
    A2DP_LHDCV5_EXMBR_DISABLE,
    // FeatureSupported: Low Latency
    true,
    // FeatureSupported: Lossless (standard 48 KHz)
    false,
    // Lossless extended configurable: 24 bit-per-sample
    false,
    // Lossless extended configurable: 96 KHz
    false,
    // FeatureSupported: Lossless Raw mode (standard 48 KHz)
    false,
    // newMmBR
    false,
    // br128kbps
    false,
};

// source set_configuration
static const tA2DP_LHDCV5_CIE a2dp_lhdcv5_source_default_caps = {
    A2DP_LHDC_VENDOR_ID,  // vendorId
    A2DP_LHDCV5_CODEC_ID, // codecId
    // Sampling Frequency
    A2DP_LHDCV5_SAMPLING_FREQ_48000,
    // Bits Per Sample
    A2DP_LHDCV5_BIT_FMT_24,
    // Channel Mode
    A2DP_LHDCV5_CHANNEL_MODE_STEREO,
    // Codec Version Number
    A2DP_LHDCV5_VER_1,
    // Encoded Frame Length
    A2DP_LHDCV5_FRAME_LEN_5MS,
    // Frame Length select
    A2DP_LHDCV5_FRAME_LEN_SELECT_5MS,
    // Max Target Bit Rate Type
    A2DP_LHDCV5_MAX_BIT_RATE_00,
    // Min Target Bit Rate Type
    A2DP_LHDCV5_MIN_BIT_RATE_00,
    // Extended MaxBitrate mode
    A2DP_LHDCV5_EXMBR_DISABLE,
    // FeatureSupported: Low Latency
    true,
    // FeatureSupported: Lossless (standard 48 KHz)
    false,
    // Lossless extended configurable: 24 bit-per-sample
    false,
    // Lossless extended configurable: 96 KHz
    false,
    // FeatureSupported: Lossless Raw mode (standard 48 KHz)
    false,
    // newMmBR
    false,
    // br128kbps
    false,
};

// sink capabilities
static const tA2DP_LHDCV5_CIE a2dp_lhdcv5_sink_caps = {
    A2DP_LHDC_VENDOR_ID,  // vendorId
    A2DP_LHDCV5_CODEC_ID, // codecId
    // Sampling Frequency
    (A2DP_LHDCV5_SAMPLING_FREQ_44100 | A2DP_LHDCV5_SAMPLING_FREQ_48000 |
        A2DP_LHDCV5_SAMPLING_FREQ_96000 | A2DP_LHDCV5_SAMPLING_FREQ_192000),
    // Bits Per Sample
    (A2DP_LHDCV5_BIT_FMT_16 | A2DP_LHDCV5_BIT_FMT_24),
    // Channel Mode
    A2DP_LHDCV5_CHANNEL_MODE_STEREO,
    // Codec Version Number
    A2DP_LHDCV5_VER_1,
    // Encoded Frame Length
    (A2DP_LHDCV5_FRAME_LEN_2P5MS | A2DP_LHDCV5_FRAME_LEN_5MS | A2DP_LHDCV5_FRAME_LEN_10MS),
    // Frame Length select
    (A2DP_LHDCV5_FRAME_LEN_SELECT_5MS),
    // Max Target Bit Rate Type
    A2DP_LHDCV5_MAX_BIT_RATE_00,
    // Min Target Bit Rate Type
    A2DP_LHDCV5_MIN_BIT_RATE_10,
    // Extended MaxBitrate mode
    A2DP_LHDCV5_EXMBR_DISABLE,
    // FeatureSupported: Low Latency
    true,
    // FeatureSupported: Lossless (standard 48 KHz 16 Bits)
    false,
    // Lossless extended configurable: 24 bit-per-sample
    false,
    // Lossless extended configurable: 96 KHz
    false,
    // FeatureSupported: Lossless Raw mode (standard 48 KHz  16 Bits)
    false,
    // newMmBR
    true,
    // br128kbps
    false,
};

// sink set_configuration
UNUSED_ATTR static const tA2DP_LHDCV5_CIE a2dp_lhdcv5_sink_default_caps = {
    A2DP_LHDC_VENDOR_ID,  // vendorId
    A2DP_LHDCV5_CODEC_ID, // codecId
    // Sampling Frequency
    A2DP_LHDCV5_SAMPLING_FREQ_48000,
    // Bits Per Sample
    A2DP_LHDCV5_BIT_FMT_24,
    // Channel Mode
    A2DP_LHDCV5_CHANNEL_MODE_STEREO,
    // Codec Version Number
    A2DP_LHDCV5_VER_1,
    // Encoded Frame Length
    (A2DP_LHDCV5_FRAME_LEN_2P5MS | A2DP_LHDCV5_FRAME_LEN_5MS | A2DP_LHDCV5_FRAME_LEN_10MS),
    // Frame Length select
    (A2DP_LHDCV5_FRAME_LEN_SELECT_5MS),
    // Max Target Bit Rate Type
    A2DP_LHDCV5_MAX_BIT_RATE_00,
    // Min Target Bit Rate Type
    A2DP_LHDCV5_MIN_BIT_RATE_10,
    // Extended MaxBitrate mode
    A2DP_LHDCV5_EXMBR_DISABLE,
    // FeatureSupported: Low Latency
    false,
    // FeatureSupported: Lossless (standard 48 KHz)
    false,
    // Lossless extended configurable: 24 bit-per-sample
    false,
    // Lossless extended configurable: 96 KHz
    false,
    // FeatureSupported: Lossless Raw mode (standard 48 KHz)
    false,
    // newMmBR
    true,
    // br128kbps
    false,
};

//
// Utilities for LHDC configuration on A2DP specifics - START
//
typedef struct {
  btav_a2dp_codec_config_t *_codec_config_;
  btav_a2dp_codec_config_t *_codec_local_capability_;
  btav_a2dp_codec_config_t *_codec_selectable_capability_;
  btav_a2dp_codec_config_t *_codec_user_config_;
  btav_a2dp_codec_config_t *_codec_audio_config_;
}tA2DP_CODEC_CONFIGS_PACK;

typedef struct {
  uint8_t   featureCode;  /* code of LHDC features */
  uint8_t   inSpecBank;   /* target specific to store the feature flag */
  uint8_t   bitPos;       /* the bit index(0~63) of the specific(int64_t) that bit store */
  int64_t   value;        /* real value of the bit position written to the target specific */
}tA2DP_LHDC_FEATURE_POS;

// default settings of LHDC features configuration on specifics
// info of feature: Low Latency
static const tA2DP_LHDC_FEATURE_POS a2dp_lhdcv5_source_spec_LL = {
    LHDCV5_FEATURE_CODE_LL,
    LHDCV5_FEATURE_ON_A2DP_SPECIFIC_2,
    LHDCV5_FEATURE_LL_SPEC_BIT_POS,
    (0x1ULL << LHDCV5_FEATURE_LL_SPEC_BIT_POS),
};

// info of feature: LossLess
static const tA2DP_LHDC_FEATURE_POS a2dp_lhdcv5_source_spec_LLESS = {
    LHDCV5_FEATURE_CODE_LLESS,
    LHDCV5_FEATURE_ON_A2DP_SPECIFIC_3,
    LHDCV5_FEATURE_LLESS_SPEC_BIT_POS,
    (0x1ULL << LHDCV5_FEATURE_LLESS_SPEC_BIT_POS),
};

// info of feature: LossLess Raw
static const tA2DP_LHDC_FEATURE_POS a2dp_lhdcv5_source_spec_LLESS_RAW = {
    LHDCV5_FEATURE_CODE_LLESS_RAW,
    LHDCV5_FEATURE_ON_A2DP_SPECIFIC_3,
    LHDCV5_FEATURE_LLESS_RAW_SPEC_BIT_POS,
    (0x1ULL << LHDCV5_FEATURE_LLESS_RAW_SPEC_BIT_POS),
};


UNUSED_ATTR static const tA2DP_LHDC_FEATURE_POS a2dp_lhdcv5_source_spec_all[] = {
    a2dp_lhdcv5_source_spec_LL,
    a2dp_lhdcv5_source_spec_LLESS,
    a2dp_lhdcv5_source_spec_LLESS_RAW,
};


static std::string lhdcV5_sampleRate_toString(uint8_t value) {
  switch((int)value)
  {
    case A2DP_LHDCV5_SAMPLING_FREQ_44100:
      return "44.1KHz";
    case A2DP_LHDCV5_SAMPLING_FREQ_48000:
      return "48KHz";
    case A2DP_LHDCV5_SAMPLING_FREQ_96000:
      return "96KHz";
    case A2DP_LHDCV5_SAMPLING_FREQ_192000:
      return "192KHz";
    default:
      return "Unknown Sample Rate";
  }
}

static std::string lhdcV5_bitPerSample_toString(uint8_t value) {
  switch((int)value)
  {
    case A2DP_LHDCV5_BIT_FMT_16:
      return "16";
    case A2DP_LHDCV5_BIT_FMT_24:
      return "24";
    default:
      return "Unknown Bit Per Sample";
  }
}

static std::string lhdcV5_frameLenType_toString(uint8_t value) {
  switch((int)value)
  {
    case A2DP_LHDCV5_FRAME_LEN_2P5MS:
      return "2.5ms";
    case A2DP_LHDCV5_FRAME_LEN_5MS:
      return "5ms";
    case A2DP_LHDCV5_FRAME_LEN_10MS:
      return "10ms";
    default:
      return "Unknown frame length type";
  }
}

static std::string lhdcV5_frameLenSelect_toString(uint8_t value) {
  switch((int)value)
  {
    case A2DP_LHDCV5_FRAME_LEN_SELECT_2P5MS:
      return "2.5ms";
    case A2DP_LHDCV5_FRAME_LEN_SELECT_5MS:
      return "5ms";
    case A2DP_LHDCV5_FRAME_LEN_SELECT_10MS:
      return "10ms";
    default:
      return "Unknown frame length select";
  }
}

static std::string lhdcV5_MaxTargetBitRate_toString(uint8_t value) {
  switch((int)value)
  {
    case A2DP_LHDCV5_MAX_BIT_RATE_11:
      return "MaxBR_11";
    case A2DP_LHDCV5_MAX_BIT_RATE_10:
      return "MaxBR_10";
    case A2DP_LHDCV5_MAX_BIT_RATE_01:
      return "MaxBR_01";
    case A2DP_LHDCV5_MAX_BIT_RATE_00:
      return "MaxBR_00";
    default:
      return "Unknown MaxBitRate";
  }
}

static std::string lhdcV5_MinTargetBitRate_toString(uint8_t value) {
  switch((int)value)
  {
    case A2DP_LHDCV5_MIN_BIT_RATE_11:
      return "minBR_11";
    case A2DP_LHDCV5_MIN_BIT_RATE_10:
      return "minBR_10";
    case A2DP_LHDCV5_MIN_BIT_RATE_01:
      return "minBR_01";
    case A2DP_LHDCV5_MIN_BIT_RATE_00:
      return "minBR_00";
    default:
      return "Unknown MinBitRate";
  }
}

static std::string lhdcV5_exMBR_toString(uint8_t value) {
  switch((int)value)
  {
    case A2DP_LHDCV5_EXMBR_DISABLE:
      return "exMBR_None";
    case A2DP_LHDCV5_EXMBR_001:
      return "exMBR_001";
    case A2DP_LHDCV5_EXMBR_010:
      return "exMBR_010";
    case A2DP_LHDCV5_EXMBR_011:
      return "exMBR_011";
    case A2DP_LHDCV5_EXMBR_100:
      return "exMBR_100";
    case A2DP_LHDCV5_EXMBR_101:
      return "exMBR_101";
    default:
      return "Unknown exMBR";
  }
}

static std::string lhdcV5_quality_index_toString(uint8_t value) {
  switch((int)value)
  {
    case A2DP_LHDCV5_QUALITY_ABR:
      return "AUTO";
    case A2DP_LHDCV5_QUALITY_HIGH1:
      return "HIGH1 (1000 Kbps)";
    case A2DP_LHDCV5_QUALITY_HIGH:
      return "HIGH (900 Kbps)";
    case A2DP_LHDCV5_QUALITY_MID:
      return "MID (480/520 Kbps)";
    case A2DP_LHDCV5_QUALITY_LOW:
      return "LOW (380/390 Kbps)";
    case A2DP_LHDCV5_QUALITY_LOW4:
      return "LOW4 (300/320 Kbps)";
    case A2DP_LHDCV5_QUALITY_LOW3:
      return "LOW3 (240/260 Kbps)";
    case A2DP_LHDCV5_QUALITY_LOW2:
      return "LOW2 (180/200 Kbps)";
    case A2DP_LHDCV5_QUALITY_LOW1:
      return "LOW1 (160 Kbps)";
    case A2DP_LHDCV5_QUALITY_LOW0:
      return "LOW0 (120/130 Kbps)";
    default:
    return "Unknown Bit Rate Mode";
  }
}

static bool lhdcV5_get_maxBitrate_index(uint8_t* index, tA2DP_LHDCV5_CIE* config_cie) {
  if (config_cie->sampleRate == A2DP_LHDCV5_SAMPLING_FREQ_44100 ||
      config_cie->sampleRate == A2DP_LHDCV5_SAMPLING_FREQ_48000) {
    // for sample rate: 44.1KHz or 48KHz:
    if (config_cie->exMBR != A2DP_LHDCV5_EXMBR_DISABLE &&
        config_cie->hasFeatureNewMmBR == true) {
      // extended maxBitrate specification
      switch(config_cie->exMBR)
      {
        case A2DP_LHDCV5_EXMBR_001:
        {
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW4;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW0;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
            *index = A2DP_LHDCV5_QUALITY_LOW0;
          break;
        }
        case A2DP_LHDCV5_EXMBR_010:
        {
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW4;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW1;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
            *index = A2DP_LHDCV5_QUALITY_LOW1;
          break;
        }
        case A2DP_LHDCV5_EXMBR_011:
        {
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW2;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
            *index = A2DP_LHDCV5_QUALITY_LOW2;
          break;
        }
        case A2DP_LHDCV5_EXMBR_100:
        {
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW3;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
            *index = A2DP_LHDCV5_QUALITY_LOW3;
          break;
        }
        case A2DP_LHDCV5_EXMBR_101:
        {
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW4;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
            *index = A2DP_LHDCV5_QUALITY_LOW4;
          break;
        }
        default:
        {
          return false;
        }
      }
    } else {
      // regular maxBitrate specification
      switch(config_cie->maxTargetBitrate)
      {
        case A2DP_LHDCV5_MAX_BIT_RATE_01:
        {
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
            *index = A2DP_LHDCV5_QUALITY_MID;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
            *index = A2DP_LHDCV5_QUALITY_LOW;
          break;
        }
        case A2DP_LHDCV5_MAX_BIT_RATE_10:
        {
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
            *index = A2DP_LHDCV5_QUALITY_HIGH;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
            *index = A2DP_LHDCV5_QUALITY_MID;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
            *index = A2DP_LHDCV5_QUALITY_MID;
          break;
        }
        case A2DP_LHDCV5_MAX_BIT_RATE_11:
        {
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
            *index = A2DP_LHDCV5_QUALITY_HIGH;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
            *index = A2DP_LHDCV5_QUALITY_HIGH;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
            *index = A2DP_LHDCV5_QUALITY_MID;
          break;
        }
        case A2DP_LHDCV5_MAX_BIT_RATE_00:
        {
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
            *index = A2DP_LHDCV5_QUALITY_HIGH;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
            *index = A2DP_LHDCV5_QUALITY_HIGH;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
            *index = A2DP_LHDCV5_QUALITY_MID;
          break;
        }
        default:
        {
          return false;
        }
      }
    }
  } else {
    // for sample rate: 96KHz or 192KHz:
    switch(config_cie->maxTargetBitrate)
    {
      case A2DP_LHDCV5_MAX_BIT_RATE_01:
      {
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
          *index = A2DP_LHDCV5_QUALITY_MID;
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
          *index = A2DP_LHDCV5_QUALITY_LOW;
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
          *index = A2DP_LHDCV5_QUALITY_LOW;
        break;
      }
      case A2DP_LHDCV5_MAX_BIT_RATE_10:
      {
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
          *index = A2DP_LHDCV5_QUALITY_HIGH;
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
          *index = A2DP_LHDCV5_QUALITY_MID;
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
          *index = A2DP_LHDCV5_QUALITY_MID;
        break;
      }
      case A2DP_LHDCV5_MAX_BIT_RATE_11:
      {
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
          *index = A2DP_LHDCV5_QUALITY_HIGH;
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
          *index = A2DP_LHDCV5_QUALITY_HIGH;
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
          *index = A2DP_LHDCV5_QUALITY_MID;
        break;
      }
      case A2DP_LHDCV5_MAX_BIT_RATE_00:
      {
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
          *index = A2DP_LHDCV5_QUALITY_HIGH1;
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
          *index = A2DP_LHDCV5_QUALITY_HIGH1;
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
          *index = A2DP_LHDCV5_QUALITY_MID;
        break;
      }
      default:
      {
        return false;
      }
    }
  }

  return true;
}

static bool lhdcV5_get_minBitrate_index(uint8_t* index, tA2DP_LHDCV5_CIE* config_cie) {
  if (config_cie->sampleRate == A2DP_LHDCV5_SAMPLING_FREQ_44100 ||
      config_cie->sampleRate == A2DP_LHDCV5_SAMPLING_FREQ_48000) {
    if (config_cie->hasFeatureNewMmBR == true) {
      switch (config_cie->minTargetBitrate)
      {
        case A2DP_LHDCV5_MIN_BIT_RATE_00:
        {
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW2;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
            *index = A2DP_LHDCV5_QUALITY_LOW1;
          break;
        }
        case A2DP_LHDCV5_MIN_BIT_RATE_01:
        {
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW4;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW1;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
            *index = A2DP_LHDCV5_QUALITY_LOW0;
          break;
        }
        case A2DP_LHDCV5_MIN_BIT_RATE_10:
        {
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW4;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW0;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
            *index = A2DP_LHDCV5_QUALITY_LOW0;
          break;
        }
        case A2DP_LHDCV5_MIN_BIT_RATE_11:
        {
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
            *index = A2DP_LHDCV5_QUALITY_MID;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW3;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
            *index = A2DP_LHDCV5_QUALITY_LOW2;
          break;
        }
        default:
        {
          return false;
        }
      }
    } else {
      switch (config_cie->minTargetBitrate)
      {
        case A2DP_LHDCV5_MIN_BIT_RATE_00:
        {
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW2;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
            *index = A2DP_LHDCV5_QUALITY_LOW1;
          break;
        }
        case A2DP_LHDCV5_MIN_BIT_RATE_01:
        {
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW4;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW1;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
            *index = A2DP_LHDCV5_QUALITY_LOW0;
          break;
        }
        case A2DP_LHDCV5_MIN_BIT_RATE_10:
        {
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW4;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW3;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
            *index = A2DP_LHDCV5_QUALITY_LOW0;
          break;
        }
        case A2DP_LHDCV5_MIN_BIT_RATE_11:
        {
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
            *index = A2DP_LHDCV5_QUALITY_MID;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
            *index = A2DP_LHDCV5_QUALITY_LOW;
          if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
            *index = A2DP_LHDCV5_QUALITY_LOW2;
          break;
        }
        default:
        {
          return false;
        }
      }
    }
  } else {
    // sample rate: 96KHz or 192KHz:
    switch (config_cie->minTargetBitrate)
    {
      case A2DP_LHDCV5_MIN_BIT_RATE_00:
      {
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
          *index = A2DP_LHDCV5_QUALITY_LOW;
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
          *index = A2DP_LHDCV5_QUALITY_LOW3;
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
          *index = A2DP_LHDCV5_QUALITY_LOW3;
        break;
      }
      case A2DP_LHDCV5_MIN_BIT_RATE_01:
      {
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
          *index = A2DP_LHDCV5_QUALITY_LOW4;
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
          *index = A2DP_LHDCV5_QUALITY_LOW3;
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
          *index = A2DP_LHDCV5_QUALITY_LOW3;
        break;
      }
      case A2DP_LHDCV5_MIN_BIT_RATE_10:
      {
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
          *index = A2DP_LHDCV5_QUALITY_LOW4;
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
          *index = A2DP_LHDCV5_QUALITY_LOW3;
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
          *index = A2DP_LHDCV5_QUALITY_LOW3;
        break;
      }
      case A2DP_LHDCV5_MIN_BIT_RATE_11:
      {
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_2P5MS)
          *index = A2DP_LHDCV5_QUALITY_MID;
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS)
          *index = A2DP_LHDCV5_QUALITY_LOW;
        if (config_cie->frameLenType == A2DP_LHDCV5_FRAME_LEN_10MS)
          *index = A2DP_LHDCV5_QUALITY_LOW4;
        break;
      }
      default:
      {
        return false;
      }
    }
  }

  return true;
}

// to check if target feature bit is set in codec_user_config_
static bool A2DP_IsFeatureInUserConfigLhdcV5(tA2DP_CODEC_CONFIGS_PACK* cfgsPtr, uint8_t featureCode) {
  if (cfgsPtr == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  switch (featureCode)
  {
    case LHDCV5_FEATURE_CODE_LL:
    {
      return LHDCV5_CHECK_IN_A2DP_SPEC(cfgsPtr->_codec_user_config_,
          a2dp_lhdcv5_source_spec_LL.inSpecBank, a2dp_lhdcv5_source_spec_LL.value);
    } break;
    case LHDCV5_FEATURE_CODE_LLESS:
    {
      return LHDCV5_CHECK_IN_A2DP_SPEC(cfgsPtr->_codec_user_config_,
          a2dp_lhdcv5_source_spec_LLESS.inSpecBank, a2dp_lhdcv5_source_spec_LLESS.value);
    } break;
    case LHDCV5_FEATURE_CODE_LLESS_RAW:
    {
      return LHDCV5_CHECK_IN_A2DP_SPEC(cfgsPtr->_codec_user_config_,
          a2dp_lhdcv5_source_spec_LLESS_RAW.inSpecBank, a2dp_lhdcv5_source_spec_LLESS_RAW.value);
    } break;
    default:
      break;
    }

  return false;
}

// to check if target feature bit is set in codec_config_
static bool A2DP_IsFeatureInCodecConfigLhdcV5(tA2DP_CODEC_CONFIGS_PACK* cfgsPtr, uint8_t featureCode) {
  if (cfgsPtr == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  switch(featureCode)
  {
    case LHDCV5_FEATURE_CODE_LL:
    {
      return LHDCV5_CHECK_IN_A2DP_SPEC(cfgsPtr->_codec_config_,
          a2dp_lhdcv5_source_spec_LL.inSpecBank, a2dp_lhdcv5_source_spec_LL.value);
    } break;
    case LHDCV5_FEATURE_CODE_LLESS:
    {
      return LHDCV5_CHECK_IN_A2DP_SPEC(cfgsPtr->_codec_config_,
          a2dp_lhdcv5_source_spec_LLESS.inSpecBank, a2dp_lhdcv5_source_spec_LLESS.value);
    } break;
    case LHDCV5_FEATURE_CODE_LLESS_RAW:
    {
      return LHDCV5_CHECK_IN_A2DP_SPEC(cfgsPtr->_codec_config_,
          a2dp_lhdcv5_source_spec_LLESS_RAW.inSpecBank, a2dp_lhdcv5_source_spec_LLESS_RAW.value);
    } break;
    default:
      break;
  }

  return false;
}

static void A2DP_UpdateFeatureToSpecLhdcV5(tA2DP_CODEC_CONFIGS_PACK* cfgsPtr,
    uint16_t toCodecCfg, bool hasFeature, uint8_t toSpec, int64_t value) {
  if (cfgsPtr == nullptr) {
    log::error( ": nullptr input");
    return;
  }

  if (toCodecCfg & A2DP_LHDC_TO_A2DP_CODEC_CONFIG_) {
    LHDC_SETUP_A2DP_SPEC(cfgsPtr->_codec_config_, toSpec, hasFeature, value);
  }
  if (toCodecCfg & A2DP_LHDC_TO_A2DP_CODEC_LOCAL_CAP_) {
    LHDC_SETUP_A2DP_SPEC(cfgsPtr->_codec_local_capability_, toSpec, hasFeature, value);
  }
  if (toCodecCfg & A2DP_LHDC_TO_A2DP_CODEC_SELECT_CAP_) {
    LHDC_SETUP_A2DP_SPEC(cfgsPtr->_codec_selectable_capability_, toSpec, hasFeature, value);
  }
  if (toCodecCfg & A2DP_LHDC_TO_A2DP_CODEC_USER_) {
    LHDC_SETUP_A2DP_SPEC(cfgsPtr->_codec_user_config_, toSpec, hasFeature, value);
  }
  if (toCodecCfg & A2DP_LHDC_TO_A2DP_CODEC_AUDIO_) {
    LHDC_SETUP_A2DP_SPEC(cfgsPtr->_codec_audio_config_, toSpec, hasFeature, value);
  }
}

// to update feature bit value to target codec config's specific
static void A2DP_UpdateFeatureToA2dpConfigLhdcV5(tA2DP_CODEC_CONFIGS_PACK *cfgsPtr,
    uint8_t featureCode,  uint16_t toCodecCfg, bool hasFeature) {
  if (cfgsPtr == nullptr) {
    log::error( ": nullptr input");
    return;
  }

  switch(featureCode)
  {
    case LHDCV5_FEATURE_CODE_LL:
      A2DP_UpdateFeatureToSpecLhdcV5(cfgsPtr, toCodecCfg, hasFeature,
          a2dp_lhdcv5_source_spec_LL.inSpecBank, a2dp_lhdcv5_source_spec_LL.value);
      break;
    case LHDCV5_FEATURE_CODE_LLESS:
      A2DP_UpdateFeatureToSpecLhdcV5(cfgsPtr, toCodecCfg, hasFeature,
          a2dp_lhdcv5_source_spec_LLESS.inSpecBank, a2dp_lhdcv5_source_spec_LLESS.value);
      break;
    case LHDCV5_FEATURE_CODE_LLESS_RAW:
      A2DP_UpdateFeatureToSpecLhdcV5(cfgsPtr, toCodecCfg, hasFeature,
          a2dp_lhdcv5_source_spec_LLESS_RAW.inSpecBank, a2dp_lhdcv5_source_spec_LLESS_RAW.value);
      break;
    default:
      break;
  }
}
//
// Utilities for LHDC configuration on A2DP specifics - END

static const tA2DP_ENCODER_INTERFACE a2dp_encoder_interface_lhdcv5 = {
    a2dp_vendor_lhdcv5_encoder_init,
    a2dp_vendor_lhdcv5_encoder_cleanup,
    a2dp_vendor_lhdcv5_feeding_reset,
    a2dp_vendor_lhdcv5_feeding_flush,
    a2dp_vendor_lhdcv5_get_encoder_interval_ms,
    a2dp_vendor_lhdcv5_get_effective_frame_size,
    a2dp_vendor_lhdcv5_send_frames,
    a2dp_vendor_lhdcv5_set_transmit_queue_length,
};

#ifdef HAS_LHDCV5_SINK
static const tA2DP_DECODER_INTERFACE a2dp_decoder_interface_lhdcv5 = {
    a2dp_vendor_lhdcv5_decoder_init,
    a2dp_vendor_lhdcv5_decoder_cleanup,
    a2dp_vendor_lhdcv5_decoder_decode_packet,
    a2dp_vendor_lhdcv5_decoder_start,
    a2dp_vendor_lhdcv5_decoder_suspend,
    a2dp_vendor_lhdcv5_decoder_configure,
};
#endif

UNUSED_ATTR static tA2DP_STATUS A2DP_CodecInfoMatchesCapabilityLhdcV5(
    const tA2DP_LHDCV5_CIE* p_cap, const uint8_t* p_codec_info,
    bool is_capability);


// check if target version is supported right now
static bool is_codec_version_supported(uint8_t version, bool is_source) {
  const tA2DP_LHDCV5_CIE* p_a2dp_lhdcv5_caps =
      (is_source) ? &a2dp_lhdcv5_source_caps : &a2dp_lhdcv5_sink_caps;

  if ((version & p_a2dp_lhdcv5_caps->version) != A2DP_LHDCV5_VER_NS) {
    return true;
  }

  log::error( ": versoin unsupported! peer:{} local:{}",
       version, p_a2dp_lhdcv5_caps->version);
  return false;
}

// Builds the LHDC Media Codec Capabilities byte sequence beginning from the
// LOSC octet. |media_type| is the media type |AVDT_MEDIA_TYPE_*|.
// |p_ie| is a pointer to the LHDC Codec Information Element information.
// The result is stored in |p_result|. Returns A2DP_SUCCESS on success,
// otherwise the corresponding A2DP error status code.
static tA2DP_STATUS A2DP_BuildInfoLhdcV5(uint8_t media_type,
    const tA2DP_LHDCV5_CIE* p_ie,
    uint8_t* p_result) {

  const uint8_t* tmpInfo = p_result;
  uint8_t para = 0;

  if (p_ie == nullptr || p_result == nullptr) {
    log::error( ": nullptr input");
    return A2DP_FAIL;
  }

  *p_result++ = A2DP_LHDCV5_CODEC_LEN;  //H0
  *p_result++ = (media_type << 4);      //H1
  *p_result++ = A2DP_MEDIA_CT_NON_A2DP; //H2

  // Vendor ID(P0-P3) and Codec ID(P4-P5)
  *p_result++ = (uint8_t)(p_ie->vendorId & 0x000000FF);
  *p_result++ = (uint8_t)((p_ie->vendorId & 0x0000FF00) >> 8);
  *p_result++ = (uint8_t)((p_ie->vendorId & 0x00FF0000) >> 16);
  *p_result++ = (uint8_t)((p_ie->vendorId & 0xFF000000) >> 24);
  *p_result++ = (uint8_t)(p_ie->codecId & 0x00FF);
  *p_result++ = (uint8_t)((p_ie->codecId & 0xFF00) >> 8);

  para = 0;
  // P6[7:6]: Frame Length(Duration) select
  para |= (p_ie->frameLenSelect & A2DP_LHDCV5_FRAME_LEN_SELECT_MASK);

  // P6[5:4] + P6[2] + P6[0]: Sampling Frequency
  if ((p_ie->sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_MASK) != A2DP_LHDCV5_SAMPLING_FREQ_NS) {
    para |= (p_ie->sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_MASK);
  } else {
    log::error( ": invalid sample rate (0x{:02X})",  p_ie->sampleRate);
    return A2DP_FAIL;
  }
  // update P6
  *p_result++ = para; para = 0;

  // P7[2:0]: Bit Depth
  if ((p_ie->bitsPerSample & A2DP_LHDCV5_BIT_FMT_MASK) != A2DP_LHDCV5_BIT_FMT_NS &&
      (p_ie->bitsPerSample & A2DP_LHDCV5_BIT_FMT_MASK) != A2DP_LHDCV5_BIT_FMT_ERR) {
    para |= (p_ie->bitsPerSample & A2DP_LHDCV5_BIT_FMT_MASK);
  } else {
    log::error( ": invalid bits per sample (0x{:02X})",  p_ie->bitsPerSample);
    return A2DP_FAIL;
  }
  // P7[5:4] Max Target Bit Rate
  para |= (p_ie->maxTargetBitrate & A2DP_LHDCV5_MAX_BIT_RATE_MASK);
  // P7[7:6] Min Target Bit Rate
  para |= (p_ie->minTargetBitrate & A2DP_LHDCV5_MIN_BIT_RATE_MASK);
  // update P7
  *p_result++ = para; para = 0;

  // P8[3:0] Codec SubVersion
  if ((p_ie->version & A2DP_LHDCV5_VERSION_MASK) != A2DP_LHDCV5_VER_NS) {
    para = para | (p_ie->version & A2DP_LHDCV5_VERSION_MASK);
  } else {
    log::error( ": invalid codec subversion (0x{:02X})",  p_ie->version);
    return A2DP_FAIL;
  }
  // P8[7:6] + P8[4]: Frame Length Type
  if ((p_ie->frameLenType & A2DP_LHDCV5_FRAME_LEN_MASK) != A2DP_LHDCV5_FRAME_LEN_NS) {
    para = para | (p_ie->frameLenType & A2DP_LHDCV5_FRAME_LEN_MASK);
  } else {
    log::error( ": invalid frame_length type (0x{:02X})",  p_ie->frameLenType);
    return A2DP_FAIL;
  }
  // update P8
  *p_result++ = para; para = 0;

  // P9[4] HasLossless96K
  // P9[5] HasLossless24Bit
  // P9[6] HasLL
  // P9[7] HasLossless48K
  if (p_ie->hasFeatureLL) {
    para |= A2DP_LHDCV5_FEATURE_LL;
  }
  if (p_ie->hasFeatureLLESS) {
    para |= A2DP_LHDCV5_FEATURE_LLESS48K;
  }
  if (p_ie->hasFeatureLLESS24Bit) {
    para |= A2DP_LHDCV5_FEATURE_LLESS24BIT;
  }
  if (p_ie->hasFeatureLLESS96K) {
    para |= A2DP_LHDCV5_FEATURE_LLESS96K;
  }
  // update P9
  *p_result++ = para; para = 0;

  // P10[7] HaslosslessRaw
  // P10[6] hasFeatureNewMmBR
  // P10[5] hasFeaturebr128kbps
  // P10[3:1] exMBR
  if (p_ie->hasFeatureLLESSRaw) {
    para |= A2DP_LHDCV5_FEATURE_LLESS_RAW;
  }
  if (p_ie->hasFeatureNewMmBR) {
    para |= A2DP_LHDCV5_FEATURE_newMmBR;
  }
  if (p_ie->hasFeaturebr128kbps) {
    para |= A2DP_LHDCV5_FEATURE_128KBPS;
  }
  if (p_ie->exMBR) {
    para = para | (p_ie->exMBR & A2DP_LHDCV5_EXMBR_MASK);
  }
  // update P10
  *p_result++ = para; para = 0;

  log::info( ": codec info built = H0-H2:[{:02X} {:02X} {:02X}] P0-P3:[{:02X} "
      "{:02X} {:02X} {:02X}] P4-P5:[{:02X} {:02X}] "
      "P6:{:02X} P7:{:02X} P8:{:02X} P9:{:02X} P10:{:02X}",
      tmpInfo[0], tmpInfo[1], tmpInfo[2], tmpInfo[3], tmpInfo[4], tmpInfo[5], tmpInfo[6], tmpInfo[7],
      tmpInfo[8], tmpInfo[9], tmpInfo[10], tmpInfo[11], tmpInfo[12], tmpInfo[A2DP_LHDCV5_CODEC_LEN]);

  return A2DP_SUCCESS;
}

// Parses the LHDC Media Codec Capabilities byte sequence beginning from the
// LOSC octet. The result is stored in |p_ie|. The byte sequence to parse is
// |p_codec_info|. If |is_capability| is true, the byte sequence is
// codec capabilities, otherwise is codec configuration.
// Returns A2DP_SUCCESS on success, otherwise the corresponding A2DP error
// status code.
static tA2DP_STATUS A2DP_ParseInfoLhdcV5(tA2DP_LHDCV5_CIE* p_ie,
    const uint8_t* p_codec_info,
    bool is_capability,
    bool is_source) {
  uint8_t losc;
  uint8_t media_type;
  tA2DP_CODEC_TYPE codec_type;
  const uint8_t* tmpInfo = p_codec_info;
#ifdef HAS_LHDCV5_SINK
  const uint8_t* p_codec_Info_save = p_codec_info;
#endif

  if (p_ie == nullptr || p_codec_info == nullptr) {
    log::error( ": nullptr input");
    return A2DP_FAIL;
  }

  // Codec capability length
  losc = *p_codec_info++;
  if (losc != A2DP_LHDCV5_CODEC_LEN) {
    log::error( ": wrong length {}",  losc);
    return A2DP_FAIL;
  }

  media_type = (*p_codec_info++) >> 4;
  codec_type = (tA2DP_CODEC_TYPE)*p_codec_info++;

  // Media Type and Media Codec Type
  if (media_type != AVDT_MEDIA_TYPE_AUDIO ||
      codec_type != A2DP_MEDIA_CT_NON_A2DP) {
    log::error( ": invalid media type 0x{:02X} codec_type 0x{:02X}",  media_type, codec_type);
    return A2DP_FAIL;
  }

  // Vendor ID(P0-P3) and Codec ID(P4-P5)
  p_ie->vendorId = (*p_codec_info & 0x000000FF) |
      (*(p_codec_info + 1) << 8 & 0x0000FF00) |
      (*(p_codec_info + 2) << 16 & 0x00FF0000) |
      (*(p_codec_info + 3) << 24 & 0xFF000000);
  p_codec_info += 4;
  p_ie->codecId = (*p_codec_info & 0x00FF) | (*(p_codec_info + 1) << 8 & 0xFF00);
  p_codec_info += 2;
  if (p_ie->vendorId != A2DP_LHDC_VENDOR_ID ||
      p_ie->codecId != A2DP_LHDCV5_CODEC_ID) {
    log::error( ": invalid vendorId 0x{:02X} codecId 0x{:02X}",
        p_ie->vendorId, p_ie->codecId);
    return A2DP_FAIL;
  }

  // P6[7:6]: Frame Length(Duration) select
  p_ie->frameLenSelect = (*p_codec_info & A2DP_LHDCV5_FRAME_LEN_SELECT_MASK);

  // P6[5:4] + P6[2] + P6[0]: Sampling Frequency
  p_ie->sampleRate = (*p_codec_info & A2DP_LHDCV5_SAMPLING_FREQ_MASK);
  if (p_ie->sampleRate == A2DP_LHDCV5_SAMPLING_FREQ_NS) {
    log::error( ": invalid sample rate 0x{:02X}",  p_ie->sampleRate);
    return A2DP_FAIL;
  }
  p_codec_info += 1;

  // P7[2:0]: Bit depth
  p_ie->bitsPerSample = (*p_codec_info & A2DP_LHDCV5_BIT_FMT_MASK);
  if (p_ie->bitsPerSample == A2DP_LHDCV5_BIT_FMT_NS ||
      p_ie->bitsPerSample == A2DP_LHDCV5_BIT_FMT_ERR) {
    log::error( ": invalid bit per sample 0x{:02X}",  p_ie->bitsPerSample);
    return A2DP_FAIL;
  }
  // P7[5:4]: Max Target Bit Rate
  p_ie->maxTargetBitrate = (*p_codec_info & A2DP_LHDCV5_MAX_BIT_RATE_MASK);
  // P7[7:6]: Min Target Bit Rate
  p_ie->minTargetBitrate = (*p_codec_info & A2DP_LHDCV5_MIN_BIT_RATE_MASK);
  p_codec_info += 1;

  // Channel Mode: stereo only
  p_ie->channelMode = A2DP_LHDCV5_CHANNEL_MODE_STEREO;

  // P8[3:0]: Codec SubVersion
  p_ie->version = (*p_codec_info & A2DP_LHDCV5_VERSION_MASK);
  if (p_ie->version == A2DP_LHDCV5_VER_NS) {
    log::error( ": invalid version 0x{:02X}",  p_ie->version);
    return A2DP_FAIL;
  } else {
    if (!is_codec_version_supported(p_ie->version, is_source)) {
      log::error( ": unsupported version 0x{:02X}",  p_ie->version);
      return A2DP_FAIL;
    }
  }
  // P8[7:6] + P8[4]: Frame Length Type
  p_ie->frameLenType = (*p_codec_info & A2DP_LHDCV5_FRAME_LEN_MASK);
  if (p_ie->frameLenType == A2DP_LHDCV5_FRAME_LEN_NS) {
    log::error( ": invalid frame_length mode 0x{:02X}",  p_ie->frameLenType);
    return A2DP_FAIL;
  }
  p_codec_info += 1;

  // Features:
  // P9[4] HasLossless96K
  // P9[5] HasLossless24bit
  // P9[6] HasLL
  // P9[7] HasLossless
  p_ie->hasFeatureLL = ((*p_codec_info & A2DP_LHDCV5_FEATURE_LL) != 0) ? true : false;
  p_ie->hasFeatureLLESS = ((*p_codec_info & A2DP_LHDCV5_FEATURE_LLESS48K) != 0) ? true : false;
  p_ie->hasFeatureLLESS24Bit = ((*p_codec_info & A2DP_LHDCV5_FEATURE_LLESS24BIT) != 0) ? true : false;
  p_ie->hasFeatureLLESS96K = ((*p_codec_info & A2DP_LHDCV5_FEATURE_LLESS96K) != 0) ? true : false;
  p_codec_info += 1;

  // P10[7] HasLosslessRaw
  // P10[6] hasFeatureNewMmBR
  // P10[5] hasFeaturebr128kbps
  // P10[3:1] hasFeaturebrExMBR
  p_ie->hasFeatureLLESSRaw = ((*p_codec_info & A2DP_LHDCV5_FEATURE_LLESS_RAW) != 0) ? true : false;
  p_ie->hasFeatureNewMmBR = ((*p_codec_info & A2DP_LHDCV5_FEATURE_newMmBR) != 0) ? true : false;
  p_ie->hasFeaturebr128kbps = ((*p_codec_info & A2DP_LHDCV5_FEATURE_128KBPS) != 0) ? true : false;
  p_ie->exMBR = (*p_codec_info & A2DP_LHDCV5_EXMBR_MASK);

  log::info( ": codec info parsed = H0-H2:[{:02X} {:02X} {:02X}] "
      "P0-P3(vender id):[{:02X} {:02X} {:02X} {:02X}] P4-P5(codec id):[{:02X} {:02X}] "
      "P6:{:02X} P7:{:02X} P8:{:02X} P9:{:02X} P10:{:02X}",
      tmpInfo[0], tmpInfo[1], tmpInfo[2], tmpInfo[3], tmpInfo[4], tmpInfo[5], tmpInfo[6], tmpInfo[7],
      tmpInfo[8], tmpInfo[9], tmpInfo[10], tmpInfo[11], tmpInfo[12], tmpInfo[A2DP_LHDCV5_CODEC_LEN]);

  log::info( ": Role:{} isCap:{} SR:0x{:02X} Bits:0x{:02X} Ver:0x{:02X} FL:0x{:02X} FLS:0x{:02X} "
      "maxBR:0x{:02X} minBR:0x{:02X} exMBR:0x{:02X} "
      "[LL({}) LLESS({}) LLESS24({}) LLESS96K({}) LLESSRaw({}) newMmBR({}) br128({})]",
      (is_source?"SRC":"SNK"),
      is_capability,
      p_ie->sampleRate,
      p_ie->bitsPerSample,
      p_ie->version,
      p_ie->frameLenType,
      p_ie->frameLenSelect,
      p_ie->maxTargetBitrate,
      p_ie->minTargetBitrate,
      p_ie->exMBR,
      p_ie->hasFeatureLL,
      p_ie->hasFeatureLLESS,
      p_ie->hasFeatureLLESS24Bit,
      p_ie->hasFeatureLLESS96K,
      p_ie->hasFeatureLLESSRaw,
      p_ie->hasFeatureNewMmBR,
      p_ie->hasFeaturebr128kbps);

#ifdef HAS_LHDCV5_SINK
  // LHDC local SNK only: save decoder needed parameters from SRC's configuration
  if (!is_source && !is_capability) {
    if (!a2dp_lhdcv5_decoder_save_codec_info(p_codec_Info_save)) {
      log::info( ": save decoder parameters error");
    }
  }
#endif

  return A2DP_SUCCESS;
}

bool A2DP_IsVendorSourceCodecValidLhdcV5(const uint8_t* p_codec_info) {
  tA2DP_LHDCV5_CIE cfg_cie;
  bool ret = false;
  /* Use a liberal check when parsing the codec info */
  ret = (A2DP_ParseInfoLhdcV5(&cfg_cie, p_codec_info, false, IS_SRC) == A2DP_SUCCESS) ||
      (A2DP_ParseInfoLhdcV5(&cfg_cie, p_codec_info, true, IS_SRC) == A2DP_SUCCESS);
  if (ret == false) log::debug( "false");
  return ret;
}
bool A2DP_IsVendorSinkCodecValidLhdcV5(const uint8_t* p_codec_info) {
  tA2DP_LHDCV5_CIE cfg_cie;
  bool ret = false;
  /* Use a liberal check when parsing the codec info */
  ret = (A2DP_ParseInfoLhdcV5(&cfg_cie, p_codec_info, false, IS_SNK) == A2DP_SUCCESS) ||
      (A2DP_ParseInfoLhdcV5(&cfg_cie, p_codec_info, true, IS_SNK) == A2DP_SUCCESS);
  if (ret == false) log::debug( "false");
  return ret;
}

bool A2DP_IsVendorPeerSinkCodecValidLhdcV5(const uint8_t* p_codec_info) {
  tA2DP_LHDCV5_CIE cfg_cie;
  bool ret = false;
  /* Use a liberal check when parsing the codec info */
  ret = (A2DP_ParseInfoLhdcV5(&cfg_cie, p_codec_info, false, IS_SRC) == A2DP_SUCCESS) ||
      (A2DP_ParseInfoLhdcV5(&cfg_cie, p_codec_info, true, IS_SRC) == A2DP_SUCCESS);
  if (ret == false) log::debug( "false");
  return ret;
}
bool A2DP_IsVendorPeerSourceCodecValidLhdcV5(const uint8_t* p_codec_info) {
  tA2DP_LHDCV5_CIE cfg_cie;
  bool ret = false;
  /* Use a liberal check when parsing the codec info */
  ret = (A2DP_ParseInfoLhdcV5(&cfg_cie, p_codec_info, false, IS_SNK) == A2DP_SUCCESS) ||
      (A2DP_ParseInfoLhdcV5(&cfg_cie, p_codec_info, true, IS_SNK) == A2DP_SUCCESS);
  if (ret == false) log::debug( "false");
  return ret;
}

// NOTE: Should be done only for local Sink codec
bool A2DP_IsVendorSinkCodecSupportedLhdcV5(const uint8_t* p_codec_info) {
  bool ret = false;
  ret =  (A2DP_CodecInfoMatchesCapabilityLhdcV5(&a2dp_lhdcv5_sink_caps, p_codec_info,
      false) == A2DP_SUCCESS);
  if (ret == false) log::debug( "false");
  return ret;
}
// NOTE: Should be done only for local Sink codec
bool A2DP_IsPeerSourceCodecSupportedLhdcV5(const uint8_t* p_codec_info) {
  bool ret = false;
  ret =  (A2DP_CodecInfoMatchesCapabilityLhdcV5(&a2dp_lhdcv5_sink_caps, p_codec_info,
      true) == A2DP_SUCCESS);
  if (ret == false) log::debug( "false");
  return ret;
}

// Checks whether A2DP LHDC codec configuration matches with a device's codec
// capabilities.
//  |p_cap| is the LHDC local codec capabilities.
//  |p_codec_info| is peer's codec capabilities acting as an A2DP source.
// If |is_capability| is true, the byte sequence is codec capabilities,
// otherwise is codec configuration.
// Returns A2DP_SUCCESS if the codec configuration matches with capabilities,
// otherwise the corresponding A2DP error status code.
static tA2DP_STATUS A2DP_CodecInfoMatchesCapabilityLhdcV5(
    const tA2DP_LHDCV5_CIE* p_cap, const uint8_t* p_codec_info, bool is_capability) {
  tA2DP_STATUS status;
  tA2DP_LHDCV5_CIE cfg_cie;

  if (p_cap == nullptr || p_codec_info == nullptr) {
    log::error( ": nullptr input");
    return A2DP_FAIL;
  }

  // parse configuration
  status = A2DP_ParseInfoLhdcV5(&cfg_cie, p_codec_info, is_capability, IS_SNK);
  if (status != A2DP_SUCCESS) {
    log::error( ": parsing failed {}",  status);
    return status;
  }

  // verify that each parameter is in range
  log::info( ": FREQ peer: 0x{:02X}, capability 0x{:02X}",
      cfg_cie.sampleRate, p_cap->sampleRate);

  log::info( ": BIT_FMT peer: 0x{:02X}, capability 0x{:02X}",
      cfg_cie.bitsPerSample, p_cap->bitsPerSample);

  // sampling frequency
  if ((cfg_cie.sampleRate & p_cap->sampleRate) == 0) return A2DP_FAIL;

  // bits per sample
  if ((cfg_cie.bitsPerSample & p_cap->bitsPerSample) == 0) return A2DP_FAIL;

  return A2DP_SUCCESS;
}

bool A2DP_VendorUsesRtpHeaderLhdcV5(UNUSED_ATTR bool content_protection_enabled,
    UNUSED_ATTR const uint8_t* p_codec_info) {
  // TODO: Is this correct? The RTP header is always included?
  return true;
}

const char* A2DP_VendorCodecNameLhdcV5(UNUSED_ATTR const uint8_t* p_codec_info) {
  return "LHDC V5";
}

bool A2DP_VendorCodecTypeEqualsLhdcV5(const uint8_t* p_codec_info_a,
    const uint8_t* p_codec_info_b) {
  tA2DP_LHDCV5_CIE lhdc_cie_a;
  tA2DP_LHDCV5_CIE lhdc_cie_b;
  tA2DP_STATUS a2dp_status;

  if (p_codec_info_a == nullptr || p_codec_info_b == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  // Check whether the codec info contains valid data
  a2dp_status =
      A2DP_ParseInfoLhdcV5(&lhdc_cie_a, p_codec_info_a, true, IS_SRC);
  if (a2dp_status != A2DP_SUCCESS) {
    log::error( ": cannot decode codec information: {}",
        a2dp_status);
    return false;
  }
  a2dp_status = A2DP_ParseInfoLhdcV5(&lhdc_cie_b, p_codec_info_b, true, IS_SRC);
  if (a2dp_status != A2DP_SUCCESS) {
    log::error( ": cannot decode codec information: {}",
        a2dp_status);
    return false;
  }

  return true;
}

bool A2DP_VendorCodecEqualsLhdcV5(const uint8_t* p_codec_info_a,
    const uint8_t* p_codec_info_b) {
  tA2DP_LHDCV5_CIE lhdc_cie_a;
  tA2DP_LHDCV5_CIE lhdc_cie_b;
  tA2DP_STATUS a2dp_status;

  bool ret = false;

  if (p_codec_info_a == nullptr || p_codec_info_b == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  // Check whether the codec info contains valid data
  a2dp_status =
      A2DP_ParseInfoLhdcV5(&lhdc_cie_a, p_codec_info_a, true, IS_SRC);
  if (a2dp_status != A2DP_SUCCESS) {
    log::error( ": cannot decode codec information of a: {}",
        a2dp_status);
    return false;
  }

  a2dp_status =
      A2DP_ParseInfoLhdcV5(&lhdc_cie_b, p_codec_info_b, true, IS_SRC);
  if (a2dp_status != A2DP_SUCCESS) {
    log::error( ": cannot decode codec information of b: {}",
        a2dp_status);
    return false;
  }

  // exam items that require to update codec config with peer if different
  ret = (lhdc_cie_a.sampleRate == lhdc_cie_b.sampleRate) &&
      (lhdc_cie_a.bitsPerSample == lhdc_cie_b.bitsPerSample) &&
      (lhdc_cie_a.channelMode == lhdc_cie_b.channelMode) &&
      (lhdc_cie_a.frameLenType == lhdc_cie_b.frameLenType) &&
      (lhdc_cie_a.hasFeatureLL == lhdc_cie_b.hasFeatureLL) &&
      (lhdc_cie_a.hasFeatureLLESS == lhdc_cie_b.hasFeatureLLESS &&
      (lhdc_cie_a.hasFeatureLLESSRaw == lhdc_cie_b.hasFeatureLLESSRaw));

  return ret;
}

int A2DP_VendorGetBitRateLhdcV5(const uint8_t* p_codec_info) {

  A2dpCodecConfig* current_codec = bta_av_get_a2dp_current_codec();
  btav_a2dp_codec_config_t codec_config_ = current_codec->getCodecConfig();
  uint8_t bitRateIndex = 0;

  UNUSED(p_codec_info);

  if ((codec_config_.codec_specific_1 & A2DP_LHDCV5_QUALITY_CMD_MASK) ==
      A2DP_LHDCV5_QUALITY_MAGIC_NUM) {
    bitRateIndex = codec_config_.codec_specific_1 & A2DP_LHDCV5_QUALITY_MASK;
    switch (bitRateIndex) {
      case A2DP_LHDCV5_QUALITY_LOW0:
        return 130000;
      case A2DP_LHDCV5_QUALITY_LOW1:
        return 160000;
      case A2DP_LHDCV5_QUALITY_LOW2:
        return 200000;
      case A2DP_LHDCV5_QUALITY_LOW3:
        return 260000;
      case A2DP_LHDCV5_QUALITY_LOW4:
        return 320000;
      case A2DP_LHDCV5_QUALITY_LOW:
        return 390000;
      case A2DP_LHDCV5_QUALITY_MID:
        return 520000;
      case A2DP_LHDCV5_QUALITY_HIGH:
        return 900000;
      case A2DP_LHDCV5_QUALITY_HIGH1:
        return 1000000;
      case A2DP_LHDCV5_QUALITY_ABR:
        return 9999999;
      default:
        log::error(": non-supported bitrate index ({})",  bitRateIndex);
        return -1;
    }
  }

  return 390000;
}

int A2DP_VendorGetTrackSampleRateLhdcV5(const uint8_t* p_codec_info) {
  tA2DP_LHDCV5_CIE lhdc_cie;
  tA2DP_STATUS a2dp_status;

  if (p_codec_info == nullptr) {
    log::error( ": nullptr input");
    return -1;
  }

  // Check whether the codec info contains valid data
  a2dp_status = A2DP_ParseInfoLhdcV5(&lhdc_cie, p_codec_info, false, IS_SRC);
  if (a2dp_status != A2DP_SUCCESS) {
    log::error( ": cannot decode codec information: {}",
        a2dp_status);
    return -1;
  }

  switch (lhdc_cie.sampleRate) {
  case A2DP_LHDCV5_SAMPLING_FREQ_44100:
    return 44100;
  case A2DP_LHDCV5_SAMPLING_FREQ_48000:
    return 48000;
  case A2DP_LHDCV5_SAMPLING_FREQ_96000:
    return 96000;
  case A2DP_LHDCV5_SAMPLING_FREQ_192000:
    return 192000;
  }

  return -1;
}

int A2DP_VendorGetTrackBitsPerSampleLhdcV5(const uint8_t* p_codec_info) {
  tA2DP_LHDCV5_CIE lhdc_cie;
  tA2DP_STATUS a2dp_status;

  if (p_codec_info == nullptr) {
    log::error( ": nullptr input");
    return -1;
  }

  // Check whether the codec info contains valid data
  a2dp_status = A2DP_ParseInfoLhdcV5(&lhdc_cie, p_codec_info, false, IS_SRC);
  if (a2dp_status != A2DP_SUCCESS) {
    log::error( ": cannot decode codec information: {}",
        a2dp_status);
    return -1;
  }

  switch (lhdc_cie.bitsPerSample) {
  case A2DP_LHDCV5_BIT_FMT_16:
    return 16;
  case A2DP_LHDCV5_BIT_FMT_24:
    return 24;
  }

  return -1;
}

int A2DP_VendorGetTrackChannelCountLhdcV5(const uint8_t* p_codec_info) {
  tA2DP_LHDCV5_CIE lhdc_cie;
  tA2DP_STATUS a2dp_status;

  if (p_codec_info == nullptr) {
    log::error( ": nullptr input");
    return -1;
  }

  // Check whether the codec info contains valid data
  a2dp_status = A2DP_ParseInfoLhdcV5(&lhdc_cie, p_codec_info, false, IS_SRC);
  if (a2dp_status != A2DP_SUCCESS) {
    log::error( ": cannot decode codec information: {}",
        a2dp_status);
    return -1;
  }

  switch (lhdc_cie.channelMode) {
  case A2DP_LHDCV5_CHANNEL_MODE_MONO:
    return 1;
  case A2DP_LHDCV5_CHANNEL_MODE_DUAL:
    return 2;
  case A2DP_LHDCV5_CHANNEL_MODE_STEREO:
    return 2;
  }

  return -1;
}

int A2DP_VendorGetSinkTrackChannelTypeLhdcV5(const uint8_t* p_codec_info) {
  tA2DP_LHDCV5_CIE lhdc_cie;
  tA2DP_STATUS a2dp_status;

  if (p_codec_info == nullptr) {
    log::error( ": nullptr input");
    return -1;
  }

  // Check whether the codec info contains valid data
  a2dp_status = A2DP_ParseInfoLhdcV5(&lhdc_cie, p_codec_info, false, IS_SNK);
  if (a2dp_status != A2DP_SUCCESS) {
    log::error( ": cannot decode codec information: {}",
        a2dp_status);
    return -1;
  }

  switch (lhdc_cie.channelMode) {
  case A2DP_LHDCV5_CHANNEL_MODE_MONO:
    return 1;
  case A2DP_LHDCV5_CHANNEL_MODE_DUAL:
    return 3;
  case A2DP_LHDCV5_CHANNEL_MODE_STEREO:
    return 3;
  }

  return -1;
}

int A2DP_VendorGetChannelModeCodeLhdcV5(const uint8_t* p_codec_info) {
  tA2DP_LHDCV5_CIE lhdc_cie;
  tA2DP_STATUS a2dp_status;

  if (p_codec_info == nullptr) {
    log::error( ": nullptr input");
    return -1;
  }

  // Check whether the codec info contains valid data
  a2dp_status = A2DP_ParseInfoLhdcV5(&lhdc_cie, p_codec_info, false, IS_SRC);
  if (a2dp_status != A2DP_SUCCESS) {
    log::error( ": cannot decode codec information: {}",
        a2dp_status);
    return -1;
  }

  switch (lhdc_cie.channelMode) {
  case A2DP_LHDCV5_CHANNEL_MODE_MONO:
  case A2DP_LHDCV5_CHANNEL_MODE_DUAL:
  case A2DP_LHDCV5_CHANNEL_MODE_STEREO:
    return lhdc_cie.channelMode;
  default:
    break;
  }

  return -1;
}

bool A2DP_VendorGetPacketTimestampLhdcV5(UNUSED_ATTR const uint8_t* p_codec_info,
    const uint8_t* p_data,
    uint32_t* p_timestamp) {
  if (p_codec_info == nullptr || p_data == nullptr || p_timestamp == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  // TODO: Is this function really codec-specific?
  *p_timestamp = *(const uint32_t*)p_data;
  return true;
}

bool A2DP_VendorBuildCodecHeaderLhdcV5(UNUSED_ATTR const uint8_t* p_codec_info,
    BT_HDR* p_buf,
    uint16_t frames_per_packet) {
  uint8_t* p;

  if (p_codec_info == nullptr || p_buf == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  p_buf->offset -= A2DP_LHDC_MPL_HDR_LEN;
  p = (uint8_t*)(p_buf + 1) + p_buf->offset;
  p_buf->len += A2DP_LHDC_MPL_HDR_LEN;

  // Not support fragmentation
  p[0] = ( uint8_t)( frames_per_packet & 0xff);
  p[1] = ( uint8_t)( ( frames_per_packet >> 8) & 0xff);

  return true;
}

void A2DP_VendorDumpCodecInfoLhdcV5(const uint8_t* p_codec_info) {
  tA2DP_STATUS a2dp_status;
  tA2DP_LHDCV5_CIE lhdc_cie;

  a2dp_status = A2DP_ParseInfoLhdcV5(&lhdc_cie, p_codec_info, true, IS_SRC);
  if (a2dp_status != A2DP_SUCCESS) {
    log::error( ": A2DP_ParseInfoLhdcV5 fail:{}",  a2dp_status);
    return;
  }

  log::info( "\tsamp_freq: 0x{:02X} ", lhdc_cie.sampleRate);
  if (lhdc_cie.sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_44100) {
    log::info( "\tsamp_freq: (44100)");
  }
  if (lhdc_cie.sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_48000) {
    log::info( "\tsamp_freq: (48000)");
  }
  if (lhdc_cie.sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_96000) {
    log::info( "\tsamp_freq: (96000)");
  }
  if (lhdc_cie.sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_192000) {
    log::info( "\tsamp_freq: (19200)");
  }

  log::info( "\tbitsPerSample: 0x{} ", lhdc_cie.bitsPerSample);
  if (lhdc_cie.bitsPerSample & A2DP_LHDCV5_BIT_FMT_16) {
    log::info( "\tbit_depth: (16)");
  }
  if (lhdc_cie.bitsPerSample & A2DP_LHDCV5_BIT_FMT_24) {
    log::info( "\tbit_depth: (24)");
  }

  log::info( "\tchannelMode: 0x{:02X} ", lhdc_cie.channelMode);
  if (lhdc_cie.channelMode & A2DP_LHDCV5_CHANNEL_MODE_MONO) {
    log::info( "\tchannle_mode: (mono)");
  }
  if (lhdc_cie.channelMode & A2DP_LHDCV5_CHANNEL_MODE_DUAL) {
    log::info( "\tchannle_mode: (dual)");
  }
  if (lhdc_cie.channelMode & A2DP_LHDCV5_CHANNEL_MODE_STEREO) {
    log::info( "\tchannle_mode: (stereo)");
  }
}

std::string A2DP_VendorCodecInfoStringLhdcV5(const uint8_t* p_codec_info) {
  std::stringstream res;
  std::string field;
  tA2DP_STATUS a2dp_status;
  tA2DP_LHDCV5_CIE lhdc_cie;
  uint8_t maxBitrate_Idx;
  uint8_t minBitrate_Idx;

  if (p_codec_info == nullptr) {
    res << "A2DP_VendorCodecInfoStringLhdcV5 nullptr";
    return res.str();
  }

  a2dp_status = A2DP_ParseInfoLhdcV5(&lhdc_cie, p_codec_info, true, IS_SRC);
  if (a2dp_status != A2DP_SUCCESS) {
    res << "A2DP_ParseInfoLhdcV5 fail: " << loghex(static_cast<uint8_t>(a2dp_status));
    return res.str();
  }

  res << "\tname: LHDC V5\n";

  // Sample frequency
  field.clear();
  AppendField(&field, (lhdc_cie.sampleRate == A2DP_LHDCV5_SAMPLING_FREQ_NS), "NONE");
  AppendField(&field, (lhdc_cie.sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_44100),
      "44100");
  AppendField(&field, (lhdc_cie.sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_48000),
      "48000");
  AppendField(&field, (lhdc_cie.sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_96000),
      "96000");
  AppendField(&field, (lhdc_cie.sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_192000),
      "192000");
  res << "\tsamp_freq: " << field << " (" << loghex(lhdc_cie.sampleRate)
                                                                      << ")\n";

  // bits per sample
  field.clear();
  AppendField(&field, (lhdc_cie.bitsPerSample == A2DP_LHDCV5_BIT_FMT_NS), "NONE");
  AppendField(&field, (lhdc_cie.bitsPerSample & A2DP_LHDCV5_BIT_FMT_16),
      "16");
  AppendField(&field, (lhdc_cie.bitsPerSample & A2DP_LHDCV5_BIT_FMT_24),
      "24");
  res << "\tbits_depth: " << field << " bits (" << loghex((int)lhdc_cie.bitsPerSample)
                                                                      << ")\n";

  // Channel mode
  field.clear();
  AppendField(&field, (lhdc_cie.channelMode == A2DP_LHDCV5_CHANNEL_MODE_NS), "NONE");
  AppendField(&field, (lhdc_cie.channelMode & A2DP_LHDCV5_CHANNEL_MODE_MONO),
      "Mono");
  AppendField(&field, (lhdc_cie.channelMode & A2DP_LHDCV5_CHANNEL_MODE_DUAL),
      "Dual");
  AppendField(&field, (lhdc_cie.channelMode & A2DP_LHDCV5_CHANNEL_MODE_STEREO),
      "Stereo");
  res << "\tch_mode: " << field << " (" << loghex(lhdc_cie.channelMode)
                                                                      << ")\n";

  // Version
  field.clear();
  AppendField(&field, (lhdc_cie.version == A2DP_LHDCV5_VER_NS), "NONE");
  AppendField(&field, (lhdc_cie.version == A2DP_LHDCV5_VER_1),
      "LHDCV5 Ver1");
  res << "\tversion: " << field << " (" << loghex(lhdc_cie.version)
                                                                      << ")\n";

  // frame length
  field.clear();
  AppendField(&field, (lhdc_cie.frameLenType == A2DP_LHDCV5_FRAME_LEN_NS), "NONE");
  AppendField(&field, (lhdc_cie.frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS),
      "5 ms");
  res << "\tframeLenType: " << field << " (" << loghex(lhdc_cie.frameLenType)
                                                                      << ")\n";

  // maxBitrate code and corresponding index
  field.clear();
  AppendField(&field, ((lhdc_cie.maxTargetBitrate & A2DP_LHDCV5_MAX_BIT_RATE_MASK) == A2DP_LHDCV5_MAX_BIT_RATE_00),
      "MBR_00 = ");
  AppendField(&field, ((lhdc_cie.maxTargetBitrate & A2DP_LHDCV5_MAX_BIT_RATE_MASK) == A2DP_LHDCV5_MAX_BIT_RATE_11),
      "MBR_30 = ");
  AppendField(&field, ((lhdc_cie.maxTargetBitrate & A2DP_LHDCV5_MAX_BIT_RATE_MASK) == A2DP_LHDCV5_MAX_BIT_RATE_10),
      "MBR_20 = ");
  AppendField(&field, ((lhdc_cie.maxTargetBitrate & A2DP_LHDCV5_MAX_BIT_RATE_MASK) == A2DP_LHDCV5_MAX_BIT_RATE_01),
      "MBR_10 = ");
  AppendField(&field, lhdcV5_get_maxBitrate_index(&maxBitrate_Idx, &lhdc_cie),
      lhdcV5_quality_index_toString(maxBitrate_Idx));
  res << "\tMax target-bitrate: " << field << " (" << loghex((lhdc_cie.maxTargetBitrate & A2DP_LHDCV5_MAX_BIT_RATE_MASK))
                                                                      << ")\n";

  // minBitrate code and corresponding index
  field.clear();
  AppendField(&field, ((lhdc_cie.minTargetBitrate & A2DP_LHDCV5_MIN_BIT_RATE_MASK) == A2DP_LHDCV5_MIN_BIT_RATE_11),
      "mBR_C0 = ");
  AppendField(&field, ((lhdc_cie.minTargetBitrate & A2DP_LHDCV5_MIN_BIT_RATE_MASK) == A2DP_LHDCV5_MIN_BIT_RATE_10),
      "mBR_80 = ");
  AppendField(&field, ((lhdc_cie.minTargetBitrate & A2DP_LHDCV5_MIN_BIT_RATE_MASK) == A2DP_LHDCV5_MIN_BIT_RATE_01),
      "mBR_40 = ");
  AppendField(&field, ((lhdc_cie.minTargetBitrate & A2DP_LHDCV5_MIN_BIT_RATE_MASK) == A2DP_LHDCV5_MIN_BIT_RATE_00),
      "mBR_00 = ");
  AppendField(&field, lhdcV5_get_minBitrate_index(&minBitrate_Idx, &lhdc_cie),
      lhdcV5_quality_index_toString(minBitrate_Idx));
  res << "\tMin target-bitrate: " << field << " (" << loghex((lhdc_cie.minTargetBitrate & A2DP_LHDCV5_MIN_BIT_RATE_MASK))
                                                                      << ")\n";

  return res.str();
}

const tA2DP_ENCODER_INTERFACE* A2DP_VendorGetEncoderInterfaceLhdcV5(
    const uint8_t* p_codec_info) {

  if (p_codec_info == nullptr) {
    log::error( ": nullptr input");
    return NULL;
  }

  if (!A2DP_IsVendorSourceCodecValidLhdcV5(p_codec_info)) return NULL;

  return &a2dp_encoder_interface_lhdcv5;
}

#ifdef HAS_LHDCV5_SINK
const tA2DP_DECODER_INTERFACE* A2DP_VendorGetDecoderInterfaceLhdcV5(
    const uint8_t* p_codec_info) {
  if (p_codec_info == nullptr) {
    log::error( ": nullptr input");
    return NULL;
  }

  if (!A2DP_IsVendorSinkCodecValidLhdcV5(p_codec_info)) return NULL;

  return &a2dp_decoder_interface_lhdcv5;
}
#endif

bool A2DP_VendorAdjustCodecLhdcV5(uint8_t* p_codec_info) {
  tA2DP_LHDCV5_CIE cfg_cie;

  if (p_codec_info == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  // Nothing to do: just verify the codec info is valid
  if (A2DP_ParseInfoLhdcV5(&cfg_cie, p_codec_info, true, IS_SRC) != A2DP_SUCCESS) {
    return false;
  }

  return true;
}

btav_a2dp_codec_index_t A2DP_VendorSourceCodecIndexLhdcV5(
    UNUSED_ATTR const uint8_t* p_codec_info) {
  return BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV5;
}

#ifdef HAS_LHDCV5_SINK
btav_a2dp_codec_index_t A2DP_VendorSinkCodecIndexLhdcV5(
    UNUSED_ATTR const uint8_t* p_codec_info) {
  return BTAV_A2DP_CODEC_INDEX_SINK_LHDCV5;
}
#endif

const char* A2DP_VendorCodecIndexStrLhdcV5(void) { return "LHDC V5"; }

#ifdef HAS_LHDCV5_SINK
const char* A2DP_VendorCodecIndexStrLhdcV5Sink(void) { return "LHDC V5 SINK"; }
#endif

bool A2DP_VendorInitCodecConfigLhdcV5(AvdtpSepConfig* p_cfg) {
  const tA2DP_LHDCV5_CIE* p_a2dp_lhdcv5_caps = NULL;

  if (p_cfg == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  p_a2dp_lhdcv5_caps = &a2dp_lhdcv5_source_caps;

  if (A2DP_BuildInfoLhdcV5(AVDT_MEDIA_TYPE_AUDIO, p_a2dp_lhdcv5_caps,
      p_cfg->codec_info) != A2DP_SUCCESS) {
    return false;
  }

#if (BTA_AV_CO_CP_SCMS_T == TRUE)
  /* Content protection info - support SCMS-T */
  uint8_t* p = p_cfg->protect_info;
  *p++ = AVDT_CP_LOSC;
  UINT16_TO_STREAM(p, AVDT_CP_SCMS_T_ID);
  p_cfg->num_protect = 1;
#endif

  return true;
}

bool A2DP_VendorInitCodecConfigLhdcV5Sink(AvdtpSepConfig* p_cfg) {
  if (p_cfg == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  if (A2DP_BuildInfoLhdcV5(AVDT_MEDIA_TYPE_AUDIO, &a2dp_lhdcv5_sink_caps,
      p_cfg->codec_info) != A2DP_SUCCESS) {
    return false;
  }

  return true;
}

UNUSED_ATTR static void build_codec_config(const tA2DP_LHDCV5_CIE& config_cie,
    btav_a2dp_codec_config_t* result) {
  if (result == nullptr) {
    log::error( ": nullptr input");
    return;
  }

  // sample rate
  result->sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_NONE;
  if (config_cie.sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_44100)
    result->sample_rate |= BTAV_A2DP_CODEC_SAMPLE_RATE_44100;
  if (config_cie.sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_48000)
    result->sample_rate |= BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
  if (config_cie.sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_96000)
    result->sample_rate |= BTAV_A2DP_CODEC_SAMPLE_RATE_96000;
  if (config_cie.sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_192000)
    result->sample_rate |= BTAV_A2DP_CODEC_SAMPLE_RATE_192000;

  // bits per sample
  result->bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE;
  if (config_cie.bitsPerSample & A2DP_LHDCV5_BIT_FMT_16)
    result->bits_per_sample |= BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
  if (config_cie.bitsPerSample & A2DP_LHDCV5_BIT_FMT_24)
    result->bits_per_sample |= BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24;

  // channel mode
  result->channel_mode = BTAV_A2DP_CODEC_CHANNEL_MODE_NONE;
  if (config_cie.channelMode & A2DP_LHDCV5_CHANNEL_MODE_MONO)
    result->channel_mode |= BTAV_A2DP_CODEC_CHANNEL_MODE_MONO;
  if (config_cie.channelMode &
      (A2DP_LHDCV5_CHANNEL_MODE_DUAL | A2DP_LHDCV5_CHANNEL_MODE_STEREO)) {
    result->channel_mode |= BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO;
  }
}

A2dpCodecConfigLhdcV5Source::A2dpCodecConfigLhdcV5Source(
    btav_a2dp_codec_priority_t codec_priority)
: A2dpCodecConfigLhdcV5Base(BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV5,
    A2DP_VendorCodecIndexStrLhdcV5(),
    codec_priority, true) {

  // Compute the local capability
  codec_local_capability_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_NONE;
  if (a2dp_lhdcv5_source_caps.sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_44100) {
    codec_local_capability_.sample_rate |= BTAV_A2DP_CODEC_SAMPLE_RATE_44100;
  }
  if (a2dp_lhdcv5_source_caps.sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_48000) {
    codec_local_capability_.sample_rate |= BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
  }
  if (a2dp_lhdcv5_source_caps.sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_96000) {
    codec_local_capability_.sample_rate |= BTAV_A2DP_CODEC_SAMPLE_RATE_96000;
  }
  if (a2dp_lhdcv5_source_caps.sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_192000) {
    codec_local_capability_.sample_rate |= BTAV_A2DP_CODEC_SAMPLE_RATE_192000;
  }

  codec_local_capability_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE;
  if (a2dp_lhdcv5_source_caps.bitsPerSample & A2DP_LHDCV5_BIT_FMT_16) {
    codec_local_capability_.bits_per_sample |= BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
  }
  if (a2dp_lhdcv5_source_caps.bitsPerSample & A2DP_LHDCV5_BIT_FMT_24) {
    codec_local_capability_.bits_per_sample |= BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24;
  }

  codec_local_capability_.channel_mode = BTAV_A2DP_CODEC_CHANNEL_MODE_NONE;
  if (a2dp_lhdcv5_source_caps.channelMode & A2DP_LHDCV5_CHANNEL_MODE_MONO) {
    codec_local_capability_.channel_mode |= BTAV_A2DP_CODEC_CHANNEL_MODE_MONO;
  }
  if (a2dp_lhdcv5_source_caps.channelMode & A2DP_LHDCV5_CHANNEL_MODE_DUAL) {
    codec_local_capability_.channel_mode |= BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO;
  }
  if (a2dp_lhdcv5_source_caps.channelMode & A2DP_LHDCV5_CHANNEL_MODE_STEREO) {
    codec_local_capability_.channel_mode |= BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO;
  }
}

A2dpCodecConfigLhdcV5Source::~A2dpCodecConfigLhdcV5Source() {}

bool A2dpCodecConfigLhdcV5Source::init() {
  // Load the encoder
  if (!A2DP_VendorLoadEncoderLhdcV5()) {
    log::error( ": cannot load the encoder");
    return false;
  }

  return true;
}

bool A2dpCodecConfigLhdcV5Source::useRtpHeaderMarkerBit() const { return false; }

//
// Selects the best sample rate from |sampleRate|.
// The result is stored in |p_result| and |p_codec_config|.
// Returns true if a selection was made, otherwise false.
//
static bool select_best_sample_rate(uint8_t sampleRate,
    tA2DP_LHDCV5_CIE* p_result,
    btav_a2dp_codec_config_t* p_codec_config) {
  if (p_codec_config == nullptr || p_result == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  // LHDC V5 priority: 48K > 44.1K > 96K > 192K > others(min to max)
  if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_48000) {
    p_result->sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_48000;
    p_codec_config->sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
    return true;
  }
  if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_44100) {
    p_result->sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_44100;
    p_codec_config->sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_44100;
    return true;
  }
  if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_96000) {
    p_result->sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_96000;
    p_codec_config->sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_96000;
    return true;
  }
  if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_192000) {
    p_result->sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_192000;
    p_codec_config->sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_192000;
    return true;
  }
  return false;
}

//
// Selects the audio sample rate from |p_codec_audio_config|.
// |sampleRate| contains the capability.
// The result is stored in |p_result| and |p_codec_config|.
// Returns true if a selection was made, otherwise false.
//
static bool select_audio_sample_rate(
    const btav_a2dp_codec_config_t* p_codec_audio_config, uint8_t sampleRate,
    tA2DP_LHDCV5_CIE* p_result, btav_a2dp_codec_config_t* p_codec_config) {
  if (p_codec_audio_config == nullptr || p_result == nullptr || p_codec_config == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  // LHDC V5 priority: 48K > 44.1K > 96K > 192K > others(min to max)
  switch (p_codec_audio_config->sample_rate) {
  case BTAV_A2DP_CODEC_SAMPLE_RATE_48000:
    if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_48000) {
      p_result->sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_48000;
      p_codec_config->sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
      return true;
    }
    break;
  case BTAV_A2DP_CODEC_SAMPLE_RATE_44100:
    if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_44100) {
      p_result->sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_44100;
      p_codec_config->sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_44100;
      return true;
    }
    break;
  case BTAV_A2DP_CODEC_SAMPLE_RATE_96000:
    if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_96000) {
      p_result->sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_96000;
      p_codec_config->sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_96000;
      return true;
    }
    break;
  case BTAV_A2DP_CODEC_SAMPLE_RATE_192000:
    if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_192000) {
      p_result->sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_192000;
      p_codec_config->sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_192000;
      return true;
    }
    break;
  case BTAV_A2DP_CODEC_SAMPLE_RATE_16000:
  case BTAV_A2DP_CODEC_SAMPLE_RATE_24000:
  case BTAV_A2DP_CODEC_SAMPLE_RATE_176400:
  case BTAV_A2DP_CODEC_SAMPLE_RATE_88200:
  case BTAV_A2DP_CODEC_SAMPLE_RATE_NONE:
    break;
  }
  return false;
}

//
// Selects the best bits per sample from |bitsPerSample|.
// |bitsPerSample| contains the capability.
// The result is stored in |p_result| and |p_codec_config|.
// Returns true if a selection was made, otherwise false.
//
static bool select_best_bits_per_sample(
    uint8_t bitsPerSample, tA2DP_LHDCV5_CIE* p_result,
    btav_a2dp_codec_config_t* p_codec_config) {

  if (p_result == nullptr || p_codec_config == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  // LHDC V5 priority: 24 > 16 > 32
  if (bitsPerSample & A2DP_LHDCV5_BIT_FMT_24) {
    p_codec_config->bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24;
    p_result->bitsPerSample = A2DP_LHDCV5_BIT_FMT_24;
    return true;
  }
  if (bitsPerSample & A2DP_LHDCV5_BIT_FMT_16) {
    p_codec_config->bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
    p_result->bitsPerSample = A2DP_LHDCV5_BIT_FMT_16;
    return true;
  }
  return false;
}

//
// Selects the audio bits per sample from |p_codec_audio_config|.
// |bitsPerSample| contains the capability.
// The result is stored in |p_result| and |p_codec_config|.
// Returns true if a selection was made, otherwise false.
//
static bool select_audio_bits_per_sample(
    const btav_a2dp_codec_config_t* p_codec_audio_config,
    uint8_t bitsPerSample, tA2DP_LHDCV5_CIE* p_result,
    btav_a2dp_codec_config_t* p_codec_config) {

  if (p_codec_audio_config == nullptr || p_result == nullptr || p_codec_config == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  // LHDC V5 priority: 24 > 16 > 32
  switch (p_codec_audio_config->bits_per_sample) {
  case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24:
    if (bitsPerSample & A2DP_LHDCV5_BIT_FMT_24) {
      p_codec_config->bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24;
      p_result->bitsPerSample = A2DP_LHDCV5_BIT_FMT_24;
      return true;
    }
    break;
  case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16:
    if (bitsPerSample & A2DP_LHDCV5_BIT_FMT_16) {
      p_codec_config->bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
      p_result->bitsPerSample = A2DP_LHDCV5_BIT_FMT_16;
      return true;
    }
    break;
  case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_32:
  case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE:
    break;
  }
  return false;
}

tA2DP_STATUS A2dpCodecConfigLhdcV5Base::setCodecConfig(const uint8_t* p_peer_codec_info,
    bool is_capability,
    uint8_t* p_result_codec_config) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  tA2DP_LHDCV5_CIE sink_info_cie;
  tA2DP_LHDCV5_CIE result_config_cie;
  uint8_t sampleRate = 0;
  uint8_t bitsPerSample = 0;
  uint8_t sampleRate_cap = 0;
  uint8_t bitsPerSample_cap = 0;
  uint8_t frameLenType = 0;
  uint8_t frameLenSelect = 0;
  bool hasFeature = false;
  bool hasUserSet = false;
  uint8_t qualityMode = 0;
  uint8_t maxBitrate_Idx = 0;
  uint8_t minBitrate_Idx = 0;
  const tA2DP_LHDCV5_CIE* p_a2dp_lhdcv5_caps = NULL;

  tA2DP_STATUS status;

  if (is_capability) {
    p_a2dp_lhdcv5_caps = (is_source_) ? &a2dp_lhdcv5_source_caps : &a2dp_lhdcv5_sink_default_caps;
  } else {
    p_a2dp_lhdcv5_caps = (is_source_) ? &a2dp_lhdcv5_source_caps : &a2dp_lhdcv5_sink_caps;
  }

  // Save the internal state
  btav_a2dp_codec_config_t saved_codec_config = codec_config_;
  btav_a2dp_codec_config_t saved_codec_selectable_capability =
      codec_selectable_capability_;
  btav_a2dp_codec_config_t saved_codec_user_config = codec_user_config_;
  btav_a2dp_codec_config_t saved_codec_audio_config = codec_audio_config_;
  uint8_t saved_ota_codec_config[AVDT_CODEC_SIZE];
  uint8_t saved_ota_codec_peer_capability[AVDT_CODEC_SIZE];
  uint8_t saved_ota_codec_peer_config[AVDT_CODEC_SIZE];
  memcpy(saved_ota_codec_config, ota_codec_config_, sizeof(ota_codec_config_));
  memcpy(saved_ota_codec_peer_capability, ota_codec_peer_capability_,
      sizeof(ota_codec_peer_capability_));
  memcpy(saved_ota_codec_peer_config, ota_codec_peer_config_,
      sizeof(ota_codec_peer_config_));

  tA2DP_CODEC_CONFIGS_PACK allCfgPack;
  allCfgPack._codec_config_ = &codec_config_;
  allCfgPack._codec_local_capability_ = &codec_local_capability_;
  allCfgPack._codec_selectable_capability_ = &codec_selectable_capability_;
  allCfgPack._codec_user_config_ = &codec_user_config_;
  allCfgPack._codec_audio_config_ = &codec_audio_config_;

  if (p_peer_codec_info == nullptr || p_result_codec_config == nullptr) {
    log::error( ": nullptr input");
    goto fail;
  }

  // is_source: A2DP role: SRC or SNK
  // is_capability: initiator of SET_CONFIGURATION
  log::info( ": is_source:{} is_cap:{}", is_source_, is_capability);

  status = A2DP_ParseInfoLhdcV5(&sink_info_cie, p_peer_codec_info, is_capability, is_source_);
  if (status != A2DP_SUCCESS) {
    log::error( ": can't parse peer's Sink capabilities: error = {}",
         status);
    goto fail;
  }

  if (is_source_) {
    sampleRate_cap = (a2dp_lhdcv5_source_caps.sampleRate & sink_info_cie.sampleRate);
  } else {
    sampleRate_cap = (a2dp_lhdcv5_sink_caps.sampleRate & sink_info_cie.sampleRate);
  }

  if (is_source_) {
    bitsPerSample_cap = (a2dp_lhdcv5_source_caps.bitsPerSample & sink_info_cie.bitsPerSample);
  } else {
    bitsPerSample_cap = (a2dp_lhdcv5_sink_caps.bitsPerSample & sink_info_cie.bitsPerSample);
  }

  //
  // Build the preferred configuration
  //
  memset(&result_config_cie, 0, sizeof(result_config_cie));
  result_config_cie.vendorId = p_a2dp_lhdcv5_caps->vendorId;
  result_config_cie.codecId = p_a2dp_lhdcv5_caps->codecId;
  result_config_cie.version = sink_info_cie.version;

  //
  // Select the sample frequency initialization
  //
  sampleRate = p_a2dp_lhdcv5_caps->sampleRate & sink_info_cie.sampleRate;
  log::info( ": sampleRate: peer:0x{:02X} local:0x{:02X} cap:0x{:02X} user:0x{:02X}",
      sink_info_cie.sampleRate, p_a2dp_lhdcv5_caps->sampleRate,
      sampleRate, codec_user_config_.sample_rate);

  if (is_capability) {
    codec_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_NONE;
    switch (codec_user_config_.sample_rate) {
    case BTAV_A2DP_CODEC_SAMPLE_RATE_44100:
      if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_44100) {
        result_config_cie.sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_44100;
        codec_config_.sample_rate = codec_user_config_.sample_rate;
      }
      break;
    case BTAV_A2DP_CODEC_SAMPLE_RATE_48000:
      if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_48000) {
        result_config_cie.sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_48000;
        codec_config_.sample_rate = codec_user_config_.sample_rate;
      }
      break;
    case BTAV_A2DP_CODEC_SAMPLE_RATE_96000:
      if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_96000) {
        result_config_cie.sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_96000;
        codec_config_.sample_rate = codec_user_config_.sample_rate;
      }
      break;
    case BTAV_A2DP_CODEC_SAMPLE_RATE_192000:
      if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_192000) {
        result_config_cie.sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_192000;
        codec_config_.sample_rate = codec_user_config_.sample_rate;
      }
      break;
    case BTAV_A2DP_CODEC_SAMPLE_RATE_16000:
    case BTAV_A2DP_CODEC_SAMPLE_RATE_24000:
    case BTAV_A2DP_CODEC_SAMPLE_RATE_88200:
    case BTAV_A2DP_CODEC_SAMPLE_RATE_176400:
    case BTAV_A2DP_CODEC_SAMPLE_RATE_NONE:
      codec_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_NONE;
      break;
    }

    // Select the sample frequency if there is no user preference
    do {
      // Compute the selectable capability
      if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_44100)
        codec_selectable_capability_.sample_rate |= BTAV_A2DP_CODEC_SAMPLE_RATE_44100;
      if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_48000)
        codec_selectable_capability_.sample_rate |= BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
      if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_96000)
        codec_selectable_capability_.sample_rate |= BTAV_A2DP_CODEC_SAMPLE_RATE_96000;
      if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_192000)
        codec_selectable_capability_.sample_rate |= BTAV_A2DP_CODEC_SAMPLE_RATE_192000;

      if (codec_config_.sample_rate != BTAV_A2DP_CODEC_SAMPLE_RATE_NONE) {
        log::info( ": sample rate configured by UI successfully 0x{:02X}",
             result_config_cie.sampleRate);
        break;
      }
      // Ignore follows if codec config is setup, otherwise pick a best one from default rules

       // No user preference - try the codec audio config
      if (select_audio_sample_rate(&codec_audio_config_, sampleRate,
          &result_config_cie, &codec_config_)) {
        log::info( ": select sample rate from audio: 0x{:02X}",
            result_config_cie.sampleRate);
        break;
      }

      // No user preference - try the default config
      if (select_best_sample_rate(
          a2dp_lhdcv5_source_default_caps.sampleRate & sink_info_cie.sampleRate,
          &result_config_cie, &codec_config_)) {
        log::info( ": select sample rate by local prefered: 0x{:02X}",
            result_config_cie.sampleRate);
        break;
      }

      // No user preference - use the best match
      if (select_best_sample_rate(sampleRate, &result_config_cie,
          &codec_config_)) {
        log::info( ": select sample rate from best match: 0x{:02X}",
            result_config_cie.sampleRate);
        break;
      }
    } while (false);

    if (codec_config_.sample_rate == BTAV_A2DP_CODEC_SAMPLE_RATE_NONE) {
      log::error(
          ": cannot match sample frequency: local caps = 0x{:02X} "
          "peer info = 0x{:02X}",
           p_a2dp_lhdcv5_caps->sampleRate, sink_info_cie.sampleRate);
      goto fail;
    }
    codec_user_config_.sample_rate = codec_config_.sample_rate;
  } else {
    // SET_CONFIGURATION acceptor
    if (sampleRate == A2DP_LHDCV5_SAMPLING_FREQ_44100) {
      codec_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_44100;
      codec_user_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_44100;
      result_config_cie.sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_44100;
    } else if (sampleRate == A2DP_LHDCV5_SAMPLING_FREQ_48000) {
      codec_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
      codec_user_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
      result_config_cie.sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_48000;
    } else if (sampleRate == A2DP_LHDCV5_SAMPLING_FREQ_96000) {
      codec_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_96000;
      codec_user_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_96000;
      result_config_cie.sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_96000;
    } else if (sampleRate == A2DP_LHDCV5_SAMPLING_FREQ_192000) {
      codec_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_192000;
      codec_user_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_192000;
      result_config_cie.sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_192000;
    } else {
      log::error(
          ": invalid peer sampleRate: 0x{:02X}", sampleRate);
      goto fail;
    }
  }
  log::info( ": => sampleRate(0x{:02X}) = {}",
      result_config_cie.sampleRate,
      lhdcV5_sampleRate_toString(result_config_cie.sampleRate).c_str());

  //
  // Select the bits per sample initialization
  //
  bitsPerSample = p_a2dp_lhdcv5_caps->bitsPerSample & sink_info_cie.bitsPerSample;
  log::info( ": bitsPerSample: peer:0x{:02X} local:0x{:02X} cap:0x{:02X} user:0x{:02X}",
      sink_info_cie.bitsPerSample, p_a2dp_lhdcv5_caps->bitsPerSample,
      bitsPerSample, codec_user_config_.bits_per_sample);

  if (is_capability) {
    codec_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE;
    switch (codec_user_config_.bits_per_sample) {
    case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16:
      if (bitsPerSample & A2DP_LHDCV5_BIT_FMT_16) {
        result_config_cie.bitsPerSample = A2DP_LHDCV5_BIT_FMT_16;
        codec_config_.bits_per_sample = codec_user_config_.bits_per_sample;
      }
      break;
    case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24:
      if (bitsPerSample & A2DP_LHDCV5_BIT_FMT_24) {
        result_config_cie.bitsPerSample = A2DP_LHDCV5_BIT_FMT_24;
        codec_config_.bits_per_sample = codec_user_config_.bits_per_sample;
      }
      break;
    case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_32:
    case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE:
      result_config_cie.bitsPerSample = A2DP_LHDCV5_BIT_FMT_NS;
      codec_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE;
      break;
    }

    // Select the bits per sample if there is no user preference
    do {
      // Compute the selectable capability
      if (bitsPerSample & A2DP_LHDCV5_BIT_FMT_16)
        codec_selectable_capability_.bits_per_sample |= BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
      if (bitsPerSample & A2DP_LHDCV5_BIT_FMT_24)
        codec_selectable_capability_.bits_per_sample |= BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24;

      if (codec_config_.bits_per_sample != BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE) {
        log::info( ": bit_per_sample configured by UI successfully 0x{:02X}",
             result_config_cie.bitsPerSample);
        break;
      }
      // Ignore follows if codec config is setup, otherwise pick a best one from default rules

      // No user preference - the the codec audio config
      if (select_audio_bits_per_sample(&codec_audio_config_, bitsPerSample,
          &result_config_cie, &codec_config_)) {
        log::info( ": select bit per sample from audio: 0x{:02X}",
            result_config_cie.bitsPerSample);
        break;
      }

      // No user preference - try the default config
      if (select_best_bits_per_sample(
          a2dp_lhdcv5_source_default_caps.bitsPerSample & sink_info_cie.bitsPerSample,
          &result_config_cie, &codec_config_)) {
        log::info( ": select bit per sample by local preferred: 0x{:02X}",
            result_config_cie.bitsPerSample);
        break;
      }

      // No user preference - use the best match
      if (select_best_bits_per_sample(bitsPerSample, &result_config_cie,
          &codec_config_)) {
        log::info( ": select sample rate from best match: 0x{:02X}",
            result_config_cie.bitsPerSample);
        break;
      }
    } while (false);

    if (codec_config_.bits_per_sample == BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE) {
      log::error(
          ": cannot match bits per sample: local caps = 0x{:02X} "
          "peer info = 0x{:02X}",
           p_a2dp_lhdcv5_caps->bitsPerSample,
          sink_info_cie.bitsPerSample);
      goto fail;
    }
    codec_user_config_.bits_per_sample = codec_config_.bits_per_sample;
  } else {
    // SET_CONFIGURATION acceptor
    if (bitsPerSample == A2DP_LHDCV5_BIT_FMT_16) {
      codec_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
      codec_user_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
      result_config_cie.bitsPerSample = A2DP_LHDCV5_BIT_FMT_16;
    } else if (bitsPerSample == A2DP_LHDCV5_BIT_FMT_24) {
      codec_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24;
      codec_user_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24;
      result_config_cie.bitsPerSample = A2DP_LHDCV5_BIT_FMT_24;
    } else {
      log::error(
          ": invalid peer bitsPerSample: 0x{:02X}", bitsPerSample);
      goto fail;
    }
  }
  log::info( ": => bitsPerSample(0x{:02X}) = {}",
      result_config_cie.bitsPerSample,
      lhdcV5_bitPerSample_toString(result_config_cie.bitsPerSample).c_str());

  // Select the channel mode
  codec_user_config_.channel_mode = BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO;
  codec_selectable_capability_.channel_mode = BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO;
  codec_config_.channel_mode = BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO;
  result_config_cie.channelMode = A2DP_LHDCV5_CHANNEL_MODE_STEREO;
  log::info( ": => channelMode = Only supported stereo");

  //
  // frame length(duration) and frame length select initialization
  //
  frameLenType = p_a2dp_lhdcv5_caps->frameLenType & sink_info_cie.frameLenType;
  log::info( ": frameLenType: peer:0x{:02X} local:0x{:02X} cap:0x{:02X}",
      sink_info_cie.frameLenType,
      p_a2dp_lhdcv5_caps->frameLenType,
      frameLenType);

  frameLenSelect = p_a2dp_lhdcv5_caps->frameLenSelect & sink_info_cie.frameLenSelect;
  log::info( ": (lossy) frameLenSelect: peer:0x{:02X} local:0x{:02X} cap:0x{:02X}",
      sink_info_cie.frameLenSelect,
      p_a2dp_lhdcv5_caps->frameLenSelect,
      frameLenSelect);

  // In the stock V5 wire format, zero selects the mandatory 5 ms frame.
  // The public AOSP 16 integration also defines this value as zero, so it
  // must not be rejected as an unspecified selection.

  if (is_capability) {
    // SET_CONFIGURATION initiator
    if ((frameLenType & A2DP_LHDCV5_FRAME_LEN_5MS) == 0) {
      // 5ms is mandatory in SRC and SNK both sides
      log::error( ": fatal! frame_length cap 5ms(0x{:2X}) is mandatory", A2DP_LHDCV5_FRAME_LEN_5MS);
      goto fail;
    }

    result_config_cie.frameLenSelect = frameLenSelect;

    if (frameLenType == A2DP_LHDCV5_FRAME_LEN_5MS) {
      // if only 5ms is intersected, select 5ms directly
      result_config_cie.frameLenType = A2DP_LHDCV5_FRAME_LEN_5MS;
      result_config_cie.frameLenSelect = A2DP_LHDCV5_FRAME_LEN_SELECT_5MS;
    } else {
      //// exam frame_length with frameLenSelect matching
      // FLenSelect == 2.5ms:
      if (frameLenSelect == A2DP_LHDCV5_FRAME_LEN_SELECT_2P5MS) {
        if ((frameLenType & A2DP_LHDCV5_FRAME_LEN_2P5MS) != 0) {
          result_config_cie.frameLenType = A2DP_LHDCV5_FRAME_LEN_2P5MS;
        } else {
          log::error( ": invalid 2.5ms frameLenSelect:0x{:02X}, frameLengthCap:0x{:02X} not supported",
              frameLenSelect, frameLenType);
          goto fail;
        }
      }

      // FLenSelect == 5ms:
      if (frameLenSelect == A2DP_LHDCV5_FRAME_LEN_SELECT_5MS) {
        if ((frameLenType & A2DP_LHDCV5_FRAME_LEN_5MS) != 0) {
          result_config_cie.frameLenType = A2DP_LHDCV5_FRAME_LEN_5MS;
        } else {
          log::error( ": invalid 5ms frameLenSelect:0x{:02X}, frameLengthCap:0x{:02X} not supported",
              frameLenSelect, frameLenType);
          goto fail;
        }
      }

      // FLenSelect == 10ms:
      if (frameLenSelect == A2DP_LHDCV5_FRAME_LEN_SELECT_10MS) {
        if ((frameLenType & A2DP_LHDCV5_FRAME_LEN_10MS) != 0) {
          result_config_cie.frameLenType = A2DP_LHDCV5_FRAME_LEN_10MS;
        } else {
          log::error( ": invalid 10ms frameLenSelect:0x{:02X}, frameLengthCap:0x{:02X} not supported",
              frameLenSelect, frameLenType);
          goto fail;
        }
      }
    }
  } else {
    // SET_CONFIGURATION acceptor
    if (frameLenType == A2DP_LHDCV5_FRAME_LEN_NS) {
      // peer bugged: frameLenType is not available in local.
      log::error( ": invalid peer frameLenType:0x{:02X} {}!", frameLenType,
          lhdcV5_frameLenType_toString(frameLenType).c_str());
      goto fail;
    }

    result_config_cie.frameLenSelect = frameLenSelect;
    result_config_cie.frameLenType = frameLenType;
  }
  log::info( ": => (lossy) frameLenSelect:(0x{:02X}) = {}",
      result_config_cie.frameLenSelect,
      lhdcV5_frameLenSelect_toString(result_config_cie.frameLenSelect).c_str());

  log::info( ": => frameLenType:(0x{:02X}) = {}",
      result_config_cie.frameLenType,
      lhdcV5_frameLenType_toString(result_config_cie.frameLenType).c_str());

  //
  // Reset LHDC features status if the record is corrupted
  //
  /*******************************************
   * Reset features status by checking the tag on the specifics:
   * features:
   *    low latency, Lossless, lossless raw
   *******************************************/
  //features on specific 3
  if ((codec_user_config_.codec_specific_3 & A2DP_LHDCV5_VENDOR_FEATURE_MASK) != A2DP_LHDCV5_FEATURE_MAGIC_NUM)
  {
    // reset the specific and apply tag
    codec_user_config_.codec_specific_3 = A2DP_LHDCV5_FEATURE_MAGIC_NUM;

    // get previous status of user-control enabling features from codec_config, then restore to user settings
    //
    // Feature: Low latency mode (default UI: previous stored config)
    hasUserSet = A2DP_IsFeatureInCodecConfigLhdcV5(&allCfgPack, LHDCV5_FEATURE_CODE_LL);
    A2DP_UpdateFeatureToA2dpConfigLhdcV5(
        &allCfgPack,
        LHDCV5_FEATURE_CODE_LL,
        A2DP_LHDC_TO_A2DP_CODEC_USER_,
        (hasUserSet?true:false));
    log::info( ": LHDC features tag check fail, default UI status[LL] => {}",  hasUserSet?"true":"false");

    // Feature: Lossless (default UI: OFF in generic versions)
    //                   (default UI: ON in MTK platform version)
    hasUserSet = false;
    A2DP_UpdateFeatureToA2dpConfigLhdcV5(
        &allCfgPack,
        LHDCV5_FEATURE_CODE_LLESS,
        A2DP_LHDC_TO_A2DP_CODEC_USER_,
        (hasUserSet?true:false));
    log::info( ": LHDC features tag check fail, default UI status[LLESS] => {}",  hasUserSet?"true":"false");

    // Feature: Lossless Raw(default UI: OFF)
    hasUserSet = false;
    A2DP_UpdateFeatureToA2dpConfigLhdcV5(
        &allCfgPack,
        LHDCV5_FEATURE_CODE_LLESS_RAW,
        A2DP_LHDC_TO_A2DP_CODEC_USER_,
        (hasUserSet?true:false));
    log::info( ": LHDC features tag check fail, default UI status[LLESS Raw] => {}",  hasUserSet?"true":"false");
  }

  /*************************************************
   *  quality mode initialization
   *************************************************/
  if ((codec_user_config_.codec_specific_1 & A2DP_LHDCV5_QUALITY_CMD_MASK) != A2DP_LHDCV5_QUALITY_MAGIC_NUM) {
    codec_user_config_.codec_specific_1 &= ~(A2DP_LHDCV5_QUALITY_CMD_MASK | A2DP_LHDCV5_QUALITY_MASK);
    codec_user_config_.codec_specific_1 |= (A2DP_LHDCV5_QUALITY_MAGIC_NUM | A2DP_LHDCV5_QUALITY_ABR);
    log::info( ": codec_specific_1 tag not match, use default Quality Mode: 0x{:02X}({})",
        A2DP_LHDCV5_QUALITY_ABR,
        lhdcV5_quality_index_toString(A2DP_LHDCV5_QUALITY_ABR).c_str());
  }
  qualityMode = (uint8_t)codec_user_config_.codec_specific_1 & A2DP_LHDCV5_QUALITY_MASK;
  log::info(": => temp quality_mode = 0x{:02X}({})",  qualityMode,
      lhdcV5_quality_index_toString(qualityMode).c_str());

  /*******************************************
   *  Low Latency ON/OFF:
   *    SRC control
   *******************************************/
  {
    hasFeature = (p_a2dp_lhdcv5_caps->hasFeatureLL & sink_info_cie.hasFeatureLL);
    // reset first
    result_config_cie.hasFeatureLL = false;
    hasUserSet = A2DP_IsFeatureInUserConfigLhdcV5(&allCfgPack, LHDCV5_FEATURE_CODE_LL);

    A2DP_UpdateFeatureToA2dpConfigLhdcV5(
        &allCfgPack,
        LHDCV5_FEATURE_CODE_LL,
        (A2DP_LHDC_TO_A2DP_CODEC_CONFIG_ |
            A2DP_LHDC_TO_A2DP_CODEC_SELECT_CAP_ | A2DP_LHDC_TO_A2DP_CODEC_USER_),
        false);
    // update
    if (hasFeature && hasUserSet) {
      result_config_cie.hasFeatureLL = true;
      A2DP_UpdateFeatureToA2dpConfigLhdcV5(
          &allCfgPack,
          LHDCV5_FEATURE_CODE_LL,
          (A2DP_LHDC_TO_A2DP_CODEC_SELECT_CAP_ | A2DP_LHDC_TO_A2DP_CODEC_USER_),
              true);
    }
    log::info( ": featureLL: enabled? <{}> Peer:0x{:02X} Local:0x{:02X} User:{}",
        (result_config_cie.hasFeatureLL?"Y":"N"),
        sink_info_cie.hasFeatureLL,
        p_a2dp_lhdcv5_caps->hasFeatureLL,
        (hasUserSet?"Y":"N"));
  }

  /*******************************************
   *  Lossless ON/OFF:
   *    SRC + is_capability: SRC control
   *    SRC + !is_capability: follow SNK configuration
   *    SNK + is_capability: SNK control
   *    SNK + !is_capability: follow SRC configuration
   *******************************************/
  {
    hasFeature = (p_a2dp_lhdcv5_caps->hasFeatureLLESS & sink_info_cie.hasFeatureLLESS);
    // reset first
    result_config_cie.hasFeatureLLESS = false;
    result_config_cie.hasFeatureLLESS24Bit = false;
    result_config_cie.hasFeatureLLESS96K = false;

    if (is_source_) {
      if (is_capability) {
        // SRC configuration: UI-control
        hasUserSet = A2DP_IsFeatureInUserConfigLhdcV5(&allCfgPack, LHDCV5_FEATURE_CODE_LLESS);
      } else {
        // follow SNK configuration
        hasUserSet = true;
      }
    } else {
      if (is_capability) {
        hasUserSet = false;
        // SNK configuration: able to set lossless raw enable when SRC has frame duration: 2.5ms
        if (sink_info_cie.frameLenSelect & A2DP_LHDCV5_FRAME_LEN_SELECT_2P5MS) {
          hasUserSet = true;
          log::info( ": SNK (is_cap): prefer to lossless ON");
        }
      } else {
        // follow SRC configuration
        hasUserSet = true;
      }
    }

    A2DP_UpdateFeatureToA2dpConfigLhdcV5(
        &allCfgPack,
        LHDCV5_FEATURE_CODE_LLESS,
        (A2DP_LHDC_TO_A2DP_CODEC_CONFIG_ |
            A2DP_LHDC_TO_A2DP_CODEC_SELECT_CAP_ | A2DP_LHDC_TO_A2DP_CODEC_USER_),
        false);

    // update
    if (hasFeature && hasUserSet) {
      result_config_cie.hasFeatureLLESS = true;
      A2DP_UpdateFeatureToA2dpConfigLhdcV5(
          &allCfgPack,
          LHDCV5_FEATURE_CODE_LLESS,
          (A2DP_LHDC_TO_A2DP_CODEC_SELECT_CAP_ | A2DP_LHDC_TO_A2DP_CODEC_USER_),
              true);

      if (p_a2dp_lhdcv5_caps->hasFeatureLLESS24Bit & sink_info_cie.hasFeatureLLESS24Bit) {
        result_config_cie.hasFeatureLLESS24Bit = true;
      }

      if (p_a2dp_lhdcv5_caps->hasFeatureLLESS96K & sink_info_cie.hasFeatureLLESS96K) {
        result_config_cie.hasFeatureLLESS96K = true;
      }
    }

    log::info( ": featureLLESS: enabled? <{}> Peer:0x{:02X} Local:0x{:02X} User:{}",
        (result_config_cie.hasFeatureLLESS?"Y":"N"),
        sink_info_cie.hasFeatureLLESS,
        p_a2dp_lhdcv5_caps->hasFeatureLLESS,
        (hasUserSet?"Y":"N"));

    log::info( ": FeatureLLESS24Bit: supported? <{}> Peer:0x{:02X} Local:0x{:02X}",
        (result_config_cie.hasFeatureLLESS24Bit?"Y":"N"),
        sink_info_cie.hasFeatureLLESS24Bit,
        p_a2dp_lhdcv5_caps->hasFeatureLLESS24Bit);

    log::info( ": FeatureLLESS96K: supported? <{}> Peer:0x{:02X} Local:0x{:02X}",
        (result_config_cie.hasFeatureLLESS96K?"Y":"N"),
        sink_info_cie.hasFeatureLLESS96K,
        p_a2dp_lhdcv5_caps->hasFeatureLLESS96K);
  }

  /*******************************************
   *  Lossless Raw ON/OFF:
   *    SRC + is_capability: SRC control
   *    SRC + !is_capability: follow SNK configuration
   *    SNK + is_capability: SNK control
   *    SNK + !is_capability: follow SRC configuration
   *******************************************/
  {
    hasFeature = (p_a2dp_lhdcv5_caps->hasFeatureLLESSRaw & sink_info_cie.hasFeatureLLESSRaw);
    // reset first
    result_config_cie.hasFeatureLLESSRaw = false;

    if (is_source_) {
      if (is_capability) {
        // SRC configuration: UI-control
        hasUserSet = A2DP_IsFeatureInUserConfigLhdcV5(&allCfgPack, LHDCV5_FEATURE_CODE_LLESS_RAW);
      } else {
        // follow SNK configuration
        hasUserSet = true;
      }
    } else {
      if (is_capability) {
        hasUserSet = false;
        // SNK configuration: able to set lossless raw enable when SRC has frame duration: 2.5ms
        if (sink_info_cie.frameLenSelect & A2DP_LHDCV5_FRAME_LEN_SELECT_2P5MS) {
          hasUserSet = true;
          log::info( ": SNK (is_cap): prefer to lossless Raw ON");
        }
      } else {
        // follow SRC configuration
        hasUserSet = true;
      }
    }

    A2DP_UpdateFeatureToA2dpConfigLhdcV5(
        &allCfgPack,
        LHDCV5_FEATURE_CODE_LLESS_RAW,
        (A2DP_LHDC_TO_A2DP_CODEC_CONFIG_ |
            A2DP_LHDC_TO_A2DP_CODEC_SELECT_CAP_ | A2DP_LHDC_TO_A2DP_CODEC_USER_),
        false);

    // update, lossless must be enabled first for raw mode
    if (hasFeature && hasUserSet && result_config_cie.hasFeatureLLESS) {
      result_config_cie.hasFeatureLLESSRaw = true;
      A2DP_UpdateFeatureToA2dpConfigLhdcV5(
          &allCfgPack,
          LHDCV5_FEATURE_CODE_LLESS_RAW,
          (A2DP_LHDC_TO_A2DP_CODEC_SELECT_CAP_ | A2DP_LHDC_TO_A2DP_CODEC_USER_),
              true);
    }

    log::info( ": featureLLESSRaw: enabled? <{}> Peer:0x{:02X} Local:0x{:02X} User:{}",
        (result_config_cie.hasFeatureLLESSRaw?"Y":"N"),
        sink_info_cie.hasFeatureLLESSRaw,
        p_a2dp_lhdcv5_caps->hasFeatureLLESSRaw,
        (hasUserSet?"Y":"N"));
  }

  //
  // max/min bitrate
  //
  // newMmBR
  result_config_cie.hasFeatureNewMmBR = p_a2dp_lhdcv5_caps->hasFeatureNewMmBR & sink_info_cie.hasFeatureNewMmBR;
  log::info( ": => newMmBR: peer:{} local:{} => {}",
      sink_info_cie.hasFeatureNewMmBR,
      p_a2dp_lhdcv5_caps->hasFeatureNewMmBR,
      result_config_cie.hasFeatureNewMmBR);

  // exMBR
  result_config_cie.exMBR = p_a2dp_lhdcv5_caps->exMBR & sink_info_cie.exMBR;
  log::info( ": => exMBR: peer:0x{:02X} local:0x{:02X} => 0x{:02X}({})",
      sink_info_cie.exMBR,
      p_a2dp_lhdcv5_caps->exMBR,
      result_config_cie.exMBR,
      lhdcV5_exMBR_toString(result_config_cie.exMBR).c_str());

  // maxTargetBitrate
  result_config_cie.maxTargetBitrate = p_a2dp_lhdcv5_caps->maxTargetBitrate & sink_info_cie.maxTargetBitrate;
  log::info( ": => MaxBitRate Cap: peer:0x{:02X} local:0x{:02X} => 0x{:02X}({})",
      sink_info_cie.maxTargetBitrate,
      p_a2dp_lhdcv5_caps->maxTargetBitrate,
      result_config_cie.maxTargetBitrate,
      lhdcV5_MaxTargetBitRate_toString(result_config_cie.maxTargetBitrate).c_str());

  // minTargetBitrate
  result_config_cie.minTargetBitrate = p_a2dp_lhdcv5_caps->minTargetBitrate & sink_info_cie.minTargetBitrate;
  log::info( ": => MinBitRate Cap: peer:0x{:02X} local:0x{:02X} => 0x{:02X}({})",
      sink_info_cie.minTargetBitrate,
      p_a2dp_lhdcv5_caps->minTargetBitrate,
      result_config_cie.minTargetBitrate,
      lhdcV5_MinTargetBitRate_toString(result_config_cie.minTargetBitrate).c_str());

  // get the index of target maxBitrate
  if (!lhdcV5_get_maxBitrate_index(&maxBitrate_Idx, &result_config_cie)) {
    log::error( ": unknown maxBitrate index!");
    goto fail;
  }

  // get the index of target minBitrate
  if (!lhdcV5_get_minBitrate_index(&minBitrate_Idx, &result_config_cie)) {
    log::error( ": unknown minBitrate index!");
    goto fail;
  }

  // when lossless disabled,
  // if minBitrate > maxBitrate, downgrade minBitrate
  if (result_config_cie.hasFeatureLLESS == false) {
    if (minBitrate_Idx > maxBitrate_Idx) {
      minBitrate_Idx = maxBitrate_Idx;
    }
  }

  // update max/min bitrate index to UI
  codec_user_config_.codec_specific_1 &= ~(A2DP_LHDCV5_UI_MAX_BITRATE_MASK);
  codec_user_config_.codec_specific_1 |= (maxBitrate_Idx << A2DP_LHDCV5_UI_MAX_BITRATE_SHIFT_BIT);

  codec_user_config_.codec_specific_1 &= ~(A2DP_LHDCV5_UI_MIN_BITRATE_MASK);
  codec_user_config_.codec_specific_1 |= (minBitrate_Idx << A2DP_LHDCV5_UI_MIN_BITRATE_SHIFT_BIT);

  // improved low bitrate: br128kbps
  result_config_cie.hasFeaturebr128kbps = p_a2dp_lhdcv5_caps->hasFeaturebr128kbps & sink_info_cie.hasFeaturebr128kbps;
  log::info( ": => br128kbps: peer:{} local:{} => {}",
      sink_info_cie.hasFeaturebr128kbps,
      p_a2dp_lhdcv5_caps->hasFeaturebr128kbps,
      result_config_cie.hasFeaturebr128kbps);


  //
  // operation rule: lossless audio format (sample_rate, bits-per-sample) re-adjustion
  // Lossless audio format:
  //    1. 48KHz + 16Bits (requires hasFeatureLLESS only)
  //    2. 48KHz + 24Bits (requires additional: hasFeatureLLESS24Bit)
  //    3. 96KHz + 24Bits (requires additional: hasFeatureLLESS24Bit + hasFeatureLLESS96K)
  //    4. 96KHz + 16Bits (not supported!)
  //
  if (result_config_cie.hasFeatureLLESS == true) {
    if (is_capability) {
      //// determine sample_rate:
      if (result_config_cie.hasFeatureLLESS96K == false) {
        // capability of lossless96K is false, only 48KHz sample rate is valid
        if (sampleRate_cap & A2DP_LHDCV5_SAMPLING_FREQ_48000) {
          codec_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
          codec_user_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
          result_config_cie.sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_48000;
          log::info( ": (lossless96K false): valid sample_rate: 48KHz");
        } else {
          log::error( ": (lossless96K false): cannot pick valid sample_rate!");
          goto fail;
        }
      } else {
        // capability of lossless96K is true, check if current sample rate is valid
        if (codec_user_config_.sample_rate != BTAV_A2DP_CODEC_SAMPLE_RATE_48000 &&
            codec_user_config_.sample_rate != BTAV_A2DP_CODEC_SAMPLE_RATE_96000) {
          // sample rate is not valid in lossless, pick default 48KHz prior to 96KHz
          if (sampleRate_cap & A2DP_LHDCV5_SAMPLING_FREQ_48000) {
            codec_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
            codec_user_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
            result_config_cie.sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_48000;
            log::info( ": (lossless96K true): pick default sample_rate: 48KHz");
          } else if (sampleRate_cap & A2DP_LHDCV5_SAMPLING_FREQ_96000) {
            codec_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_96000;
            codec_user_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_96000;
            result_config_cie.sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_96000;
            log::info( ": (lossless96K true): pick default sample_rate: 96KHz");
          } else {
            log::error( ": (lossless96K true): cannot pick valid sample_rate!");
            goto fail;
          }
        } else {
          // apply valid sample_rate for lossless from UI
          log::info( ": (lossless96K true): valid sample_rate: {}",
              lhdcV5_sampleRate_toString(result_config_cie.sampleRate).c_str());
        }
      }
      // final sample_rate for lossless enabled
      log::info( ": => (lossless): final select sample_rate: {}",
          lhdcV5_sampleRate_toString(result_config_cie.sampleRate).c_str());


      //// determine bits_per_sample:
      if (result_config_cie.hasFeatureLLESS24Bit == false) {
        // capability of lossless24Bit is false, only 16 bits is valid
        if (bitsPerSample_cap & A2DP_LHDCV5_BIT_FMT_16) {
          codec_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
          codec_user_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
          result_config_cie.bitsPerSample = A2DP_LHDCV5_BIT_FMT_16;
          log::info( ": (lossless24Bit false): valid bits_per_sample: 16");
        } else {
          log::error( ": (lossless24Bit false): cannot pick valid bits_per_sample!");
          goto fail;
        }
      } else {
        // capability of lossless24Bit is true, check if bits_per_sample is valid
        if (codec_user_config_.bits_per_sample != BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16 &&
            codec_user_config_.bits_per_sample != BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24) {
          // bits_per_sample is not valid in lossless, pick default 24Bit prior to 16Bit
          if (bitsPerSample_cap & A2DP_LHDCV5_BIT_FMT_24) {
            codec_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24;
            codec_user_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24;
            result_config_cie.bitsPerSample = A2DP_LHDCV5_BIT_FMT_24;
            log::info( ": (lossless24Bit true): pick default bits_per_sample: 24");
          } else if (bitsPerSample_cap & A2DP_LHDCV5_BIT_FMT_16) {
            codec_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
            codec_user_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
            result_config_cie.bitsPerSample = A2DP_LHDCV5_BIT_FMT_16;
            log::info( ": (lossless24Bit true): pick default bits_per_sample: 16");
          } else {
            log::error( ": (lossless24Bit true): cannot pick valid bits_per_sample!");
            goto fail;
          }
        } else {
          // apply valid bits-per-sample for lossless from UI
          log::info( ": (lossless24Bit true): valid bits_per_sample: {}",
              lhdcV5_bitPerSample_toString(result_config_cie.bitsPerSample).c_str());
        }
      }
      // final bits-per-sample for lossless enabled
      log::info( ": => (lossless): final select bits_per_sample: {}",
          lhdcV5_bitPerSample_toString(result_config_cie.bitsPerSample).c_str());


      //// unsupported lossless configuration (96KHz + 16Bits) readjust
      //    96KHz+24Bits > 48KHz+16Bits > 48KHz+24Bits
      if (result_config_cie.hasFeatureLLESS96K == true &&
         result_config_cie.sampleRate == A2DP_LHDCV5_SAMPLING_FREQ_96000 &&
         result_config_cie.bitsPerSample == A2DP_LHDCV5_BIT_FMT_16) {
        if (result_config_cie.hasFeatureLLESS24Bit == true) {
          if ((sampleRate_cap & A2DP_LHDCV5_SAMPLING_FREQ_96000) && (
              bitsPerSample_cap & A2DP_LHDCV5_BIT_FMT_24)) {
            // readjust to 96KHz+24Bits
            codec_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24;
            codec_user_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24;
            result_config_cie.bitsPerSample = A2DP_LHDCV5_BIT_FMT_24;
            log::info( ": => lossless96KHz+16Bit (lossless24Bit true): re-adjust to 96KHz+24Bits");
          } else if ((sampleRate_cap & A2DP_LHDCV5_SAMPLING_FREQ_48000) &&
              (bitsPerSample_cap & A2DP_LHDCV5_BIT_FMT_16)) {
            // readjust to 48KHz+16Bits
            codec_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
            codec_user_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
            result_config_cie.sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_48000;

            codec_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
            codec_user_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
            result_config_cie.bitsPerSample = A2DP_LHDCV5_BIT_FMT_16;
            log::info( ": => lossless96KHz+16Bit (lossless24Bit true): re-adjust to 48KHz+16Bits");
          } else if ((sampleRate_cap & A2DP_LHDCV5_SAMPLING_FREQ_48000) &&
              (bitsPerSample_cap & A2DP_LHDCV5_BIT_FMT_24)) {
            // readjust to 48KHz+24Bits
            codec_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
            codec_user_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
            result_config_cie.sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_48000;

            codec_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24;
            codec_user_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24;
            result_config_cie.bitsPerSample = A2DP_LHDCV5_BIT_FMT_24;
            log::info( ": => lossless96KHz+16Bit (lossless24Bit true): re-adjust to 48KHz+24Bits");
          } else {
            log::error( ": lossless96KHz+16Bit (lossless24Bit true): cannot pick valid format!");
            goto fail;
          }
        } else {
          if ((sampleRate_cap & A2DP_LHDCV5_SAMPLING_FREQ_48000) &&
              (bitsPerSample_cap & A2DP_LHDCV5_BIT_FMT_16)) {
            codec_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
            codec_user_config_.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
            result_config_cie.sampleRate = A2DP_LHDCV5_SAMPLING_FREQ_48000;

            codec_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
            codec_user_config_.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
            result_config_cie.bitsPerSample = A2DP_LHDCV5_BIT_FMT_16;
            log::info( ": => lossless96KHz+16Bit (lossless24Bit false): re-adjust to 48KHz+16Bits");
          } else {
            log::error( ": lossless96KHz+16Bit (lossless24Bit false): cannot pick valid format!");
            goto fail;
          }
        }
      }
    } else {
      // peer's set-configuration recheck
      if (result_config_cie.sampleRate == A2DP_LHDCV5_SAMPLING_FREQ_96000 &&
          result_config_cie.bitsPerSample == A2DP_LHDCV5_BIT_FMT_16) {
        log::error( ": lossless96KHz+16Bit: not supported!");
        goto fail;
      }
    }
  }

  //
  // operation rule: lossless frame duration re-adjustion
  //
  // when lossless mode enabled, prefer to adopt frame duration: 2.5ms
  if (result_config_cie.hasFeatureLLESS == true) {
    if (is_capability) {
      if (frameLenType & A2DP_LHDCV5_FRAME_LEN_2P5MS) {
        result_config_cie.frameLenType = A2DP_LHDCV5_FRAME_LEN_2P5MS;
        log::info( ": => (lossless): preferred frame_length: {}",
            lhdcV5_frameLenType_toString(result_config_cie.frameLenType).c_str());
      } else {
        result_config_cie.frameLenType = A2DP_LHDCV5_FRAME_LEN_5MS;
        log::info( ": => (lossless): default frame_length: {}",
            lhdcV5_frameLenType_toString(result_config_cie.frameLenType).c_str());
      }
    } else {
      // peer's unsupported configuration recheck
      if (result_config_cie.frameLenType & A2DP_LHDCV5_FRAME_LEN_10MS) {
        log::error( ": fatal (lossless 10ms): peer config not supported!");
        goto fail;
      } else {
        log::info( ": => (lossless): peer indicated frame_length: {}",
            lhdcV5_frameLenType_toString(result_config_cie.frameLenType).c_str());
      }
    }
  }

  //
  // operation rule: audio quality mode re-adjustion
  //
  if (result_config_cie.hasFeatureLLESS == true) {
    // (lossless enabled): can only ABR
    if ((qualityMode != A2DP_LHDCV5_QUALITY_ABR)) {
      codec_user_config_.codec_specific_1 &= ~(A2DP_LHDCV5_QUALITY_CMD_MASK | A2DP_LHDCV5_QUALITY_MASK);
      codec_user_config_.codec_specific_1 |= (A2DP_LHDCV5_QUALITY_MAGIC_NUM | A2DP_LHDCV5_QUALITY_ABR);
      qualityMode = A2DP_LHDCV5_QUALITY_ABR;
    }
  } else {
    // (lossless disabled): limit quality index by maxBitrate and minBitrate
    if (qualityMode < A2DP_LHDCV5_QUALITY_ABR) {
      // downgrade audio quality according to the max target bit rate
      if (qualityMode > maxBitrate_Idx) {
        codec_user_config_.codec_specific_1 &= ~(A2DP_LHDCV5_QUALITY_CMD_MASK | A2DP_LHDCV5_QUALITY_MASK);
        codec_user_config_.codec_specific_1 |= (A2DP_LHDCV5_QUALITY_MAGIC_NUM | maxBitrate_Idx);
        qualityMode = maxBitrate_Idx;
      }

      // upgrade audio quality according to the min target bit rate
      if (qualityMode < minBitrate_Idx) {
        codec_user_config_.codec_specific_1 &= ~(A2DP_LHDCV5_QUALITY_CMD_MASK | A2DP_LHDCV5_QUALITY_MASK);
        codec_user_config_.codec_specific_1 |= (A2DP_LHDCV5_QUALITY_MAGIC_NUM | minBitrate_Idx);
        qualityMode = minBitrate_Idx;
      }
    }
  }


  // Final configuration note:
  log::info( ": Final => sampleRate: 0x{:02X} = {} (User:{})", result_config_cie.sampleRate,
      lhdcV5_sampleRate_toString(result_config_cie.sampleRate).c_str(),
      codec_user_config_.sample_rate);

  log::info( ": Final => bitsPerSample: 0x{:02X} = {} (User:{})", result_config_cie.bitsPerSample,
      lhdcV5_bitPerSample_toString(result_config_cie.bitsPerSample).c_str(),
      codec_user_config_.bits_per_sample);

  log::info( ": Final => frameLenType: 0x{:02X} = {}", result_config_cie.frameLenType,
      lhdcV5_frameLenType_toString(result_config_cie.frameLenType).c_str());

  log::info( ": Final => (Lossy) frameLenSelect: 0x{:02X} = {}", result_config_cie.frameLenSelect,
      lhdcV5_frameLenSelect_toString(result_config_cie.frameLenSelect).c_str());

  log::info( ": Final => FeatureLLESS: {}", (result_config_cie.hasFeatureLLESS?"Y":"N"));

  log::info( ": Final => FeatureLLESSRaw: {}", (result_config_cie.hasFeatureLLESSRaw?"Y":"N"));

  log::info( ": Final => newMmBR: {}", (result_config_cie.hasFeatureNewMmBR?"Y":"N"));

  log::info( ": Final => exMBR: {}", lhdcV5_exMBR_toString(result_config_cie.exMBR).c_str());

  log::info( ": Final => (Lossy) MaxBitrateIndex: 0x{:02X} = {}", maxBitrate_Idx,
      lhdcV5_quality_index_toString(maxBitrate_Idx).c_str());

  log::info( ": Final => (Lossy) MinBitrateIndex: 0x{:02X} = {}", minBitrate_Idx,
      lhdcV5_quality_index_toString(minBitrate_Idx).c_str());

  log::info( ": Final => quality_mode: 0x{:02X} = {}", qualityMode,
      lhdcV5_quality_index_toString(qualityMode).c_str());


  /* Build final result of configuration to peer */
  if (A2DP_BuildInfoLhdcV5(AVDT_MEDIA_TYPE_AUDIO, &result_config_cie,
      p_result_codec_config) != A2DP_SUCCESS) {
    log::error( ": A2DP build info fail");
    goto fail;
  }
  log::info( ": => final build codec info success!");

  //
  // Copy the codec-specific fields if they are not zero
  //
  if (codec_user_config_.codec_specific_1 != 0)
    codec_config_.codec_specific_1 = codec_user_config_.codec_specific_1;
  if (codec_user_config_.codec_specific_2 != 0)
    codec_config_.codec_specific_2 = codec_user_config_.codec_specific_2;
  if (codec_user_config_.codec_specific_3 != 0)
    codec_config_.codec_specific_3 = codec_user_config_.codec_specific_3;
  if (codec_user_config_.codec_specific_4 != 0)
    codec_config_.codec_specific_4 = codec_user_config_.codec_specific_4;

  // Create a local copy of the peer codec capability, and the
  // result codec config.
  if (is_capability) {
    status = A2DP_BuildInfoLhdcV5(AVDT_MEDIA_TYPE_AUDIO, &sink_info_cie,
        ota_codec_peer_capability_);
  } else {
    status = A2DP_BuildInfoLhdcV5(AVDT_MEDIA_TYPE_AUDIO, &sink_info_cie,
        ota_codec_peer_config_);
  }
  CHECK(status == A2DP_SUCCESS);

  status = A2DP_BuildInfoLhdcV5(AVDT_MEDIA_TYPE_AUDIO, &result_config_cie,
      ota_codec_config_);
  CHECK(status == A2DP_SUCCESS);

  return A2DP_SUCCESS;

fail:
  // Restore the internal state
  codec_config_ = saved_codec_config;
  codec_selectable_capability_ = saved_codec_selectable_capability;
  codec_user_config_ = saved_codec_user_config;
  codec_audio_config_ = saved_codec_audio_config;
  memcpy(ota_codec_config_, saved_ota_codec_config, sizeof(ota_codec_config_));
  memcpy(ota_codec_peer_capability_, saved_ota_codec_peer_capability,
      sizeof(ota_codec_peer_capability_));
  memcpy(ota_codec_peer_config_, saved_ota_codec_peer_config,
      sizeof(ota_codec_peer_config_));

  return A2DP_FAIL;
}

bool A2dpCodecConfigLhdcV5Base::setPeerCodecCapabilities(
    const uint8_t* p_peer_codec_capabilities) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  tA2DP_LHDCV5_CIE peer_info_cie;
  uint8_t sampleRate;
  uint8_t bits_per_sample;
  tA2DP_STATUS status;

  const tA2DP_LHDCV5_CIE* p_a2dp_lhdcv5_caps =
      (is_source_) ? &a2dp_lhdcv5_source_caps : &a2dp_lhdcv5_sink_caps;

  // Save the internal state
  btav_a2dp_codec_config_t saved_codec_selectable_capability =
      codec_selectable_capability_;
  uint8_t saved_ota_codec_peer_capability[AVDT_CODEC_SIZE];
  memcpy(saved_ota_codec_peer_capability, ota_codec_peer_capability_,
      sizeof(ota_codec_peer_capability_));

  log::info( ": is_source_: {}", is_source_);

  if (p_peer_codec_capabilities == nullptr) {
    log::error( ": nullptr input");
    goto fail;
  }

  status = A2DP_ParseInfoLhdcV5(&peer_info_cie, p_peer_codec_capabilities, true, is_source_);
  if (status != A2DP_SUCCESS) {
    log::error( ": can't parse peer's capabilities: error = {}",
         status);
    goto fail;
  }

  // Compute the selectable capability - sample rate
  sampleRate = p_a2dp_lhdcv5_caps->sampleRate & peer_info_cie.sampleRate;
  if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_44100) {
    codec_selectable_capability_.sample_rate |=
        BTAV_A2DP_CODEC_SAMPLE_RATE_44100;
  }
  if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_48000) {
    codec_selectable_capability_.sample_rate |=
        BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
  }
  if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_96000) {
    codec_selectable_capability_.sample_rate |=
        BTAV_A2DP_CODEC_SAMPLE_RATE_96000;
  }
  if (sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_192000) {
    codec_selectable_capability_.sample_rate |=
        BTAV_A2DP_CODEC_SAMPLE_RATE_192000;
  }

  // Compute the selectable capability - bits per sample
  bits_per_sample = p_a2dp_lhdcv5_caps->bitsPerSample & peer_info_cie.bitsPerSample;
  if (bits_per_sample & A2DP_LHDCV5_BIT_FMT_16) {
    codec_selectable_capability_.bits_per_sample |= BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
  }
  if (bits_per_sample & A2DP_LHDCV5_BIT_FMT_24) {
    codec_selectable_capability_.bits_per_sample |= BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24;
  }

  // Compute the selectable capability - channel mode
  codec_selectable_capability_.channel_mode = BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO;

  status = A2DP_BuildInfoLhdcV5(AVDT_MEDIA_TYPE_AUDIO, &peer_info_cie,
      ota_codec_peer_capability_);
  CHECK(status == A2DP_SUCCESS);

  return true;

fail:
  // Restore the internal state
  codec_selectable_capability_ = saved_codec_selectable_capability;
  memcpy(ota_codec_peer_capability_, saved_ota_codec_peer_capability,
      sizeof(ota_codec_peer_capability_));

  return false;
}

////////
//    class implementation for LHDC V5 Sink
////////
#ifdef HAS_LHDCV5_SINK
A2dpCodecConfigLhdcV5Sink::A2dpCodecConfigLhdcV5Sink(
    btav_a2dp_codec_priority_t codec_priority)
: A2dpCodecConfigLhdcV5Base(BTAV_A2DP_CODEC_INDEX_SINK_LHDCV5,
    A2DP_VendorCodecIndexStrLhdcV5Sink(),
    codec_priority, false) {}

A2dpCodecConfigLhdcV5Sink::~A2dpCodecConfigLhdcV5Sink() {}

bool A2dpCodecConfigLhdcV5Sink::init() {
  if (!isValid()) return false;

  // Load the decoder
  if (!A2DP_VendorLoadDecoderLhdcV5()) {
    log::error( ": cannot load the decoder");
    return false;
  }

  return true;
}

bool A2dpCodecConfigLhdcV5Sink::useRtpHeaderMarkerBit() const {
  // TODO: This method applies only to Source codecs
  return false;
}

#if 0
bool A2dpCodecConfigLhdcV5Sink::updateEncoderUserConfig(
    UNUSED_ATTR const tA2DP_ENCODER_INIT_PEER_PARAMS* p_peer_params,
    UNUSED_ATTR bool* p_restart_input, UNUSED_ATTR bool* p_restart_output,
    UNUSED_ATTR bool* p_config_updated) {
  // TODO: This method applies only to Source codecs
  return false;
}
#endif

#if 0
uint64_t A2dpCodecConfigLhdcV5Sink::encoderIntervalMs() const {
  // TODO: This method applies only to Source codecs
  return 0;
}
#endif

#if 0
int A2dpCodecConfigLhdcV5Sink::getEffectiveMtu() const {
  // TODO: This method applies only to Source codecs
  return 0;
}
#endif
#endif

////////
//    APIs for calling from encoder/decoder module - START
////////
bool A2DP_VendorGetMaxBitRateLhdcV5(uint32_t *retval, const int64_t config_specific){
  if (retval == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  *retval = (config_specific & A2DP_LHDCV5_UI_MAX_BITRATE_MASK) >>
      A2DP_LHDCV5_UI_MAX_BITRATE_SHIFT_BIT;

  return true;
}

bool A2DP_VendorGetMinBitRateLhdcV5(uint32_t *retval, const int64_t config_specific){
  if (retval == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  *retval = (config_specific & A2DP_LHDCV5_UI_MIN_BITRATE_MASK) >>
      A2DP_LHDCV5_UI_MIN_BITRATE_SHIFT_BIT;

  return true;
}

bool A2DP_VendorGetVersionLhdcV5(uint32_t *retval, const uint8_t* p_codec_info){
  tA2DP_LHDCV5_CIE lhdc_cie;
  tA2DP_STATUS a2dp_status;

  if (p_codec_info == nullptr || retval == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  // Check whether the codec info contains valid data
  a2dp_status = A2DP_ParseInfoLhdcV5(&lhdc_cie, p_codec_info, false, IS_SRC);
  if (a2dp_status != A2DP_SUCCESS) {
    log::error( ": cannot decode codec information: {}",
        a2dp_status);
    return false;
  }

  *retval = (uint32_t)lhdc_cie.version;

  return true;
}

bool A2DP_VendorGetBitPerSampleLhdcV5(uint8_t *retval, const uint8_t* p_codec_info){
  tA2DP_LHDCV5_CIE lhdc_cie;
  tA2DP_STATUS a2dp_status;

  if (p_codec_info == nullptr || retval == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  // Check whether the codec info contains valid data
  a2dp_status = A2DP_ParseInfoLhdcV5(&lhdc_cie, p_codec_info, false, IS_SRC);
  if (a2dp_status != A2DP_SUCCESS) {
    log::error( ": cannot decode codec information: {}",
        a2dp_status);
    return false;
  }

  *retval = (uint32_t)lhdc_cie.bitsPerSample;

  return true;
}

bool A2DP_VendorGetFrameLenLhdcV5(uint32_t *retval, const uint8_t* p_codec_info){
  tA2DP_LHDCV5_CIE lhdc_cie;
  tA2DP_STATUS a2dp_status;

  if (p_codec_info == nullptr || retval == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  // Check whether the codec info contains valid data
  a2dp_status = A2DP_ParseInfoLhdcV5(&lhdc_cie, p_codec_info, false, IS_SRC);
  if (a2dp_status != A2DP_SUCCESS) {
    log::error( ": cannot decode codec information: {}",
        a2dp_status);
    return false;
  }

  *retval = (uint32_t)lhdc_cie.frameLenType;

  return true;
}

//orig A2DP_VendorGetLowLatencyStateLhdcV5
bool A2DP_VendorHasLLFlagLhdcV5(uint8_t *retval, const uint8_t* p_codec_info){
  tA2DP_LHDCV5_CIE lhdc_cie;
  tA2DP_STATUS a2dp_status;

  if (p_codec_info == nullptr || retval == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  // Check whether the codec info contains valid data
  a2dp_status = A2DP_ParseInfoLhdcV5(&lhdc_cie, p_codec_info, false, IS_SRC);
  if (a2dp_status != A2DP_SUCCESS) {
    log::error( ": cannot decode codec information: {}",
        a2dp_status);
    return false;
  }
  *retval = lhdc_cie.hasFeatureLL ? 1 : 0;

  return true;
}

bool A2DP_VendorHasLLessFlagLhdcV5(uint8_t *retval, const uint8_t* p_codec_info){
  tA2DP_LHDCV5_CIE lhdc_cie;
  tA2DP_STATUS a2dp_status;

  if (p_codec_info == nullptr || retval == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  // Check whether the codec info contains valid data
  a2dp_status = A2DP_ParseInfoLhdcV5(&lhdc_cie, p_codec_info, false, IS_SRC);
  if (a2dp_status != A2DP_SUCCESS) {
    log::error( ": cannot decode codec information: {}",
        a2dp_status);
    return false;
  }
  *retval = lhdc_cie.hasFeatureLLESS ? 1 : 0;

  return true;
}

bool A2DP_VendorHasLLessRawFlagLhdcV5(uint8_t *retval, const uint8_t* p_codec_info){
  tA2DP_LHDCV5_CIE lhdc_cie;
  tA2DP_STATUS a2dp_status;

  if (p_codec_info == nullptr || retval == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  // Check whether the codec info contains valid data
  a2dp_status = A2DP_ParseInfoLhdcV5(&lhdc_cie, p_codec_info, false, IS_SRC);
  if (a2dp_status != A2DP_SUCCESS) {
    log::error( ": cannot decode codec information: {}",
        a2dp_status);
    return false;
  }
  *retval = lhdc_cie.hasFeatureLLESSRaw ? 1 : 0;

  return true;
}

bool A2DP_VendorHasNewMmBRFlagLhdcV5(uint8_t *retval, const uint8_t* p_codec_info){
  tA2DP_LHDCV5_CIE lhdc_cie;
  tA2DP_STATUS a2dp_status;

  if (p_codec_info == nullptr || retval == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  // Check whether the codec info contains valid data
  a2dp_status = A2DP_ParseInfoLhdcV5(&lhdc_cie, p_codec_info, false, IS_SRC);
  if (a2dp_status != A2DP_SUCCESS) {
    log::error( ": cannot decode codec information: {}",
        a2dp_status);
    return false;
  }
  *retval = lhdc_cie.hasFeatureNewMmBR ? 1 : 0;

  return true;
}

bool A2DP_VendorHasBr128kbpsFlagLhdcV5(uint8_t *retval, const uint8_t* p_codec_info){
  tA2DP_LHDCV5_CIE lhdc_cie;
  tA2DP_STATUS a2dp_status;

  if (p_codec_info == nullptr || retval == nullptr) {
    log::error( ": nullptr input");
    return false;
  }

  // Check whether the codec info contains valid data
  a2dp_status = A2DP_ParseInfoLhdcV5(&lhdc_cie, p_codec_info, false, IS_SRC);
  if (a2dp_status != A2DP_SUCCESS) {
    log::error( ": cannot decode codec information: {}",
        a2dp_status);
    return false;
  }
  *retval = lhdc_cie.hasFeaturebr128kbps ? 1 : 0;

  return true;
}
////////
//    APIs for calling from encoder/decoder module - END
////////
