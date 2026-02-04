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
	TextureDesc desc{
		.width = config.width,
		.height = config.height,
		.format = DXGI_FORMAT_B8G8R8A8_UNORM,
		.allow_simultaneous_access = true,
	};

	encoder_texture.Create(device->device.Get(), desc);
}

void FrameCoordinator::InitializeEncoder() {
	nvenc_session.OpenSession(device->device.Get());

	nvenc_d3d12.Initialize(&nvenc_session, device->buffer_count);

	EncoderConfig enc_config{
		.codec = config.codec,
		.preset = EncoderPreset::Fast,
		.rate_control = RateControlMode::ConstantBitrate,
		.width = config.width,
		.height = config.height,
		.frame_rate_num = config.frame_rate,
		.bitrate = config.bitrate,
		.max_bitrate = config.bitrate * 2,
		.gop_length = config.frame_rate * 2,
		.low_latency = config.low_latency,
	};

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

	NV_ENC_PIC_PARAMS pic_params{
		.version = NV_ENC_PIC_PARAMS_VER,
		.inputWidth = config.width,
		.inputHeight = config.height,
		.inputPitch = config.width,
		.frameIdx = static_cast<uint32_t>(frame_count),
		.inputTimeStamp = frame_count,
		.inputBuffer = texture.mapped_ptr,
		.outputBitstream = bitstream.registered_ptr,
		.bufferFmt = texture.buffer_format,
		.pictureStruct = NV_ENC_PIC_STRUCT_FRAME,
	};

	Try | nvenc_session.nvEncEncodePicture(encoder, &pic_params);

	nvenc_d3d12.UnmapInputTexture(0);
}

void FrameCoordinator::RetrieveEncodedFrame(FrameData& output) {
	if (nvenc_d3d12.bitstream_buffers.empty())
		throw;

	auto& bitstream = nvenc_d3d12.bitstream_buffers[0];

	void* encoder = nvenc_session.encoder;

	NV_ENC_LOCK_BITSTREAM lock_params{
		.version = NV_ENC_LOCK_BITSTREAM_VER,
		.outputBitstream = bitstream.registered_ptr,
	};

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
