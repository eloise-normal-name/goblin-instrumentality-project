#include "nvenc_session.h"

#include "try.h"

using NvEncodeAPICreateInstanceFunc = NVENCSTATUS(NVENCAPI*)(NV_ENCODE_API_FUNCTION_LIST*);

static GUID GetPresetGuid(EncoderPreset preset) {
	switch (preset) {
		case EncoderPreset::Fastest:
			return NV_ENC_PRESET_P1_GUID;
		case EncoderPreset::Fast:
			return NV_ENC_PRESET_P2_GUID;
		case EncoderPreset::Medium:
			return NV_ENC_PRESET_P4_GUID;
		case EncoderPreset::Slow:
			return NV_ENC_PRESET_P5_GUID;
		case EncoderPreset::Slowest:
			return NV_ENC_PRESET_P7_GUID;
		default:
			return NV_ENC_PRESET_P4_GUID;
	}
}

static NV_ENC_TUNING_INFO GetTuningInfo(bool low_latency) {
	if (low_latency)
		return NV_ENC_TUNING_INFO_LOW_LATENCY;
	return NV_ENC_TUNING_INFO_HIGH_QUALITY;
}

static void ConfigureRateControl(NV_ENC_CONFIG& encode_config, const EncoderConfig& config) {
	NV_ENC_RC_PARAMS& rc = encode_config.rcParams;

	switch (config.rate_control) {
		case RateControlMode::ConstantQP:
			rc.rateControlMode	= NV_ENC_PARAMS_RC_CONSTQP;
			rc.constQP.qpIntra	= config.qp;
			rc.constQP.qpInterP = config.qp;
			rc.constQP.qpInterB = config.qp;
			break;

		case RateControlMode::VariableBitrate:
			rc.rateControlMode = NV_ENC_PARAMS_RC_VBR;
			rc.averageBitRate  = config.bitrate;
			rc.maxBitRate	   = config.max_bitrate;
			break;

		case RateControlMode::ConstantBitrate:
			rc.rateControlMode = NV_ENC_PARAMS_RC_CBR;
			rc.averageBitRate  = config.bitrate;
			rc.maxBitRate	   = config.bitrate;
			break;
	}

	if (config.low_latency) {
		rc.lowDelayKeyFrameScale = 1;
		rc.zeroReorderDelay		 = 1;
	}
}

static void ConfigureH264(NV_ENC_CONFIG& encode_config, const EncoderConfig& config) {
	NV_ENC_CONFIG_H264& h264_cfg = encode_config.encodeCodecConfig.h264Config;

	h264_cfg.idrPeriod	   = config.gop_length;
	h264_cfg.sliceMode	   = 0;
	h264_cfg.sliceModeData = 0;
	h264_cfg.repeatSPSPPS  = 1;

	if (config.low_latency) {
		h264_cfg.outputAUD				  = 0;
		h264_cfg.outputPictureTimingSEI	  = 0;
		h264_cfg.outputBufferingPeriodSEI = 0;
	}

	encode_config.gopLength		 = config.gop_length;
	encode_config.frameIntervalP = config.b_frames + 1;
}

static void ConfigureHEVC(NV_ENC_CONFIG& encode_config, const EncoderConfig& config) {
	NV_ENC_CONFIG_HEVC& hevc_cfg = encode_config.encodeCodecConfig.hevcConfig;

	hevc_cfg.idrPeriod	 = config.gop_length;
	hevc_cfg.sliceMode	 = 0;
	hevc_cfg.sliceModeData = 0;
	hevc_cfg.repeatSPSPPS = 1;

	if (config.low_latency) {
		hevc_cfg.outputAUD				  = 0;
		hevc_cfg.outputPictureTimingSEI	  = 0;
		hevc_cfg.outputBufferingPeriodSEI = 0;
	}

	encode_config.gopLength		 = config.gop_length;
	encode_config.frameIntervalP = config.b_frames + 1;
}

NvencSession::NvencSession(void* d3d12_device) {
	nvenc_module = ::LoadLibraryW(L"nvEncodeAPI64.dll");
	if (!nvenc_module)
		throw;

	auto create_instance = (NvEncodeAPICreateInstanceFunc)(GetProcAddress(
		nvenc_module, "NvEncodeAPICreateInstance"));

	if (!create_instance) {
		FreeLibrary(nvenc_module);
		nvenc_module = nullptr;
		throw;
	}

	version = NV_ENCODE_API_FUNCTION_LIST_VER;
	Try | create_instance(this);

	NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS session_params{
		.version	= NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER,
		.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX,
		.device		= d3d12_device,
		.apiVersion = NVENCAPI_VERSION,
	};
	Try | nvEncOpenEncodeSessionEx(&session_params, &encoder);
	if (!encoder)
		throw;
}

NvencSession::NvencSession(void* d3d12_device, const EncoderConfig& config)
	: NvencSession(d3d12_device) {
	InitializeEncoder(config);
}

