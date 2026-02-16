#include "bitstream_file_writer.h"

BitstreamFileWriter::BitstreamFileWriter(const char* path)
	: file(path, std::ios::binary | std::ios::out | std::ios::trunc) {
	if (!file)
		throw;
}

BitstreamFileWriter::~BitstreamFileWriter() {
	if (file.is_open())
		file.close();
}

void BitstreamFileWriter::WriteFrame(const void* data, uint32_t size) {
	if (!file || !data || size == 0)
		return;

	file.write((const char*)data, size);
	file.flush();
	++frames_written;
}
