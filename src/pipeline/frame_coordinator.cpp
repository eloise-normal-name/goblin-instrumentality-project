#include "frame_coordinator.h"

FrameCoordinator::~FrameCoordinator() {
	Shutdown();
}

bool FrameCoordinator::Initialize(D3D12Device* dev, const PipelineConfig& cfg) {
	if (!dev)
		return false;

	device = dev;
	config = cfg;

	if (!commands.Initialize(device->GetDevice()))
		return false;
	if (!CreateEncoderTexture())
		return false;
	if (!InitializeEncoder())
		return false;

	return true;
}

void FrameCoordinator::Shutdown() {
	if (device) {
		device->WaitForGpu();
	}

	nvenc_d3d12.Shutdown();
	nvenc_session.CloseSession();

	encoder_texture.Reset();
	encoder_initialized = false;
	device = nullptr;
}

bool FrameCoordinator::BeginFrame() {
	if (!device)
		return false;

	current_frame_index = device->GetCurrentFrameIndex();
	commands.Reset();

	return true;
}

bool FrameCoordinator::EndFrame() {
	if (!device)
		return false;

	commands.TransitionResource(device->GetCurrentRenderTarget(),
								D3D12_RESOURCE_STATE_RENDER_TARGET,
								D3D12_RESOURCE_STATE_COPY_SOURCE);

	commands.TransitionTexture(encoder_texture, ResourceState::CopyDest);

	commands.CopyResource(encoder_texture.GetResource(), device->GetCurrentRenderTarget());

	commands.TransitionTexture(encoder_texture, ResourceState::Common);

	commands.TransitionResource(device->GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_COPY_SOURCE,
								D3D12_RESOURCE_STATE_PRESENT);

	ExecuteCommandList(device->GetCommandQueue(), commands);

	return true;
}

bool FrameCoordinator::EncodeFrame(FrameData& output) {
	if (!encoder_initialized)
		return false;

	device->WaitForGpu();

	if (!SubmitFrameToEncoder())
		return false;
	if (!RetrieveEncodedFrame(output))
		return false;

	++frame_count;
	return true;
}

bool FrameCoordinator::CreateEncoderTexture() {
	TextureDesc desc = {};
	desc.width = config.width;
	desc.height = config.height;
	desc.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.allow_render_target = false;
	desc.allow_simultaneous_access = true;

	return encoder_texture.Create(device->GetDevice(), desc);
}

bool FrameCoordinator::InitializeEncoder() {
	if (!nvenc_session.LoadLibrary())
		return false;
	if (!nvenc_session.OpenSession(device->GetDevice()))
		return false;

	if (!nvenc_d3d12.Initialize(&nvenc_session, device->GetBufferCount()))
		return false;

	EncoderConfig enc_config = {};
	enc_config.codec = config.codec;
	enc_config.preset = EncoderPreset::Fast;
	enc_config.rate_control = RateControlMode::ConstantBitrate;
	enc_config.width = config.width;
	enc_config.height = config.height;
	enc_config.frame_rate_num = config.frame_rate;
	enc_config.frame_rate_den = 1;
	enc_config.bitrate = config.bitrate;
	enc_config.max_bitrate = config.bitrate * 2;
	enc_config.gop_length = config.frame_rate * 2;
	enc_config.b_frames = 0;
	enc_config.low_latency = config.low_latency;
	enc_config.enable_async = false;

	if (!nvenc_config.Initialize(&nvenc_session, enc_config))
		return false;
	if (!nvenc_config.InitializeEncoder())
		return false;

	NV_ENC_BUFFER_FORMAT nvenc_format = DxgiFormatToNvencFormat(encoder_texture.GetFormat());
	if (!nvenc_d3d12.RegisterTexture(encoder_texture.GetResource(), config.width, config.height,
									 nvenc_format)) {
		return false;
	}

	if (!nvenc_d3d12.CreateBitstreamBuffers(1, config.width * config.height * 4)) {
		return false;
	}

	encoder_initialized = true;
	return true;
}

bool FrameCoordinator::SubmitFrameToEncoder() {
	if (!nvenc_d3d12.MapInputTexture(0))
		return false;

	auto* texture = nvenc_d3d12.GetRegisteredTexture(0);
	auto* bitstream = nvenc_d3d12.GetBitstreamBuffer(0);

	if (!texture || !bitstream)
		return false;

	void* encoder = nvenc_session.GetEncoder();

	NV_ENC_PIC_PARAMS pic_params = {};
	pic_params.version = NV_ENC_PIC_PARAMS_VER;
	pic_params.inputWidth = config.width;
	pic_params.inputHeight = config.height;
	pic_params.inputPitch = config.width;
	pic_params.inputBuffer = texture->mapped_ptr;
	pic_params.outputBitstream = bitstream->output_ptr;
	pic_params.bufferFmt = texture->buffer_format;
	pic_params.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
	pic_params.frameIdx = static_cast<uint32_t>(frame_count);
	pic_params.inputTimeStamp = frame_count;

	NVENCSTATUS status = nvenc_session.nvEncEncodePicture(encoder, &pic_params);

	nvenc_d3d12.UnmapInputTexture(0);

	return status == NV_ENC_SUCCESS;
}

bool FrameCoordinator::RetrieveEncodedFrame(FrameData& output) {
	auto* bitstream = nvenc_d3d12.GetBitstreamBuffer(0);
	if (!bitstream)
		return false;

	void* encoder = nvenc_session.GetEncoder();

	NV_ENC_LOCK_BITSTREAM lock_params = {};
	lock_params.version = NV_ENC_LOCK_BITSTREAM_VER;
	lock_params.outputBitstream = bitstream->output_ptr;
	lock_params.doNotWait = 0;

	NVENCSTATUS status = nvenc_session.nvEncLockBitstream(encoder, &lock_params);
	if (status != NV_ENC_SUCCESS)
		return false;

	output.data = static_cast<uint8_t*>(lock_params.bitstreamBufferPtr);
	output.size = lock_params.bitstreamSizeInBytes;
	output.timestamp = lock_params.outputTimeStamp;
	output.is_keyframe = (lock_params.pictureType == NV_ENC_PIC_TYPE_IDR ||
						  lock_params.pictureType == NV_ENC_PIC_TYPE_I);

	nvenc_session.nvEncUnlockBitstream(encoder, bitstream->output_ptr);

	return true;
}
