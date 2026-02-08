#pragma once

#include <cstdint>

#include "../encoder/nvenc_config.h"
#include "../encoder/nvenc_d3d12.h"
#include "../encoder/nvenc_session.h"
#include "../graphics/d3d12_command_allocators.h"
#include "../graphics/d3d12_commands.h"
#include "../graphics/d3d12_device.h"
#include "../graphics/d3d12_frame_sync.h"
#include "../graphics/d3d12_resources.h"
#include "../graphics/render_targets.h"

struct FrameData {
	uint8_t* data;
	uint32_t size;
	uint64_t timestamp;
	bool is_keyframe;
};

struct PipelineConfig {
	uint32_t width;
	uint32_t height;
	uint32_t frame_rate;
	uint32_t bitrate;
	EncoderCodec codec;
	bool low_latency;
};

class FrameCoordinator {
  public:
	explicit FrameCoordinator(D3D12Device& device, D3D12FrameSync& frame_sync,
							  D3D12CommandAllocators& allocators, RenderTargets& targets,
							  const PipelineConfig& config);
	~FrameCoordinator();

	void BeginFrame();
	void EndFrame();
	void EncodeFrame(FrameData& output);

	D3D12Commands* GetCommands() {
		return &commands;
	}
	ID3D12Resource* GetEncoderTexture() {
		return *&encoder_textures[render_targets.current_frame_index];
	}
	uint32_t GetCurrentFrameIndex() const {
		return render_targets.current_frame_index;
	}
	uint64_t GetFrameCount() const {
		return frame_count;
	}

  private:
	void SubmitFrameToEncoder();
	void RetrieveEncodedFrame(FrameData& output);

	D3D12Device& device;
	D3D12FrameSync& frame_sync;
	D3D12CommandAllocators& allocators;
	RenderTargets& render_targets;
	PipelineConfig config;

	D3D12Commands commands{*&device.device, allocators.GetAllocator(0)};
	ComPtr<ID3D12Resource> encoder_textures[3];
	ResourceState encoder_states[3]{ResourceState::Common, ResourceState::Common,
									ResourceState::Common};
	ReadbackBuffer output_buffer{*&device.device, config.width* config.height * 4};
	NvencSession nvenc_session{*&device.device};
	NvencD3D12 nvenc_d3d12{&nvenc_session, allocators.buffer_count};
	NvencConfig nvenc_config{&nvenc_session, EncoderConfig{
												 .codec			 = config.codec,
												 .preset		 = EncoderPreset::Fast,
												 .rate_control	 = RateControlMode::ConstantBitrate,
												 .width			 = config.width,
												 .height		 = config.height,
												 .frame_rate_num = config.frame_rate,
												 .bitrate		 = config.bitrate,
												 .max_bitrate	 = config.bitrate * 2,
												 .gop_length	 = config.frame_rate * 2,
												 .low_latency	 = config.low_latency,
											 }};
	HANDLE encode_fence_event = nullptr;

	uint64_t frame_count			 = 0;
	uint64_t last_encode_fence_value = 0;
};
