#include "nvenc_d3d12.h"

NvencD3D12::~NvencD3D12() {
	Shutdown();
}

bool NvencD3D12::Initialize(NvencSession* init_session, uint32_t buffer_count) {
	if (!init_session || !init_session->IsInitialized())
		return false;
	session = init_session;
	textures.reserve(buffer_count);
	return true;
}

void NvencD3D12::Shutdown() {
	UnregisterAllTextures();
	DestroyBitstreamBuffers();
	session = nullptr;
}

bool NvencD3D12::RegisterTexture(ID3D12Resource* texture, uint32_t width, uint32_t height,
								 NV_ENC_BUFFER_FORMAT format) {
	if (!session || !texture)
		return false;

	auto* fn = session->GetFunctionList();
	void* encoder = session->GetEncoder();

	NV_ENC_REGISTER_RESOURCE register_params = {};
	register_params.version = NV_ENC_REGISTER_RESOURCE_VER;
	register_params.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
	register_params.resourceToRegister = texture;
	register_params.width = width;
	register_params.height = height;
	register_params.pitch = 0;
	register_params.bufferFormat = format;
	register_params.bufferUsage = NV_ENC_INPUT_IMAGE;

	NVENCSTATUS status = fn->nvEncRegisterResource(encoder, &register_params);
	if (status != NV_ENC_SUCCESS)
		return false;

	RegisteredTexture reg_texture = {};
	reg_texture.resource = texture;
	reg_texture.registered_ptr = register_params.registeredResource;
	reg_texture.buffer_format = format;
	reg_texture.width = width;
	reg_texture.height = height;
	reg_texture.is_mapped = false;

	textures.push_back(reg_texture);
	return true;
}

bool NvencD3D12::UnregisterTexture(uint32_t index) {
	if (index >= textures.size())
		return false;

	auto& texture = textures[index];
	if (texture.is_mapped) {
		UnmapInputTexture(index);
	}

	if (texture.registered_ptr) {
		auto* fn = session->GetFunctionList();
		void* encoder = session->GetEncoder();
		fn->nvEncUnregisterResource(encoder, texture.registered_ptr);
		texture.registered_ptr = nullptr;
	}

	return true;
}

void NvencD3D12::UnregisterAllTextures() {
	for (uint32_t i = 0; i < textures.size(); ++i) {
		UnregisterTexture(i);
	}
	textures.clear();
}

bool NvencD3D12::CreateBitstreamBuffers(uint32_t count, uint32_t size) {
	if (!session)
		return false;

	auto* fn = session->GetFunctionList();
	void* encoder = session->GetEncoder();

	for (uint32_t i = 0; i < count; ++i) {
		NV_ENC_CREATE_BITSTREAM_BUFFER create_params = {};
		create_params.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;

		NVENCSTATUS status = fn->nvEncCreateBitstreamBuffer(encoder, &create_params);
		if (status != NV_ENC_SUCCESS) {
			DestroyBitstreamBuffers();
			return false;
		}

		BitstreamBuffer buffer = {};
		buffer.output_ptr = create_params.bitstreamBuffer;
		buffer.size = size;

		bitstream_buffers.push_back(buffer);
	}

	return true;
}

void NvencD3D12::DestroyBitstreamBuffers() {
	if (!session)
		return;

	auto* fn = session->GetFunctionList();
	void* encoder = session->GetEncoder();

	for (auto& buffer : bitstream_buffers) {
		if (buffer.output_ptr) {
			fn->nvEncDestroyBitstreamBuffer(encoder, buffer.output_ptr);
		}
	}

	bitstream_buffers.clear();
}

bool NvencD3D12::MapInputTexture(uint32_t index) {
	if (index >= textures.size())
		return false;

	auto& texture = textures[index];
	if (texture.is_mapped)
		return true;
	if (!texture.registered_ptr)
		return false;

	auto* fn = session->GetFunctionList();
	void* encoder = session->GetEncoder();

	NV_ENC_MAP_INPUT_RESOURCE map_params = {};
	map_params.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
	map_params.registeredResource = texture.registered_ptr;

	NVENCSTATUS status = fn->nvEncMapInputResource(encoder, &map_params);
	if (status != NV_ENC_SUCCESS)
		return false;

	texture.mapped_ptr = map_params.mappedResource;
	texture.buffer_format = map_params.mappedBufferFmt;
	texture.is_mapped = true;

	return true;
}

bool NvencD3D12::UnmapInputTexture(uint32_t index) {
	if (index >= textures.size())
		return false;

	auto& texture = textures[index];
	if (!texture.is_mapped)
		return true;

	auto* fn = session->GetFunctionList();
	void* encoder = session->GetEncoder();

	NVENCSTATUS status = fn->nvEncUnmapInputResource(encoder, texture.mapped_ptr);
	if (status != NV_ENC_SUCCESS)
		return false;

	texture.mapped_ptr = nullptr;
	texture.is_mapped = false;

	return true;
}

RegisteredTexture* NvencD3D12::GetRegisteredTexture(uint32_t index) {
	if (index >= textures.size())
		return nullptr;
	return &textures[index];
}

BitstreamBuffer* NvencD3D12::GetBitstreamBuffer(uint32_t index) {
	if (index >= bitstream_buffers.size())
		return nullptr;
	return &bitstream_buffers[index];
}

NV_ENC_BUFFER_FORMAT DxgiFormatToNvencFormat(DXGI_FORMAT format) {
	switch (format) {
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			return NV_ENC_BUFFER_FORMAT_ARGB;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return NV_ENC_BUFFER_FORMAT_ABGR;
		case DXGI_FORMAT_R10G10B10A2_UNORM:
			return NV_ENC_BUFFER_FORMAT_ABGR10;
		case DXGI_FORMAT_NV12:
			return NV_ENC_BUFFER_FORMAT_NV12;
		case DXGI_FORMAT_P010:
			return NV_ENC_BUFFER_FORMAT_YUV420_10BIT;
		default:
			return NV_ENC_BUFFER_FORMAT_ARGB;
	}
}
