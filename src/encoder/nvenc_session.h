#pragma once

#include <nvenc/nvEncodeAPI.h>
#include <windows.h>

#include <cstdint>
#include <vector>

enum class EncoderCodec { H264, HEVC, AV1 };

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
	~NvencSession();

	void QueryCapabilities(EncoderCapabilities& caps);
	bool IsCodecSupported(EncoderCodec codec);

	void* encoder = nullptr;

  private:
	HMODULE nvenc_module = nullptr;
	GUID GetCodecGuid(EncoderCodec codec) const;
};
