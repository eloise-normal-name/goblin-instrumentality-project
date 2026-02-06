#pragma once

#include <cstdint>
#include <vector>

#include "../encoder/nvenc_config.h"
#include "../encoder/nvenc_d3d12.h"
#include "../encoder/nvenc_session.h"
#include "../graphics/d3d12_commands.h"
#include "../graphics/d3d12_device.h"
#include "../graphics/d3d12_resources.h"

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
	explicit FrameCoordinator(D3D12Device& device, const PipelineConfig& config);
	~FrameCoordinator();

	void BeginFrame();
	void EndFrame();
	void EncodeFrame(FrameData& output);

	D3D12Commands* GetCommands() {
		return &commands;
	}
	SharedTexture* GetEncoderTexture() {
		return &encoder_texture;
	}
	uint32_t GetCurrentFrameIndex() const {
		return current_frame_index;
	}
	uint64_t GetFrameCount() const {
		return frame_count;
	}

  private:
	void SubmitFrameToEncoder();
	void RetrieveEncodedFrame(FrameData& output);

	D3D12Device& device;
	PipelineConfig config;

	D3D12Commands commands{device.device.Get()};
	SharedTexture encoder_texture{device.device.Get(), TextureDesc{
														   .width  = config.width,
														   .height = config.height,
														   .format = DXGI_FORMAT_B8G8R8A8_UNORM,
														   .allow_simultaneous_access = true,
													   }};
	ReadbackBuffer output_buffer{device.device.Get(), config.width* config.height * 4};
	NvencSession nvenc_session{device.device.Get()};
	NvencD3D12 nvenc_d3d12{&nvenc_session, device.buffer_count};
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

	uint32_t current_frame_index	 = 0;
	uint64_t frame_count			 = 0;
	uint64_t last_encode_fence_value = 0;
};
