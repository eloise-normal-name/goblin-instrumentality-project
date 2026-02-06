#pragma once

#include <cstdint>
#include <memory>
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
	explicit FrameCoordinator(D3D12Device* device, const PipelineConfig& config);
	~FrameCoordinator();

	void BeginFrame();
	void EndFrame();
	void EncodeFrame(FrameData& output);

	D3D12Commands* GetCommands() {
		return commands.get();
	}
	SharedTexture* GetEncoderTexture() {
		return encoder_texture.get();
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

	D3D12Device* device;
	PipelineConfig config;

	std::unique_ptr<D3D12Commands> commands;
	std::unique_ptr<SharedTexture> encoder_texture;
	std::unique_ptr<ReadbackBuffer> output_buffer;
	std::unique_ptr<NvencSession> nvenc_session;
	std::unique_ptr<NvencD3D12> nvenc_d3d12;
	std::unique_ptr<NvencConfig> nvenc_config;
	HANDLE encode_fence_event = nullptr;

	uint32_t current_frame_index = 0;
	uint64_t frame_count = 0;
	uint64_t last_encode_fence_value = 0;
};
