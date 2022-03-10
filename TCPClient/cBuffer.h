#pragma once
#include <cstddef>
#include <vector>
#include <string>

class cBuffer
{
private:
	std::vector<uint8_t> _buffer;
	int writeIndex;
	int readIndex;
public:
	cBuffer(std::size_t size);

	void writeUInt32BE(size_t index, uint32_t value);
	uint32_t readUInt32BE(size_t index);

	void writeIntBE(size_t index, int value);
	int readIntBE(size_t index);

	void writeShortBE(size_t index, short value);
	short readShortBE(size_t index);

	void writeStringBE(size_t index, char value);
	char readStringBE(size_t index);

};