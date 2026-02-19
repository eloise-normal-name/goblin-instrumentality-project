#pragma once

#include <windows.h>

#include <cstdint>
#include <vector>

class BitstreamFileWriter {
  public:
	explicit BitstreamFileWriter(const char* path);
	~BitstreamFileWriter();

	void WriteFrame(const void* data, uint32_t size);
	void DrainCompleted();
	bool HasPendingWrites() const;
	HANDLE NextWriteEvent() const;

  private:
	static constexpr uint32_t WRITE_SLOT_COUNT = 4;

	struct WriteSlot {
		HANDLE event;
		OVERLAPPED overlapped;
		std::vector<uint8_t> buffer;
	};

	HANDLE file_handle	 = INVALID_HANDLE_VALUE;
	uint64_t file_offset = 0;
	WriteSlot slots[WRITE_SLOT_COUNT];
	uint32_t head		   = 0;
	uint32_t pending_count = 0;
};
