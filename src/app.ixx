module;

#include <windows.h>

#include <array>
#include <utility>
#include <vector>

#include "frame_debug_log.h"
#include "graphics/d3d12_device.h"
#include "graphics/d3d12_swap_chain.h"
#include "try.h"

export module app;

export constexpr uint32_t WINDOW_WIDTH	= 512;
export constexpr uint32_t WINDOW_HEIGHT = 512;

export class App {
  public:
	App(HWND hwnd);
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
								  uint32_t index) {
		{
			D3D12_RESOURCE_BARRIER barrier{
				.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Transition = {.pResource	= render_target,
							   .StateBefore = D3D12_RESOURCE_STATE_COMMON,
							   .StateAfter	= D3D12_RESOURCE_STATE_RENDER_TARGET}};
			command_list->ResourceBarrier(1, &barrier);
		}

		float clear_color[]{(float)(index / 4 % 2), (float)(index / 2 % 2), (float)(index % 2), 1};
		command_list->ClearRenderTargetView(rtv, clear_color, 0, nullptr);

		{
			D3D12_RESOURCE_BARRIER barrier{
				.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Transition = {.pResource	= render_target,
							   .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
							   .StateAfter	= D3D12_RESOURCE_STATE_PRESENT}};
			command_list->ResourceBarrier(1, &barrier);
		}

		command_list->Close();
	}

	static bool ProcessWindowMessages(MSG& msg) {
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				return false;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		return true;
	}

	static FrameWaitResult WaitForFrame(FrameDebugLog& frame_debug_log, HANDLE* fence_handles,
										uint32_t back_buffer_index, MSG& msg) {
		frame_debug_log.Line() << "MsgWaitForMultipleObjects..." << "\n";
		auto wait_handle = fence_handles[back_buffer_index];
		auto wait_result = MsgWaitForMultipleObjects(1, &wait_handle, FALSE, INFINITE, QS_ALLINPUT);
		frame_debug_log.Line() << "Wait result of fence_handles[" << back_buffer_index
							   << "]: " << wait_result << "\n";

		if (wait_result != WAIT_OBJECT_0 + 1)
			return FrameWaitResult::Proceed;
		if (!ProcessWindowMessages(msg))
			return FrameWaitResult::Quit;
		return FrameWaitResult::Continue;
	}

	static bool IsFrameReady(const std::vector<FrameResources>& frames, uint32_t back_buffer_index,
							 uint32_t frames_submitted, uint64_t& completed_value) {
		completed_value = frames[back_buffer_index].fence->GetCompletedValue();
		return completed_value + frames.size() >= frames_submitted + 1;
	}

	static void LogFenceStatus(FrameDebugLog& frame_debug_log, uint64_t completed_value,
							   HRESULT present_result) {
		frame_debug_log.Line() << "Completed Value: " << completed_value << "\n";
		frame_debug_log.Line() << "present_result: " << present_result << "\n";
	}

	static void ExecuteFrame(ID3D12CommandQueue* command_queue, FrameResources& frame_resources,
							 HRESULT present_result) {
		if (present_result != S_OK)
			return;

		ID3D12CommandList* command_list_to_execute = frame_resources.command_list.Get();
		ID3D12CommandList* lists[]				   = {command_list_to_execute};
		command_queue->ExecuteCommandLists(1, lists);
	}

	static HRESULT PresentAndSignal(ID3D12CommandQueue* command_queue, IDXGISwapChain4* swap_chain,
									FrameResources& frame_resources, uint64_t signaled_value) {
		auto present_result = swap_chain->Present(1, DXGI_PRESENT_DO_NOT_WAIT);
		command_queue->Signal(frame_resources.fence.Get(), signaled_value);
		frame_resources.fence->SetEventOnCompletion(signaled_value, frame_resources.fence_event);
		return present_result;
	}

	static bool HandlePresentResult(FrameDebugLog& frame_debug_log, HRESULT present_result) {
		if (present_result != DXGI_ERROR_WAS_STILL_DRAWING)
			return true;

		frame_debug_log.Line() << "Present returned DXGI_ERROR_WAS_STILL_DRAWING, skipping..."
							   << "\n";
		return false;
	}

	static void LogFrameSubmitted(FrameDebugLog& frame_debug_log, uint32_t back_buffer_index,
								  uint64_t signaled_value, uint32_t new_back_buffer_index) {
		frame_debug_log.Line() << "Frame submitted, fence[" << back_buffer_index
							   << "] signaled with value: " << signaled_value << "\n";
		frame_debug_log.Line() << "new back_buffer_index: " << new_back_buffer_index << "\n";
	}

	void InitializeGraphics();

	HWND hwnd = nullptr;
	D3D12Device device;
	D3D12SwapChain swap_chain;
	ComPtr<ID3D12CommandAllocator> allocator;
	std::vector<FrameResources> frames;
	FrameDebugLog frame_debug_log;
};

