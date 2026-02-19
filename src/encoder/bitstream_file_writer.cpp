#include "bitstream_file_writer.h"

#include <stdexcept>

BitstreamFileWriter::BitstreamFileWriter(const char* path)
	: file_handle(CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
							  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr)) {
	if (file_handle == INVALID_HANDLE_VALUE)
		throw;

	for (auto& slot : slots) {
		slot.event = CreateEvent(nullptr, TRUE, TRUE, nullptr);
		if (!slot.event)
			throw;
	}
}

BitstreamFileWriter::~BitstreamFileWriter() {
	for (uint32_t i = 0; i < pending_count; ++i) {
		auto& slot	= slots[(head + i) % WRITE_SLOT_COUNT];
		DWORD bytes = 0;
		GetOverlappedResult(file_handle, &slot.overlapped, &bytes, TRUE);
	}

	for (auto& slot : slots)
		if (slot.event)
			CloseHandle(slot.event);

	if (file_handle != INVALID_HANDLE_VALUE)
		CloseHandle(file_handle);
}

HANDLE BitstreamFileWriter::NextWriteEvent() const {
	return slots[head].event;
}

void BitstreamFileWriter::DrainCompleted() {
	while (pending_count > 0) {
		auto& slot	= slots[head];
		DWORD bytes = 0;
		if (!GetOverlappedResult(file_handle, &slot.overlapped, &bytes, FALSE)) {
			if (GetLastError() == ERROR_IO_INCOMPLETE)
				break;
			throw std::runtime_error("DrainCompleted: GetOverlappedResult failed");
		}
		head = (head + 1) % WRITE_SLOT_COUNT;
		--pending_count;
	}
}

bool BitstreamFileWriter::HasPendingWrites() const {
	return pending_count > 0;
}

void BitstreamFileWriter::WriteFrame(const void* data, uint32_t size) {
	if (!data || size == 0 || file_handle == INVALID_HANDLE_VALUE)
		return;

	if (pending_count == WRITE_SLOT_COUNT) {
		DWORD bytes = 0;
		GetOverlappedResult(file_handle, &slots[head].overlapped, &bytes, TRUE);
		head = (head + 1) % WRITE_SLOT_COUNT;
		--pending_count;
	}

	uint32_t slot_index = (head + pending_count) % WRITE_SLOT_COUNT;
	auto& slot			= slots[slot_index];

	slot.buffer.assign((const uint8_t*)data, (const uint8_t*)data + size);
	ResetEvent(slot.event);
	slot.overlapped			   = {};
	slot.overlapped.Offset	   = (DWORD)(file_offset & 0xFFFFFFFF);
	slot.overlapped.OffsetHigh = (DWORD)(file_offset >> 32);
	slot.overlapped.hEvent	   = slot.event;
	file_offset += size;

	if (!WriteFile(file_handle, slot.buffer.data(), (DWORD)size, nullptr, &slot.overlapped)
		&& GetLastError() != ERROR_IO_PENDING)
		throw;

	++pending_count;
}