NvencSession::~NvencSession() {
	if (encoder)
		nvEncDestroyEncoder(encoder);
	if (nvenc_module)
		FreeLibrary(nvenc_module);
}

void NvencSession::InitializeEncoder(const EncoderConfig& config) {
	if (!encoder)
		throw;

	void* enc = encoder;

	GUID codec_guid			  = GetCodecGuid(config.codec);
	GUID preset_guid		  = GetPresetGuid(config.preset);
	NV_ENC_TUNING_INFO tuning = GetTuningInfo(config.low_latency);

	NV_ENC_PRESET_CONFIG preset_cfg{
		.version   = NV_ENC_PRESET_CONFIG_VER,
		.presetCfg = {.version = NV_ENC_CONFIG_VER},
	};

	Try | nvEncGetEncodePresetConfigEx(enc, codec_guid, preset_guid,
									 NV_ENC_TUNING_INFO_LOW_LATENCY, &preset_cfg);

	NV_ENC_CONFIG encode_config = preset_cfg.presetCfg;

	NV_ENC_INITIALIZE_PARAMS init_params{
		.version		   = NV_ENC_INITIALIZE_PARAMS_VER,
		.encodeGUID		   = codec_guid,
		.presetGUID		   = preset_guid,
		.encodeWidth	   = config.width,
		.encodeHeight	   = config.height,
		.darWidth		   = config.width,
		.darHeight		   = config.height,
		.frameRateNum	   = config.frame_rate_num,
		.frameRateDen	   = config.frame_rate_den,
		.enableEncodeAsync = true,
		.enablePTD		   = 1,
		.encodeConfig	   = &encode_config,
		.maxEncodeWidth	   = config.width,
		.maxEncodeHeight   = config.height,
		.tuningInfo		   = tuning,
		.bufferFormat
		= config.codec == EncoderCodec::AV1 ? NV_ENC_BUFFER_FORMAT_NV12 : NV_ENC_BUFFER_FORMAT_ARGB,
	};

	ConfigureRateControl(encode_config, config);

	if (config.codec == EncoderCodec::H264)
		ConfigureH264(encode_config, config);
	else if (config.codec == EncoderCodec::HEVC)
		ConfigureHEVC(encode_config, config);

	Try | nvEncInitializeEncoder(enc, &init_params);
}

void NvencSession::QueryCapabilities(EncoderCapabilities& caps) {
	if (!encoder)
		throw;

	caps = {};

	uint32_t guid_count = 0;
	Try | nvEncGetEncodeGUIDCount(encoder, &guid_count);

	std::vector<GUID> guids(guid_count);
	uint32_t actual_count = 0;
	Try | nvEncGetEncodeGUIDs(encoder, guids.data(), guid_count, &actual_count);

	auto guid_equals = [](const GUID& a, const GUID& b) {
		return a.Data1 == b.Data1 && a.Data2 == b.Data2 && a.Data3 == b.Data3
			   && memcmp(a.Data4, b.Data4, 8) == 0;
	};

	for (uint32_t i = 0; i < actual_count; ++i) {
		if (guid_equals(guids[i], NV_ENC_CODEC_H264_GUID))
			caps.supports_h264 = true;
		if (guid_equals(guids[i], NV_ENC_CODEC_HEVC_GUID))
			caps.supports_hevc = true;
		if (guid_equals(guids[i], NV_ENC_CODEC_AV1_GUID))
			caps.supports_av1 = true;
	}

	NV_ENC_CAPS_PARAM caps_param = {};
	caps_param.version			 = NV_ENC_CAPS_PARAM_VER;

	GUID codec_guid = caps.supports_hevc ? NV_ENC_CODEC_HEVC_GUID : NV_ENC_CODEC_H264_GUID;

	caps_param.capsToQuery = NV_ENC_CAPS_WIDTH_MAX;
	auto value			   = 0;
	if (nvEncGetEncodeCaps(encoder, codec_guid, &caps_param, &value) == NV_ENC_SUCCESS)
		caps.max_width = (uint32_t)value;

	caps_param.capsToQuery = NV_ENC_CAPS_HEIGHT_MAX;
	if (nvEncGetEncodeCaps(encoder, codec_guid, &caps_param, &value) == NV_ENC_SUCCESS)
		caps.max_height = (uint32_t)value;

	caps_param.capsToQuery = NV_ENC_CAPS_ASYNC_ENCODE_SUPPORT;
	if (nvEncGetEncodeCaps(encoder, codec_guid, &caps_param, &value) == NV_ENC_SUCCESS)
		caps.supports_async_encode = value != 0;

	caps_param.capsToQuery = NV_ENC_CAPS_SUPPORT_10BIT_ENCODE;
	if (nvEncGetEncodeCaps(encoder, codec_guid, &caps_param, &value) == NV_ENC_SUCCESS)
		caps.supports_10bit = value != 0;
}

bool NvencSession::IsCodecSupported(EncoderCodec codec) {
	EncoderCapabilities caps;
	try {
		QueryCapabilities(caps);
	} catch (...) {
		return false;
	}

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
