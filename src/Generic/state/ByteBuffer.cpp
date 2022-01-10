// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/state/ByteBuffer.h"

ByteBuffer::ByteBuffer(unsigned char* buffer, size_t bufferSize)
	: _currentBuffer(buffer), _buffer(buffer), _bufferSize(bufferSize) {
}

//Directly increment the current buffer position
ByteBuffer & ByteBuffer::operator+=(size_t offset) {
	_currentBuffer += offset;
	return *this;
}

//Directly decrement the current buffer position
ByteBuffer & ByteBuffer::operator-=(size_t offset) {
	_currentBuffer -= offset;
	return *this;
}

//Print the current buffer pointer as a hexadecimal
std::ostream & operator<<(std::ostream & out, ByteBuffer & buffer) {
	out << std::ostream::hex << (void*)(buffer._currentBuffer) << std::ostream::dec;
	return out;
}
