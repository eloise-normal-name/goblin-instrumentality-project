#include "nvenc_d3d12.h"

#include "try.h"

NvencD3D12::NvencD3D12(NvencSession* init_session, uint32_t buffer_count) {
	if (!init_session || !init_session->encoder)
		throw;
	session = init_session;
	textures.reserve(buffer_count);
}

NvencD3D12::~NvencD3D12() {
	UnregisterAllTextures();
	UnregisterAllBitstreamBuffers();
}

void NvencD3D12::RegisterTexture(ID3D12Resource* texture, uint32_t width, uint32_t height,
								 NV_ENC_BUFFER_FORMAT format) {
	if (!session || !texture)
		throw;

	void* encoder = session->encoder;

	NV_ENC_REGISTER_RESOURCE register_params{
		.version = NV_ENC_REGISTER_RESOURCE_VER,
		.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX,
		.width = width,
		.height = height,
		.resourceToRegister = texture,
		.bufferFormat = format,
		.bufferUsage = NV_ENC_INPUT_IMAGE,
	};

	Try | session->nvEncRegisterResource(encoder, &register_params);

	textures.push_back({
		.resource = texture,
		.registered_ptr = register_params.registeredResource,
		.buffer_format = format,
		.width = width,
		.height = height,
	});
}

void NvencD3D12::UnregisterTexture(uint32_t index) {
	if (index >= textures.size())
		throw;

	RegisteredTexture& texture = textures[index];
	if (texture.is_mapped)
		UnmapInputTexture(index);

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

	NV_ENC_REGISTER_RESOURCE register_params{
		.version = NV_ENC_REGISTER_RESOURCE_VER,
		.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX,
		.width = size,
		.height = 1,
		.resourceToRegister = buffer,
		.bufferFormat = NV_ENC_BUFFER_FORMAT_U8,
		.bufferUsage = NV_ENC_OUTPUT_BITSTREAM,
	};

	Try | session->nvEncRegisterResource(encoder, &register_params);

	bitstream_buffers.push_back({
		.resource = buffer,
		.registered_ptr = register_params.registeredResource,
		.size = size,
	});
}

void NvencD3D12::UnregisterBitstreamBuffer(uint32_t index) {
	if (index >= bitstream_buffers.size())
		return;

	BitstreamBuffer& buffer = bitstream_buffers[index];
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

	RegisteredTexture& texture = textures[index];
	if (texture.is_mapped)
		return;
	if (!texture.registered_ptr)
		throw;

	void* encoder = session->encoder;

	NV_ENC_MAP_INPUT_RESOURCE map_params{
		.version = NV_ENC_MAP_INPUT_RESOURCE_VER,
		.registeredResource = texture.registered_ptr,
	};

	Try | session->nvEncMapInputResource(encoder, &map_params);

	texture.mapped_ptr = map_params.mappedResource;
	texture.buffer_format = map_params.mappedBufferFmt;
	texture.is_mapped = true;
}

void NvencD3D12::UnmapInputTexture(uint32_t index) {
	if (index >= textures.size())
		throw;

	RegisteredTexture& texture = textures[index];
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
