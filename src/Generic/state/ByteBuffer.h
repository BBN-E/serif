// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef BYTE_BUFFER_H
#define BYTE_BUFFER_H

#include <iostream>
#include <string>
#include <cstring> // for memcpy

class ByteBuffer {
	//Member methods are inlined for efficiency
	//
	//Peek methods return the value at the current location
	//Skip methods move the current location the relevant number of bytes
	//Get methods peek and then skip
public:
	//Buffer allocated by calling method
	ByteBuffer(unsigned char* buffer, size_t bufferSize);
	~ByteBuffer() { }

	//4-byte integers
	int peekInt(void) { return *((int*)_currentBuffer); }
	void skipInt(void) { _currentBuffer += sizeof(int); }
	int getInt(void) { int val = this->peekInt(); this->skipInt(); return val; }

	//2-byte integers
	short peekShort(void) { return *((short*)_currentBuffer); }
	void skipShort(void) { _currentBuffer += sizeof(short); }
	short getShort(void) { short val = this->peekShort(); this->skipShort(); return val; }

	//4-byte floating point
	float peekFloat(void) { return *((float*)_currentBuffer); }
	void skipFloat(void) { _currentBuffer += sizeof(float); }
	float getFloat(void) { float val = this->peekFloat(); this->skipFloat(); return val; }

	//8-byte floating point
	double peekDouble(void) { return *((double*)_currentBuffer); }
	void skipDouble(void) { _currentBuffer += sizeof(double); }
	double getDouble(void) { double val = this->peekDouble(); this->skipDouble(); return val; }

	//Character
	char peekChar(void) { return *((char*)_currentBuffer); }
	void skipChar(void) { _currentBuffer += sizeof(char); }
	char getChar(void) { char val = this->peekChar(); this->skipChar(); return val; }

	//Character string (null-terminated)
	std::string peekString(void) { return std::string((char*)_currentBuffer); }
	std::string getString(void) { std::string string = (char*)_currentBuffer; _currentBuffer += (string.length() + 1)*sizeof(char); return string; }

	//Wide character
	wchar_t peekWideChar(void) { return *((wchar_t*)_currentBuffer); }
	void skipWideChar(void) { _currentBuffer += sizeof(wchar_t); }
	wchar_t getWideChar(void) { wchar_t val = this->peekWideChar(); this->skipWideChar(); return val; }

	//Wide character string (null-terminated)
	std::wstring peekWideString(void) { return std::wstring((wchar_t*)_currentBuffer); }
	std::wstring getWideString(void) { std::wstring string = (wchar_t*)_currentBuffer; _currentBuffer += (string.length() + 1)*sizeof(wchar_t); return string; }

	//Arbitrary byte sequence
	void getBytes(unsigned char* valBuffer, size_t valBufferSize) { std::memcpy(valBuffer, _currentBuffer, valBufferSize); _currentBuffer += valBufferSize; }

	//Offset checker
	bool isBufferAtEnd(size_t end_offset) { return ((_currentBuffer + end_offset) >= (_buffer + _bufferSize)); }

	//Stream printer
	friend std::ostream & operator<<(std::ostream & out, ByteBuffer & buffer);

	//Operators
	ByteBuffer & operator+=(size_t offset);
	ByteBuffer & operator-=(size_t offset);
private:
	unsigned char* _currentBuffer; //Increments to the current offset
	unsigned char* _buffer; //Does not change
	size_t _bufferSize; //The length of the original buffer in bytes
};

#endif
