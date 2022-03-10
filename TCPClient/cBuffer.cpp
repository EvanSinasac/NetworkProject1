#include <cstddef>
#include "cBuffer.h"
#include <iostream>

cBuffer::cBuffer(std::size_t size)
{
	_buffer.resize(size);
	writeIndex = 0;
	readIndex = 0;
}

void cBuffer::writeUInt32BE(size_t index, uint32_t value)
{
	_buffer[index] = value >> 24;
	_buffer[index + 1] = value >> 16;
	_buffer[index + 2] = value >> 8;
	_buffer[index + 3] = value;
}

uint32_t cBuffer::readUInt32BE(size_t index)
{
	uint32_t value = _buffer[index] << 24;
	value |= _buffer[index + 1] << 16;
	value |= _buffer[index + 2] << 8;
	value |= _buffer[index + 3];
	return value;
}

void cBuffer::writeIntBE(size_t index, int value)
{
	_buffer[index] = value >> 24;
	_buffer[index + 1] = value >> 16;
	_buffer[index + 2] = value >> 8;
	_buffer[index + 3] = value;
}
int cBuffer::readIntBE(size_t index)
{
	int value = _buffer[index] << 24;
	value |= _buffer[index + 1] << 16;
	value |= _buffer[index + 2] << 8;
	value |= _buffer[index + 3];
	return value;
}

void cBuffer::writeShortBE(size_t index, short value)
{
	_buffer[index] = value >> 16;
	_buffer[index + 1] = value >> 8;
	_buffer[index + 2] = value >> 4;
	_buffer[index + 3] = value;
}
short cBuffer::readShortBE(size_t index)
{
	short value = _buffer[index] << 16;
	value |= _buffer[index + 1] << 8;
	value |= _buffer[index + 2] << 4;
	value |= _buffer[index + 3];
	return value;
}

void cBuffer::writeStringBE(size_t index, char value)
{
	_buffer[index] = (uint8_t)value >> 24;
	_buffer[index + 1] = (uint8_t)value >> 16;
	_buffer[index + 2] = (uint8_t)value >> 8;
	_buffer[index + 3] = (uint8_t)value;
}

char cBuffer::readStringBE(size_t index)
{
	char value = (char)_buffer[index] << 24;
	value |= (char)_buffer[index + 1] << 16;
	value |= (char)_buffer[index + 2] << 8;
	value |= (char)_buffer[index + 3];
	return value;
}
