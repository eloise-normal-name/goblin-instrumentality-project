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
	NvencSession() = default;
	~NvencSession();

	NvencSession(const NvencSession&) = delete;
	NvencSession& operator=(const NvencSession&) = delete;
	NvencSession(NvencSession&&) = delete;
	NvencSession& operator=(NvencSession&&) = delete;

	bool LoadLibrary();
	bool OpenSession(void* d3d12_device);
	void CloseSession();

	bool QueryCapabilities(EncoderCapabilities& caps);
	bool IsCodecSupported(EncoderCodec codec);

	HMODULE nvenc_module = nullptr;
	void* encoder = nullptr;

  private:
	GUID GetCodecGuid(EncoderCodec codec) const;
};
