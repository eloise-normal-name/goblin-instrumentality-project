module;

#include <windows.h>

#include <array>
#include <utility>
#include <vector>

#include "encoder/nvenc_config.h"
#include "encoder/nvenc_d3d12.h"
#include "encoder/nvenc_session.h"
#include "frame_debug_log.h"
#include "graphics/d3d12_device.h"
#include "graphics/d3d12_swap_chain.h"
#include "try.h"

export module App;

constexpr auto BUFFER_COUNT			= 3u;
constexpr auto RENDER_TARGET_FORMAT = DXGI_FORMAT_B8G8R8A8_UNORM;

export class App {
	class FrameResources {
	  public:
		FrameResources(ID3D12Device* device);
		FrameResources(FrameResources&& rhs);
		~FrameResources();

		ComPtr<ID3D12GraphicsCommandList> command_list;
		ComPtr<ID3D12Fence> fence;
		HANDLE fence_event = nullptr;
	};
	static constexpr EncoderConfig ENCODER_CONFIG{.codec		= EncoderCodec::H264,
												  .preset		= EncoderPreset::Fastest,
												  .rate_control = RateControlMode::VariableBitrate,
												  .width		= 512,
												  .height		= 512};
	HWND hwnd;
	bool headless;
	D3D12Device device;
	NvencSession nvenc_session{*&device.device};
	NvencConfig nvenc_config{&nvenc_session, ENCODER_CONFIG};
	NvencD3D12 nvenc_d3d12{nvenc_session, BUFFER_COUNT};
	D3D12SwapChain swap_chain{*&device.device, *&device.factory, *&device.command_queue, hwnd,
							  SwapChainConfig{.buffer_count			= BUFFER_COUNT,
											  .render_target_format = RENDER_TARGET_FORMAT}};
	ComPtr<ID3D12CommandAllocator> allocator;
	ComPtr<ID3D12DescriptorHeap> offscreen_rtv_heap;
	uint32_t offscreen_rtv_descriptor_size;
	uint32_t frame_width;
	uint32_t frame_height;
	std::vector<ComPtr<ID3D12Resource>> offscreen_render_targets;
	std::vector<FrameResources> frames;
	FrameDebugLog frame_debug_log{"debug_output.txt"};

  public:
	App(HWND hwnd, bool headless)
		: hwnd(hwnd)
		, headless(headless)
		, device(headless ? AdapterType::WARP : AdapterType::Hardware) {
		InitializeGraphics();
	}

	int Run() && {
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
			auto wait_result = WaitForFrame(frame_debug_log, fence_handles.data(),
											back_buffer_index, msg, headless);
			if (wait_result == FrameWaitResult::Quit)
				break;
			if (wait_result == FrameWaitResult::Continue)
				continue;

			uint64_t completed_value = 0;
			if (!IsFrameReady(frames, back_buffer_index, frames_submitted, completed_value))
				continue;
			LogFenceStatus(frame_debug_log, completed_value, present_result);

			ExecuteFrame(*&device.command_queue, frames[back_buffer_index], present_result);
			auto signaled_value = frames_submitted + 1;
			present_result		= PresentAndSignal(*&device.command_queue, *&swap_chain.swap_chain,
												   frames[back_buffer_index], signaled_value);
			if (!HandlePresentResult(frame_debug_log, present_result))
				continue;

			auto new_back_buffer_index = swap_chain.swap_chain->GetCurrentBackBufferIndex();
			LogFrameSubmitted(frame_debug_log, back_buffer_index, signaled_value,
							  new_back_buffer_index);
			back_buffer_index = new_back_buffer_index;
			++frames_submitted;

			if (headless && frames_submitted >= 300)
				break;
		}
		return 0;
	}

  private:
	enum class FrameWaitResult {
		Quit,
		Continue,
		Proceed,
	};

	static void RecordCommandList(ID3D12GraphicsCommandList* command_list,
								  D3D12_CPU_DESCRIPTOR_HANDLE rtv,
								  ID3D12Resource* offscreen_render_target,
								  ID3D12Resource* swap_chain_render_target, uint32_t index) {
		{
			D3D12_RESOURCE_BARRIER barrier{
				.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Transition = {.pResource	= offscreen_render_target,
							   .StateBefore = D3D12_RESOURCE_STATE_COMMON,
							   .StateAfter	= D3D12_RESOURCE_STATE_RENDER_TARGET}};
			command_list->ResourceBarrier(1, &barrier);
		}

		float clear_color[]{(float)(index / 4 % 2), (float)(index / 2 % 2), (float)(index % 2), 1};
		command_list->ClearRenderTargetView(rtv, clear_color, 0, nullptr);

		{
			D3D12_RESOURCE_BARRIER barrier{
				.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Transition = {.pResource	= offscreen_render_target,
							   .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
							   .StateAfter	= D3D12_RESOURCE_STATE_COPY_SOURCE}};
			command_list->ResourceBarrier(1, &barrier);
		}

		{
			D3D12_RESOURCE_BARRIER barrier{
				.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Transition = {.pResource	= swap_chain_render_target,
							   .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
							   .StateAfter	= D3D12_RESOURCE_STATE_COPY_DEST}};
			command_list->ResourceBarrier(1, &barrier);
		}

		command_list->CopyResource(swap_chain_render_target, offscreen_render_target);

		{
			D3D12_RESOURCE_BARRIER barrier{
				.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Transition = {.pResource	= swap_chain_render_target,
							   .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
							   .StateAfter	= D3D12_RESOURCE_STATE_PRESENT}};
			command_list->ResourceBarrier(1, &barrier);
		}

		{
			D3D12_RESOURCE_BARRIER barrier{
				.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Transition = {.pResource	= offscreen_render_target,
							   .StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE,
							   .StateAfter	= D3D12_RESOURCE_STATE_COMMON}};
			command_list->ResourceBarrier(1, &barrier);
		}

		command_list->Close();
	}

	void MapEncodeTextureStub(uint32_t index, uint64_t fence_wait_value) {
		nvenc_d3d12.MapInputTexture(index, fence_wait_value);
	}

	void UnmapEncodeTextureStub(uint32_t index) {
		nvenc_d3d12.UnmapInputTexture(index);
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
										uint32_t back_buffer_index, MSG& msg, bool headless) {
		frame_debug_log.Line() << "MsgWaitForMultipleObjects..." << "\n";
		auto wait_handle = fence_handles[back_buffer_index];

		if (headless) {
			auto wait_result = WaitForSingleObject(wait_handle, INFINITE);
			frame_debug_log.Line() << "Wait result of fence_handles[" << back_buffer_index
								   << "]: " << wait_result << "\n";
			return FrameWaitResult::Proceed;
		}

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

		ID3D12CommandList* command_list_to_execute = *&frame_resources.command_list;
		ID3D12CommandList* lists[]				   = {command_list_to_execute};
		command_queue->ExecuteCommandLists(1, lists);
	}

	static HRESULT PresentAndSignal(ID3D12CommandQueue* command_queue, IDXGISwapChain4* swap_chain,
									FrameResources& frame_resources, uint64_t signaled_value) {
		auto present_result = swap_chain->Present(1, DXGI_PRESENT_DO_NOT_WAIT);
		command_queue->Signal(*&frame_resources.fence, signaled_value);
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
};

