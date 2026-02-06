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
	FrameCoordinator() = default;
	~FrameCoordinator();

	FrameCoordinator(const FrameCoordinator&) = delete;
	FrameCoordinator& operator=(const FrameCoordinator&) = delete;
	FrameCoordinator(FrameCoordinator&&) = delete;
	FrameCoordinator& operator=(FrameCoordinator&&) = delete;

	void Initialize(D3D12Device* dev, const PipelineConfig& cfg);
	void Shutdown();

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
	void CreateEncoderTexture();
	void InitializeEncoder();
	void SubmitFrameToEncoder();
	void RetrieveEncodedFrame(FrameData& output);

	D3D12Device* device = nullptr;
	PipelineConfig config;

	D3D12Commands commands;
	SharedTexture encoder_texture;
	ReadbackBuffer output_buffer;

	NvencSession nvenc_session;
	NvencD3D12 nvenc_d3d12;
	NvencConfig nvenc_config;
	HANDLE encode_fence_event = nullptr;

	uint32_t current_frame_index = 0;
	uint64_t frame_count = 0;
	uint64_t last_encode_fence_value = 0;
	bool encoder_initialized = false;
};
