#include "nvenc_d3d12.h"

NvencD3D12::~NvencD3D12() {
	Shutdown();
}

bool NvencD3D12::Initialize(NvencSession* init_session, uint32_t buffer_count) {
	if (!init_session || !init_session->encoder)
		return false;
	session = init_session;
	textures.reserve(buffer_count);
	return true;
}

void NvencD3D12::Shutdown() {
	UnregisterAllTextures();
	UnregisterAllBitstreamBuffers();
	session = nullptr;
}

bool NvencD3D12::RegisterTexture(ID3D12Resource* texture, uint32_t width, uint32_t height,
								 NV_ENC_BUFFER_FORMAT format) {
	if (!session || !texture)
		return false;

	void* encoder = session->encoder;

	NV_ENC_REGISTER_RESOURCE register_params = {};
	register_params.version = NV_ENC_REGISTER_RESOURCE_VER;
	register_params.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
	register_params.resourceToRegister = texture;
	register_params.width = width;
	register_params.height = height;
	register_params.pitch = 0;
	register_params.bufferFormat = format;
	register_params.bufferUsage = NV_ENC_INPUT_IMAGE;

	NVENCSTATUS status = session->nvEncRegisterResource(encoder, &register_params);
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
		void* encoder = session->encoder;
		session->nvEncUnregisterResource(encoder, texture.registered_ptr);
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

bool NvencD3D12::RegisterBitstreamBuffer(ID3D12Resource* buffer, uint32_t size) {
	if (!session || !buffer)
		return false;

	void* encoder = session->encoder;

	NV_ENC_REGISTER_RESOURCE register_params = {};
	register_params.version = NV_ENC_REGISTER_RESOURCE_VER;
	register_params.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
	register_params.resourceToRegister = buffer;
	register_params.width = size;
	register_params.height = 1;
	register_params.pitch = 0;
	register_params.bufferFormat = NV_ENC_BUFFER_FORMAT_U8;
	register_params.bufferUsage = NV_ENC_OUTPUT_BITSTREAM;

	NVENCSTATUS status = session->nvEncRegisterResource(encoder, &register_params);
	if (status != NV_ENC_SUCCESS)
		return false;

	BitstreamBuffer bitstream = {};
	bitstream.resource = buffer;
	bitstream.registered_ptr = register_params.registeredResource;
	bitstream.size = size;

	bitstream_buffers.push_back(bitstream);
	return true;
}

void NvencD3D12::UnregisterBitstreamBuffer(uint32_t index) {
	if (index >= bitstream_buffers.size())
		return;

	auto& buffer = bitstream_buffers[index];
	if (buffer.registered_ptr) {
		void* encoder = session->encoder;
		session->nvEncUnregisterResource(encoder, buffer.registered_ptr);
		buffer.registered_ptr = nullptr;
	}
}

void NvencD3D12::UnregisterAllBitstreamBuffers() {
	for (uint32_t i = 0; i < bitstream_buffers.size(); ++i) {
		UnregisterBitstreamBuffer(i);
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

	void* encoder = session->encoder;

	NV_ENC_MAP_INPUT_RESOURCE map_params = {};
	map_params.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
	map_params.registeredResource = texture.registered_ptr;

	NVENCSTATUS status = session->nvEncMapInputResource(encoder, &map_params);
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

	void* encoder = session->encoder;

	NVENCSTATUS status = session->nvEncUnmapInputResource(encoder, texture.mapped_ptr);
	if (status != NV_ENC_SUCCESS)
		return false;

	texture.mapped_ptr = nullptr;
	texture.is_mapped = false;

	return true;
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
