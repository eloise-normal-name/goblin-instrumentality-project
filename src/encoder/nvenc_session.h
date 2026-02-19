#pragma once

#include <nvenc/nvEncodeAPI.h>
#include <windows.h>

#include <cstdint>
#include <vector>

enum class EncoderCodec { H264, HEVC, AV1 };

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

struct EncoderCapabilities {
	bool supports_h264;
	bool supports_hevc;
	bool supports_av1;
	uint32_t max_width;
	uint32_t max_height;
	bool supports_async_encode;
	bool supports_10bit;
};

struct NvencSession : public NV_ENCODE_API_FUNCTION_LIST {
  public:
	NvencSession(void* d3d12_device);
	NvencSession(void* d3d12_device, const EncoderConfig& config);
	~NvencSession();

	void InitializeEncoder(const EncoderConfig& config);

	void QueryCapabilities(EncoderCapabilities& caps);
	bool IsCodecSupported(EncoderCodec codec);

	void* encoder = nullptr;

  private:
	HMODULE nvenc_module = nullptr;
	GUID GetCodecGuid(EncoderCodec codec) const;
};