constexpr uint32_t BUFFER_COUNT = 3;

App::FrameResources::FrameResources(ID3D12Device* device, ID3D12CommandAllocator* allocator) {
	Try | device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	fence_event = CreateEvent(nullptr, FALSE, TRUE, nullptr);
	if (!fence_event)
		throw;

	Try
		| device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, nullptr,
									IID_PPV_ARGS(&command_list));
}

App::FrameResources::FrameResources(FrameResources&& rhs) {
	command_list	= std::move(rhs.command_list);
	fence			= std::move(rhs.fence);
	fence_event		= rhs.fence_event;
	rhs.fence_event = nullptr;
}

App::FrameResources::~FrameResources() {
	CloseHandle(fence_event);
}

App::App(HWND hwnd)
	: hwnd(hwnd)
	, swap_chain(*&device.device, *&device.factory, *&device.command_queue, hwnd,
				 SwapChainConfig{.frame_width		   = WINDOW_WIDTH,
								 .frame_height		   = WINDOW_HEIGHT,
								 .buffer_count		   = BUFFER_COUNT,
								 .render_target_format = DXGI_FORMAT_R8G8B8A8_UNORM})
	, frame_debug_log("debug_output.txt") {
	InitializeGraphics();
}

int App::Run() && {
	std::array<HANDLE, BUFFER_COUNT> fence_handles{};
	for (auto i = 0u; i < BUFFER_COUNT; ++i)
		fence_handles[i] = frames[i].fence_event;

	MSG msg{};
	bool running			   = true;
	uint32_t frames_submitted  = 0;
	uint32_t back_buffer_index = swap_chain.swap_chain->GetCurrentBackBufferIndex();
	HRESULT present_result	   = S_OK;

	while (running) {
		frame_debug_log.BeginFrame(frames_submitted);
		auto wait_result
			= WaitForFrame(frame_debug_log, fence_handles.data(), back_buffer_index, msg);
		if (wait_result == FrameWaitResult::Quit)
			break;
		if (wait_result == FrameWaitResult::Continue)
			continue;

		uint64_t completed_value = 0;
		if (!IsFrameReady(frames, back_buffer_index, frames_submitted, completed_value))
			continue;
		LogFenceStatus(frame_debug_log, completed_value, present_result);

		ExecuteFrame(device.command_queue.Get(), frames[back_buffer_index], present_result);
		auto signaled_value = frames_submitted + 1;
		present_result = PresentAndSignal(device.command_queue.Get(), swap_chain.swap_chain.Get(),
										  frames[back_buffer_index], signaled_value);
		if (!HandlePresentResult(frame_debug_log, present_result))
			continue;

		auto new_back_buffer_index = swap_chain.swap_chain->GetCurrentBackBufferIndex();
		LogFrameSubmitted(frame_debug_log, back_buffer_index, signaled_value,
						  new_back_buffer_index);
		back_buffer_index = new_back_buffer_index;
		++frames_submitted;
	}
	return 0;
}

void App::InitializeGraphics() {
	Try
		| device.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
												IID_PPV_ARGS(&allocator));

	frames.reserve(BUFFER_COUNT);
	for (auto i = 0u; i < BUFFER_COUNT; ++i) {
		frames.emplace_back(device.device.Get(), allocator.Get());

		D3D12_CPU_DESCRIPTOR_HANDLE rtv = swap_chain.rtv_heap->GetCPUDescriptorHandleForHeapStart();
		rtv.ptr += i * swap_chain.rtv_descriptor_size;
		auto* render_target = swap_chain.render_targets[i].Get();
		RecordCommandList(frames[i].command_list.Get(), rtv, render_target, i);
	}
}
