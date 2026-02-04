#include "nvenc_d3d12.h"

#include "try.h"

NvencD3D12::~NvencD3D12() {
	Shutdown();
}

void NvencD3D12::Initialize(NvencSession* init_session, uint32_t buffer_count) {
	if (!init_session || !init_session->encoder)
		throw;
	session = init_session;
	textures.reserve(buffer_count);
}

void NvencD3D12::Shutdown() {
	UnregisterAllTextures();
	UnregisterAllBitstreamBuffers();
	session = nullptr;
}

void NvencD3D12::RegisterTexture(ID3D12Resource* texture, uint32_t width, uint32_t height,
								 NV_ENC_BUFFER_FORMAT format) {
	if (!session || !texture)
		throw;

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

	Try | session->nvEncRegisterResource(encoder, &register_params);

	RegisteredTexture reg_texture = {};
	reg_texture.resource = texture;
	reg_texture.registered_ptr = register_params.registeredResource;
	reg_texture.buffer_format = format;
	reg_texture.width = width;
	reg_texture.height = height;
	reg_texture.is_mapped = false;

	textures.push_back(reg_texture);
}

void NvencD3D12::UnregisterTexture(uint32_t index) {
	if (index >= textures.size())
		throw;

	auto& texture = textures[index];
	if (texture.is_mapped) {
		UnmapInputTexture(index);
	}

	if (texture.registered_ptr) {
		void* encoder = session->encoder;
		Try | session->nvEncUnregisterResource(encoder, texture.registered_ptr);
		texture.registered_ptr = nullptr;
	}
}

void NvencD3D12::UnregisterAllTextures() {
	for (uint32_t i = 0; i < textures.size(); ++i) {
		UnregisterTexture(i);
	}
	textures.clear();
}

void NvencD3D12::RegisterBitstreamBuffer(ID3D12Resource* buffer, uint32_t size) {
	if (!session || !buffer)
		throw;

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

	Try | session->nvEncRegisterResource(encoder, &register_params);

	BitstreamBuffer bitstream = {};
	bitstream.resource = buffer;
	bitstream.registered_ptr = register_params.registeredResource;
	bitstream.size = size;

	bitstream_buffers.push_back(bitstream);
}

void NvencD3D12::UnregisterBitstreamBuffer(uint32_t index) {
	if (index >= bitstream_buffers.size())
		return;

	auto& buffer = bitstream_buffers[index];
	if (buffer.registered_ptr) {
		void* encoder = session->encoder;
		Try | session->nvEncUnregisterResource(encoder, buffer.registered_ptr);
		buffer.registered_ptr = nullptr;
	}
}

void NvencD3D12::UnregisterAllBitstreamBuffers() {
	for (uint32_t i = 0; i < bitstream_buffers.size(); ++i) {
		UnregisterBitstreamBuffer(i);
	}
	bitstream_buffers.clear();
}

void NvencD3D12::MapInputTexture(uint32_t index) {
	if (index >= textures.size())
		throw;

	auto& texture = textures[index];
	if (texture.is_mapped)
		return;
	if (!texture.registered_ptr)
		throw;

	void* encoder = session->encoder;

	NV_ENC_MAP_INPUT_RESOURCE map_params = {};
	map_params.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
	map_params.registeredResource = texture.registered_ptr;

	Try | session->nvEncMapInputResource(encoder, &map_params);

	texture.mapped_ptr = map_params.mappedResource;
	texture.buffer_format = map_params.mappedBufferFmt;
	texture.is_mapped = true;
}

void NvencD3D12::UnmapInputTexture(uint32_t index) {
	if (index >= textures.size())
		throw;

	auto& texture = textures[index];
	if (!texture.is_mapped)
		return;

	void* encoder = session->encoder;

	Try | session->nvEncUnmapInputResource(encoder, texture.mapped_ptr);

	texture.mapped_ptr = nullptr;
	texture.is_mapped = false;
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
