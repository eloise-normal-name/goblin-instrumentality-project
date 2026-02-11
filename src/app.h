#pragma once

#include <windows.h>

#include <array>
#include <vector>

#include "frame_debug_log.h"
#include "graphics/d3d12_device.h"
#include "graphics/d3d12_swap_chain.h"

constexpr uint32_t WINDOW_WIDTH	 = 512;
constexpr uint32_t WINDOW_HEIGHT = 512;

class App {
  public:
	explicit App(HWND hwnd);
	int Run() &&;

  private:
	enum class FrameWaitResult {
		Quit,
		Continue,
		Proceed,
	};

	class FrameResources {
	  public:
		FrameResources(ID3D12Device* device, ID3D12CommandAllocator* allocator);
		FrameResources(FrameResources&& rhs);
		~FrameResources();

		ComPtr<ID3D12GraphicsCommandList> command_list;
		ComPtr<ID3D12Fence> fence;
		HANDLE fence_event = nullptr;
	};

	static void RecordCommandList(ID3D12GraphicsCommandList* command_list,
								  D3D12_CPU_DESCRIPTOR_HANDLE rtv, ID3D12Resource* render_target,
								  uint32_t index);
	static bool ProcessWindowMessages(MSG& msg);
	static FrameWaitResult WaitForFrame(FrameDebugLog& frame_debug_log, HANDLE* fence_handles,
										uint32_t back_buffer_index, MSG& msg);
	static bool IsFrameReady(const std::vector<FrameResources>& frames, uint32_t back_buffer_index,
							 uint32_t frames_submitted, uint64_t& completed_value);
	static void LogFenceStatus(FrameDebugLog& frame_debug_log, uint64_t completed_value,
							   HRESULT present_result);
	static void ExecuteFrame(ID3D12CommandQueue* command_queue, FrameResources& frame_resources,
							 HRESULT present_result);
	static HRESULT PresentAndSignal(ID3D12CommandQueue* command_queue, IDXGISwapChain4* swap_chain,
									FrameResources& frame_resources, uint64_t signaled_value);
	static bool HandlePresentResult(FrameDebugLog& frame_debug_log, HRESULT present_result);
	static void LogFrameSubmitted(FrameDebugLog& frame_debug_log, uint32_t back_buffer_index,
								  uint64_t signaled_value, uint32_t new_back_buffer_index);

	void InitializeGraphics();

	HWND hwnd = nullptr;
	D3D12Device device;
	D3D12SwapChain swap_chain;
	ComPtr<ID3D12CommandAllocator> allocator;
	std::vector<FrameResources> frames;
	FrameDebugLog frame_debug_log;
};
