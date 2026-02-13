#pragma once

#include <nvenc/nvEncodeAPI.h>

#include <cstdint>

#include "nvenc_session.h"

enum class EncoderPreset { Fastest, Fast, Medium, Slow, Slowest };

enum class RateControlMode { ConstantQP, VariableBitrate, ConstantBitrate };

struct EncoderConfig {
	EncoderCodec codec			 = EncoderCodec::H264;
	EncoderPreset preset		 = EncoderPreset::Medium;
	RateControlMode rate_control = RateControlMode::ConstantBitrate;

	uint32_t width;
	uint32_t height;
	uint32_t frame_rate_num = 60;
	uint32_t frame_rate_den = 1;

	uint32_t bitrate	 = 8000000;
	uint32_t max_bitrate = 12000000;
	uint32_t gop_length	 = 120;
	uint32_t b_frames	 = 0;

	uint32_t qp = 23;

	bool low_latency = true;
};

class NvencConfig {
  public:
	NvencConfig(NvencSession* session, const EncoderConfig& cfg);
	~NvencConfig() = default;

	NvencSession* session = nullptr;
	EncoderConfig config;
	NV_ENC_INITIALIZE_PARAMS init_params{};
	NV_ENC_CONFIG encode_config{};

  private:
	GUID GetPresetGuid(EncoderPreset preset) const;
	GUID GetCodecGuid(EncoderCodec codec) const;
	GUID GetProfileGuid(EncoderCodec codec) const;
	NV_ENC_TUNING_INFO GetTuningInfo(bool low_latency) const;

	void ConfigureRateControl();
	void ConfigureH264();
	void ConfigureHEVC();
};