App::FrameResources::FrameResources(ID3D12Device* device) {
	fence_event = CreateEvent(nullptr, FALSE, TRUE, nullptr);
	if (!fence_event)
		throw;

	ComPtr<ID3D12Device4> device4;
	ComPtr<ID3D12GraphicsCommandList1> command_list1;
	Try | device->QueryInterface(IID_PPV_ARGS(&device4))
		| device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))
		| device4->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
									  D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&command_list1));
	command_list.Attach(command_list1.Detach());
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

void App::InitializeGraphics() {
	Try | swap_chain.swap_chain->GetSourceSize(&frame_width, &frame_height);

	Try
		| device.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
												IID_PPV_ARGS(&allocator));

	D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc{
		.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = BUFFER_COUNT,
	};

	Try | device.device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&offscreen_rtv_heap));
	offscreen_rtv_descriptor_size
		= device.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	offscreen_render_targets.resize(BUFFER_COUNT);
	D3D12_HEAP_PROPERTIES default_heap_props{
		.Type = D3D12_HEAP_TYPE_DEFAULT,
	};

	D3D12_RESOURCE_DESC texture_desc{
		.Dimension		  = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		.Width			  = frame_width,
		.Height			  = frame_height,
		.DepthOrArraySize = 1,
		.MipLevels		  = 1,
		.Format			  = RENDER_TARGET_FORMAT,
		.SampleDesc		  = {.Count = 1, .Quality = 0},
		.Layout			  = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags			  = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
	};

	D3D12_CLEAR_VALUE clear_value{
		.Format = RENDER_TARGET_FORMAT,
		.Color	= {0, 0, 0, 1},
	};

	for (auto i = 0u; i < BUFFER_COUNT; ++i) {
		Try
			| device.device->CreateCommittedResource(&default_heap_props, D3D12_HEAP_FLAG_NONE,
													 &texture_desc, D3D12_RESOURCE_STATE_COMMON,
													 &clear_value,
													 IID_PPV_ARGS(&offscreen_render_targets[i]));

		D3D12_CPU_DESCRIPTOR_HANDLE offscreen_rtv
			= offscreen_rtv_heap->GetCPUDescriptorHandleForHeapStart();
		offscreen_rtv.ptr += i * offscreen_rtv_descriptor_size;
		device.device->CreateRenderTargetView(*&offscreen_render_targets[i], nullptr,
											  offscreen_rtv);
	}

	frames.reserve(BUFFER_COUNT);
	for (auto i = 0u; i < BUFFER_COUNT; ++i) {
		frames.emplace_back(*&device.device);
	}

	for (auto i = 0u; i < BUFFER_COUNT; ++i) {
		nvenc_d3d12.RegisterTexture(*&offscreen_render_targets[i], frame_width, frame_height,
									DxgiFormatToNvencFormat(RENDER_TARGET_FORMAT),
									*&frames[i].fence);
	}

	for (auto i = 0u; i < BUFFER_COUNT; ++i) {
		D3D12_CPU_DESCRIPTOR_HANDLE offscreen_rtv
			= offscreen_rtv_heap->GetCPUDescriptorHandleForHeapStart();
		offscreen_rtv.ptr += i * offscreen_rtv_descriptor_size;

		frames[i].command_list->Reset(*&allocator, nullptr);
		RecordCommandList(*&frames[i].command_list, offscreen_rtv, *&offscreen_render_targets[i],
						  *&swap_chain.render_targets[i], i);
	}
}
