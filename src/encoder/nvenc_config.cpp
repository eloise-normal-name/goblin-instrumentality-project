#include "nvenc_config.h"

#include <cstring>

bool NvencConfig::Initialize(NvencSession* sess, const EncoderConfig& cfg) {
	if (!sess || !sess->encoder)
		return false;

	session = sess;
	config = cfg;

	void* enc = session->encoder;

	GUID codec_guid = GetCodecGuid(config.codec);
	GUID preset_guid = GetPresetGuid(config.preset);
	NV_ENC_TUNING_INFO tuning = GetTuningInfo(config.low_latency);

	NV_ENC_PRESET_CONFIG preset_cfg = {};
	preset_cfg.version = NV_ENC_PRESET_CONFIG_VER;
	preset_cfg.presetCfg.version = NV_ENC_CONFIG_VER;

	NVENCSTATUS status =
		session->nvEncGetEncodePresetConfigEx(enc, codec_guid, preset_guid, tuning, &preset_cfg);
	if (status != NV_ENC_SUCCESS)
		return false;

	encode_config = preset_cfg.presetCfg;

	memset(&init_params, 0, sizeof(init_params));
	init_params.version = NV_ENC_INITIALIZE_PARAMS_VER;
	init_params.encodeGUID = codec_guid;
	init_params.presetGUID = preset_guid;
	init_params.encodeWidth = config.width;
	init_params.encodeHeight = config.height;
	init_params.darWidth = config.width;
	init_params.darHeight = config.height;
	init_params.frameRateNum = config.frame_rate_num;
	init_params.frameRateDen = config.frame_rate_den;
	init_params.enablePTD = 1;
	init_params.encodeConfig = &encode_config;
	init_params.maxEncodeWidth = config.width;
	init_params.maxEncodeHeight = config.height;
	init_params.tuningInfo = tuning;
	init_params.enableEncodeAsync = true;
	init_params.bufferFormat = NV_ENC_BUFFER_FORMAT_ARGB;

	ConfigureRateControl();

	if (config.codec == EncoderCodec::H264) {
		ConfigureH264();
	} else if (config.codec == EncoderCodec::HEVC) {
		ConfigureHEVC();
	}

	return true;
}

bool NvencConfig::InitializeEncoder() {
	if (!session)
		return false;

	void* enc = session->encoder;

	NVENCSTATUS status = session->nvEncInitializeEncoder(enc, &init_params);
	return status == NV_ENC_SUCCESS;
}

GUID NvencConfig::GetPresetGuid(EncoderPreset preset) const {
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

GUID NvencConfig::GetCodecGuid(EncoderCodec codec) const {
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

GUID NvencConfig::GetProfileGuid(EncoderCodec codec) const {
	switch (codec) {
		case EncoderCodec::H264:
			return NV_ENC_H264_PROFILE_HIGH_GUID;
		case EncoderCodec::HEVC:
			return NV_ENC_HEVC_PROFILE_MAIN_GUID;
		default:
			return NV_ENC_CODEC_PROFILE_AUTOSELECT_GUID;
	}
}

NV_ENC_TUNING_INFO NvencConfig::GetTuningInfo(bool low_latency) const {
	if (low_latency) {
		return NV_ENC_TUNING_INFO_LOW_LATENCY;
	}
	return NV_ENC_TUNING_INFO_HIGH_QUALITY;
}

void NvencConfig::ConfigureRateControl() {
	NV_ENC_RC_PARAMS& rc = encode_config.rcParams;

	switch (config.rate_control) {
		case RateControlMode::ConstantQP:
			rc.rateControlMode = NV_ENC_PARAMS_RC_CONSTQP;
			rc.constQP.qpIntra = config.qp;
			rc.constQP.qpInterP = config.qp;
			rc.constQP.qpInterB = config.qp;
			break;

		case RateControlMode::VariableBitrate:
			rc.rateControlMode = NV_ENC_PARAMS_RC_VBR;
			rc.averageBitRate = config.bitrate;
			rc.maxBitRate = config.max_bitrate;
			break;

		case RateControlMode::ConstantBitrate:
			rc.rateControlMode = NV_ENC_PARAMS_RC_CBR;
			rc.averageBitRate = config.bitrate;
			rc.maxBitRate = config.bitrate;
			break;
	}

	if (config.low_latency) {
		rc.lowDelayKeyFrameScale = 1;
		rc.zeroReorderDelay = 1;
	}
}

void NvencConfig::ConfigureH264() {
	NV_ENC_CONFIG_H264& h264_cfg = encode_config.encodeCodecConfig.h264Config;

	h264_cfg.idrPeriod = config.gop_length;
	h264_cfg.sliceMode = 0;
	h264_cfg.sliceModeData = 0;
	h264_cfg.repeatSPSPPS = 1;

	if (config.low_latency) {
		h264_cfg.outputAUD = 0;
		h264_cfg.outputPictureTimingSEI = 0;
		h264_cfg.outputBufferingPeriodSEI = 0;
	}

	encode_config.gopLength = config.gop_length;
	encode_config.frameIntervalP = config.b_frames + 1;
}

void NvencConfig::ConfigureHEVC() {
	NV_ENC_CONFIG_HEVC& hevc_cfg = encode_config.encodeCodecConfig.hevcConfig;

	hevc_cfg.idrPeriod = config.gop_length;
	hevc_cfg.sliceMode = 0;
	hevc_cfg.sliceModeData = 0;
	hevc_cfg.repeatSPSPPS = 1;

	if (config.low_latency) {
		hevc_cfg.outputAUD = 0;
		hevc_cfg.outputPictureTimingSEI = 0;
		hevc_cfg.outputBufferingPeriodSEI = 0;
	}

	encode_config.gopLength = config.gop_length;
	encode_config.frameIntervalP = config.b_frames + 1;
}
