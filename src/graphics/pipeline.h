#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

class D3D12Pipeline {
	Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state;

  public:
	D3D12Pipeline(ID3D12Device* device, DXGI_FORMAT render_target_format);

	ID3D12RootSignature* GetRootSignature() const {
		return root_signature.Get();
	}

	ID3D12PipelineState* GetPipelineState() const {
		return pipeline_state.Get();
	}
};
