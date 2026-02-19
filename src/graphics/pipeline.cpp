#include "graphics/pipeline.h"

#include <d3dcompiler.h>
#include <windows.h>

#include <cstring>
#include <string>
#include <vector>

#include "try.h"

static std::wstring GetExecutableDirectory() {
	wchar_t path[MAX_PATH]{};
	auto length = GetModuleFileNameW(nullptr, path, MAX_PATH);
	if (length == 0)
		throw;

	std::wstring full_path{path, path + length};
	auto last_separator = full_path.find_last_of(L"\\/");
	if (last_separator == std::wstring::npos)
		return L".";

	return full_path.substr(0, last_separator);
}

static bool FileExists(const std::wstring& path) {
	auto attributes = GetFileAttributesW(path.c_str());
	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static std::wstring ReplaceExtension(const std::wstring& path, const std::wstring& extension) {
	auto dot = path.find_last_of(L'.');
	if (dot == std::wstring::npos)
		return path + extension;
	return path.substr(0, dot) + extension;
}

static std::wstring FindShaderPath(const std::wstring& file_name) {
	auto exe_dir		   = GetExecutableDirectory();
	std::wstring candidate = exe_dir + L"\\shaders\\" + file_name;
	if (FileExists(candidate))
		return candidate;

	std::wstring repo_candidate = exe_dir + L"\\..\\..\\src\\shaders\\" + file_name;
	if (FileExists(repo_candidate))
		return repo_candidate;

	return candidate;
}

static std::vector<uint8_t> ReadFileBytes(const std::wstring& path) {
	auto handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
							  FILE_ATTRIBUTE_NORMAL, nullptr);
	if (handle == INVALID_HANDLE_VALUE)
		throw;

	LARGE_INTEGER size{};
	if (!GetFileSizeEx(handle, &size)) {
		CloseHandle(handle);
		throw;
	}

	std::vector<uint8_t> data;
	data.resize((size_t)size.QuadPart);
	DWORD read = 0;
	if (!ReadFile(handle, data.data(), (DWORD)data.size(), &read, nullptr)) {
		CloseHandle(handle);
		throw;
	}

	CloseHandle(handle);
	return data;
}

static void WriteFileBytes(const std::wstring& path, const std::vector<uint8_t>& data) {
	auto handle = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
							  FILE_ATTRIBUTE_NORMAL, nullptr);
	if (handle == INVALID_HANDLE_VALUE)
		throw;

	DWORD written = 0;
	if (!WriteFile(handle, data.data(), (DWORD)data.size(), &written, nullptr)) {
		CloseHandle(handle);
		throw;
	}

	CloseHandle(handle);
}

static std::vector<uint8_t> CompileShader(const std::wstring& path, const char* entry,
										  const char* target) {
	Microsoft::WRL::ComPtr<ID3DBlob> shader_blob;
	Microsoft::WRL::ComPtr<ID3DBlob> error_blob;
	UINT flags	= D3DCOMPILE_ENABLE_STRICTNESS;
	auto result = D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
									 entry, target, flags, 0, &shader_blob, &error_blob);
	if (FAILED(result) || !shader_blob)
		throw;

	std::vector<uint8_t> data;
	data.resize(shader_blob->GetBufferSize());
	memcpy(data.data(), shader_blob->GetBufferPointer(), shader_blob->GetBufferSize());
	return data;
}

static std::vector<uint8_t> LoadOrCompileShader(const std::wstring& hlsl_path, const char* entry,
												const char* target) {
	auto cso_path = ReplaceExtension(hlsl_path, L".cso");
	if (FileExists(cso_path))
		return ReadFileBytes(cso_path);

	auto compiled = CompileShader(hlsl_path, entry, target);
	WriteFileBytes(cso_path, compiled);
	return compiled;
}

static D3D12_BLEND_DESC CreateBlendDesc() {
	return D3D12_BLEND_DESC{
		.RenderTarget = {
			D3D12_RENDER_TARGET_BLEND_DESC{
				.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
			},
		},
	};
}

static D3D12_RASTERIZER_DESC CreateRasterizerDesc() {
	return D3D12_RASTERIZER_DESC{
		.FillMode		 = D3D12_FILL_MODE_SOLID,
		.CullMode		 = D3D12_CULL_MODE_BACK,
		.DepthClipEnable = TRUE,
	};
}

static D3D12_DEPTH_STENCIL_DESC CreateDepthStencilDesc() {
	return D3D12_DEPTH_STENCIL_DESC{
		.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS,
	};
}

D3D12Pipeline::D3D12Pipeline(ID3D12Device* device, DXGI_FORMAT render_target_format) {
	D3D12_ROOT_PARAMETER root_parameter{
		.ParameterType	  = D3D12_ROOT_PARAMETER_TYPE_CBV,
		.Descriptor		  = {.ShaderRegister = 0, .RegisterSpace = 0},
		.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX,
	};

	D3D12_ROOT_SIGNATURE_DESC root_signature_desc{
		.NumParameters = 1,
		.pParameters   = &root_parameter,
		.Flags		   = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
	};

	Microsoft::WRL::ComPtr<ID3DBlob> root_signature_blob;
	Microsoft::WRL::ComPtr<ID3DBlob> error_blob;
	Try
		| D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1,
									  &root_signature_blob, &error_blob)
		| device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
									  root_signature_blob->GetBufferSize(),
									  IID_PPV_ARGS(&root_signature));

	auto vertex_shader_path = FindShaderPath(L"mesh_vs.hlsl");
	auto pixel_shader_path	= FindShaderPath(L"mesh_ps.hlsl");
	auto vertex_shader		= LoadOrCompileShader(vertex_shader_path, "main", "vs_5_0");
	auto pixel_shader		= LoadOrCompileShader(pixel_shader_path, "main", "ps_5_0");

	D3D12_INPUT_ELEMENT_DESC input_layout[]{
		{
			.SemanticName		  = "POSITION",
			.SemanticIndex		  = 0,
			.Format				  = DXGI_FORMAT_R32G32B32_FLOAT,
			.InputSlot			  = 0,
			.AlignedByteOffset	  = 0,
			.InputSlotClass		  = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			.InstanceDataStepRate = 0,
		},
		{
			.SemanticName		  = "COLOR",
			.SemanticIndex		  = 0,
			.Format				  = DXGI_FORMAT_R32G32B32_FLOAT,
			.InputSlot			  = 0,
			.AlignedByteOffset	  = 12,
			.InputSlotClass		  = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			.InstanceDataStepRate = 0,
		},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc{
		.pRootSignature		   = root_signature.Get(),
		.VS					   = {vertex_shader.data(), vertex_shader.size()},
		.PS					   = {pixel_shader.data(), pixel_shader.size()},
		.BlendState			   = CreateBlendDesc(),
		.SampleMask			   = UINT_MAX,
		.RasterizerState	   = CreateRasterizerDesc(),
		.DepthStencilState	   = CreateDepthStencilDesc(),
		.InputLayout		   = {input_layout, (UINT)_countof(input_layout)},
		.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		.NumRenderTargets	   = 1,
		.RTVFormats			   = {render_target_format},
		.SampleDesc			   = {.Count = 1},
	};

	Try | device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipeline_state));
}

