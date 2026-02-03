#include "nvenc_session.h"

using NvEncodeAPICreateInstanceFunc = NVENCSTATUS(NVENCAPI*)(NV_ENCODE_API_FUNCTION_LIST*);

NvencSession::~NvencSession() {
    CloseSession();
    if (nvenc_module) {
        FreeLibrary(nvenc_module);
        nvenc_module = nullptr;
    }
}

bool NvencSession::LoadLibrary() {
    if (nvenc_module) return true;

    nvenc_module = ::LoadLibraryW(L"nvEncodeAPI64.dll");
    if (!nvenc_module) return false;

    auto create_instance = reinterpret_cast<NvEncodeAPICreateInstanceFunc>(
        GetProcAddress(nvenc_module, "NvEncodeAPICreateInstance"));

    if (!create_instance) {
        FreeLibrary(nvenc_module);
        nvenc_module = nullptr;
        return false;
    }

    function_list.version = NV_ENCODE_API_FUNCTION_LIST_VER;

    NVENCSTATUS status = create_instance(&function_list);
    if (status != NV_ENC_SUCCESS) {
        FreeLibrary(nvenc_module);
        nvenc_module = nullptr;
        return false;
    }

    return true;
}

bool NvencSession::OpenSession(void* d3d12_device) {
    if (!nvenc_module || !function_list.nvEncOpenEncodeSessionEx) return false;
    if (encoder) return true;

    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS session_params = {};
    session_params.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
    session_params.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
    session_params.device = d3d12_device;
    session_params.apiVersion = NVENCAPI_VERSION;

    NVENCSTATUS status = function_list.nvEncOpenEncodeSessionEx(&session_params, &encoder);
    return status == NV_ENC_SUCCESS && encoder != nullptr;
}

void NvencSession::CloseSession() {
    if (encoder && function_list.nvEncDestroyEncoder) {
        function_list.nvEncDestroyEncoder(encoder);
        encoder = nullptr;
    }
}

bool NvencSession::QueryCapabilities(EncoderCapabilities& caps) {
    if (!encoder) return false;

    caps = {};

    uint32_t guid_count = 0;
    if (function_list.nvEncGetEncodeGUIDCount(encoder, &guid_count) != NV_ENC_SUCCESS) {
        return false;
    }

    std::vector<GUID> guids(guid_count);
    uint32_t actual_count = 0;
    if (function_list.nvEncGetEncodeGUIDs(encoder, guids.data(), guid_count, &actual_count) !=
        NV_ENC_SUCCESS) {
        return false;
    }

    auto guid_equals = [](const GUID& a, const GUID& b) {
        return a.Data1 == b.Data1 && a.Data2 == b.Data2 && a.Data3 == b.Data3 &&
               memcmp(a.Data4, b.Data4, 8) == 0;
    };

    for (uint32_t i = 0; i < actual_count; ++i) {
        if (guid_equals(guids[i], NV_ENC_CODEC_H264_GUID)) caps.supports_h264 = true;
        if (guid_equals(guids[i], NV_ENC_CODEC_HEVC_GUID)) caps.supports_hevc = true;
        if (guid_equals(guids[i], NV_ENC_CODEC_AV1_GUID)) caps.supports_av1 = true;
    }

    NV_ENC_CAPS_PARAM caps_param = {};
    caps_param.version = NV_ENC_CAPS_PARAM_VER;

    GUID codec_guid = caps.supports_hevc ? NV_ENC_CODEC_HEVC_GUID : NV_ENC_CODEC_H264_GUID;

    caps_param.capsToQuery = NV_ENC_CAPS_WIDTH_MAX;
    int value = 0;
    if (function_list.nvEncGetEncodeCaps(encoder, codec_guid, &caps_param, &value) == NV_ENC_SUCCESS) {
        caps.max_width = static_cast<uint32_t>(value);
    }

    caps_param.capsToQuery = NV_ENC_CAPS_HEIGHT_MAX;
    if (function_list.nvEncGetEncodeCaps(encoder, codec_guid, &caps_param, &value) == NV_ENC_SUCCESS) {
        caps.max_height = static_cast<uint32_t>(value);
    }

    caps_param.capsToQuery = NV_ENC_CAPS_ASYNC_ENCODE_SUPPORT;
    if (function_list.nvEncGetEncodeCaps(encoder, codec_guid, &caps_param, &value) == NV_ENC_SUCCESS) {
        caps.supports_async_encode = value != 0;
    }

    caps_param.capsToQuery = NV_ENC_CAPS_SUPPORT_10BIT_ENCODE;
    if (function_list.nvEncGetEncodeCaps(encoder, codec_guid, &caps_param, &value) == NV_ENC_SUCCESS) {
        caps.supports_10bit = value != 0;
    }

    return true;
}

bool NvencSession::IsCodecSupported(EncoderCodec codec) {
    EncoderCapabilities caps;
    if (!QueryCapabilities(caps)) return false;

    switch (codec) {
        case EncoderCodec::H264:
            return caps.supports_h264;
        case EncoderCodec::HEVC:
            return caps.supports_hevc;
        case EncoderCodec::AV1:
            return caps.supports_av1;
        default:
            return false;
    }
}

GUID NvencSession::GetCodecGuid(EncoderCodec codec) const {
    switch (codec) {
        case EncoderCodec::H264:
            return NV_ENC_CODEC_H264_GUID;
        case EncoderCodec::HEVC:
            return NV_ENC_CODEC_HEVC_GUID;
        case EncoderCodec::AV1:
            return NV_ENC_CODEC_AV1_GUID;
        default:
            return NV_ENC_CODEC_H264_GUID;
    }
}
