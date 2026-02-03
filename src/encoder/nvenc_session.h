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

class NvencSession {
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

    NV_ENCODE_API_FUNCTION_LIST* GetFunctionList() { return &function_list; }
    void* GetEncoder() const { return encoder; }
    bool IsInitialized() const { return encoder != nullptr; }

   private:
    GUID GetCodecGuid(EncoderCodec codec) const;

    HMODULE nvenc_module = nullptr;
    NV_ENCODE_API_FUNCTION_LIST function_list = {};
    void* encoder = nullptr;
};
