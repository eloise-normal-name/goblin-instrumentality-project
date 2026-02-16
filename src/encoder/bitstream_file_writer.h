#pragma once

#include <windows.h>

#include <cstdint>
#include <fstream>
#include <vector>

class BitstreamFileWriter {
  public:
	BitstreamFileWriter(const char* path);
	~BitstreamFileWriter();

	void WriteFrame(const void* data, uint32_t size);

  private:
	std::ofstream file;
};
