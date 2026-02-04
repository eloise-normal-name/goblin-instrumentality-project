#include "frame_coordinator.h"

#include "try.h"

FrameCoordinator::~FrameCoordinator() {
	Shutdown();
}

void FrameCoordinator::Initialize(D3D12Device* dev, const PipelineConfig& cfg) {
	if (!dev)
		throw;

	device = dev;
	config = cfg;

	commands.Initialize(device->device.Get());
	CreateEncoderTexture();
	InitializeEncoder();
}

void FrameCoordinator::Shutdown() {
	if (device) {
		device->WaitForGpu();
	}

	nvenc_d3d12.Shutdown();
	nvenc_session.CloseSession();

	encoder_texture.Reset();
	output_buffer.Reset();
	encoder_initialized = false;
	device = nullptr;
}

void FrameCoordinator::BeginFrame() {
	if (!device)
		throw;

	current_frame_index = device->current_frame_index;
	commands.Reset();
}

void FrameCoordinator::EndFrame() {
	if (!device)
		throw;

	commands.TransitionResource(device->render_targets[device->current_frame_index].Get(),
								D3D12_RESOURCE_STATE_RENDER_TARGET,
								D3D12_RESOURCE_STATE_COPY_SOURCE);

	commands.TransitionTexture(encoder_texture, ResourceState::CopyDest);

	commands.CopyResource(encoder_texture.resource.Get(),
						  device->render_targets[device->current_frame_index].Get());

	commands.TransitionTexture(encoder_texture, ResourceState::Common);

	commands.TransitionResource(device->render_targets[device->current_frame_index].Get(),
								D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PRESENT);

	ExecuteCommandList(device->command_queue.Get(), commands);
}

void FrameCoordinator::EncodeFrame(FrameData& output) {
	if (!encoder_initialized)
		throw;

	device->WaitForGpu();

	SubmitFrameToEncoder();
	RetrieveEncodedFrame(output);

	++frame_count;
}

void FrameCoordinator::CreateEncoderTexture() {
	TextureDesc desc = {};
	desc.width = config.width;
	desc.height = config.height;
	desc.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.allow_render_target = false;
	desc.allow_simultaneous_access = true;

	encoder_texture.Create(device->device.Get(), desc);
}

void FrameCoordinator::InitializeEncoder() {
	nvenc_session.OpenSession(device->device.Get());

	nvenc_d3d12.Initialize(&nvenc_session, device->buffer_count);

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

	nvenc_config.Initialize(&nvenc_session, enc_config);
	nvenc_config.InitializeEncoder();

	NV_ENC_BUFFER_FORMAT nvenc_format = DxgiFormatToNvencFormat(encoder_texture.format);
	nvenc_d3d12.RegisterTexture(encoder_texture.resource.Get(), config.width, config.height,
								nvenc_format);

	uint32_t buffer_size = config.width * config.height * 4;
	output_buffer.Create(device->device.Get(), buffer_size);

	nvenc_d3d12.RegisterBitstreamBuffer(output_buffer.resource.Get(), buffer_size);

	encoder_initialized = true;
}

void FrameCoordinator::SubmitFrameToEncoder() {
	nvenc_d3d12.MapInputTexture(0);

	if (nvenc_d3d12.textures.empty() || nvenc_d3d12.bitstream_buffers.empty())
		throw;

	auto& texture = nvenc_d3d12.textures[0];
	auto& bitstream = nvenc_d3d12.bitstream_buffers[0];

	void* encoder = nvenc_session.encoder;

	NV_ENC_PIC_PARAMS pic_params = {};
	pic_params.version = NV_ENC_PIC_PARAMS_VER;
	pic_params.inputWidth = config.width;
	pic_params.inputHeight = config.height;
	pic_params.inputPitch = config.width;
	pic_params.inputBuffer = texture.mapped_ptr;
	pic_params.outputBitstream = bitstream.registered_ptr;
	pic_params.bufferFmt = texture.buffer_format;
	pic_params.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
	pic_params.frameIdx = static_cast<uint32_t>(frame_count);
	pic_params.inputTimeStamp = frame_count;

	Try | nvenc_session.nvEncEncodePicture(encoder, &pic_params);

	nvenc_d3d12.UnmapInputTexture(0);
}

void FrameCoordinator::RetrieveEncodedFrame(FrameData& output) {
	if (nvenc_d3d12.bitstream_buffers.empty())
		throw;

	auto& bitstream = nvenc_d3d12.bitstream_buffers[0];

	void* encoder = nvenc_session.encoder;

	NV_ENC_LOCK_BITSTREAM lock_params = {};
	lock_params.version = NV_ENC_LOCK_BITSTREAM_VER;
	lock_params.outputBitstream = bitstream.registered_ptr;
	lock_params.doNotWait = 0;

	Try | nvenc_session.nvEncLockBitstream(encoder, &lock_params);

	output.data = static_cast<uint8_t*>(lock_params.bitstreamBufferPtr);
	output.size = lock_params.bitstreamSizeInBytes;
	output.timestamp = lock_params.outputTimeStamp;
	output.is_keyframe = (lock_params.pictureType == NV_ENC_PIC_TYPE_IDR
						  || lock_params.pictureType == NV_ENC_PIC_TYPE_I);

	// The pointer 'output.data' is only valid until UnlockBitstream is called.
	// The caller must copy this data immediately.
	nvenc_session.nvEncUnlockBitstream(encoder, bitstream.registered_ptr);
}
