// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/BoostUtil.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/UnicodeUtil.h"

#include <string>

using namespace std;


UTF8OutputStream::UTF8OutputStream() : _stream() {}

UTF8OutputStream::UTF8OutputStream(const std::string file) : _stream() {
	this->open(file.c_str());
}

UTF8OutputStream::UTF8OutputStream(const std::wstring file) : _stream() {
	this->open(file.c_str());
}

UTF8OutputStream::UTF8OutputStream(const char* file) : _stream() {
	this->open(file);
}

UTF8OutputStream::UTF8OutputStream(const wchar_t* file) : _stream() {
	this->open(file);
}

UTF8OutputStream::~UTF8OutputStream() {
	this->close();
}

void UTF8OutputStream::open(const wchar_t* file, bool append)
{
	open(OutputUtil::convertToUTF8BitString(file).c_str(), append);
}

void UTF8OutputStream::open(const char* file, bool append)
{
	this->close();

	if (append)
		_stream.open(file, ios_base::app | ios_base::binary);
	else
		_stream.open(file, ios_base::out | ios_base::binary);

	if (_stream.fail()) {
#if defined(_WIN32)
		if (strlen(file) > 259) {
			throw UnrecoverableException("UTF8OutputStream::UTF8OutputStream",
				(string("File '") + file + "' has length > 259.  Windows does not allow paths with more than 259 characters.").c_str()); 
		}
#else
		boost::filesystem::path docPathBoost(file);
		std::string short_file = BOOST_FILESYSTEM_PATH_GET_FILENAME(docPathBoost);
		if (short_file.length() > 255) {
			 throw UnrecoverableException("UTF8OutputStream::UTF8OutputStream",
				(string("File '") + file + "' has length > 255.  Most Unix file systems do not allow filenames with more than 255 characters.").c_str()); 
		}
#endif
		stringstream errmsg;
		errmsg << "UTF8OutputStream (char) failed to open '" << file << "'";
		throw UnrecoverableException("UTF8OutputStream::open()", errmsg.str().c_str());
	}
	string file_as_string(file);
	_file = wstring(file_as_string.begin(), file_as_string.end());
}

void UTF8OutputStream::close()
{
	_file.clear();

	if (_stream.is_open()) {
		_stream.close();
		if (_stream.fail()) {
			stringstream errmsg;
			errmsg << "couldn't close " << string(_file.begin(), _file.end());
			throw UnrecoverableException("UTF8OutputStream::close()",
			                             errmsg.str().c_str());
		}
	}
}

// this implementation is a lot like (i.e. copied from)
// UTF8InputStream::putBack, but we put the bytes back in proper order
// not reverse order
UTF8OutputStream& UTF8OutputStream::put(wchar_t ch)
{
	// < 7f => 0xxxxxxx
	unsigned char c[4];
	unsigned char baseMask = 0x80;
	unsigned char lowMask = 0x3f;
	if (ch <= 0x007f) {
		c[0] = (char)ch;
		_stream.put(c[0]);
	}
	// < 7ff => 110xxxxx 10xxxxxx
	else if (ch <= 0x07ff) {
		c[1] = (baseMask | (lowMask&((char) ch)));
		c[0] = (0xc0 | ((char)(ch >> 6)));
		_stream.put(c[0]);
		_stream.put(c[1]);
	}
	// < ffff => 1110xxxx 10xxxxxx 10xxxxxx
	else if (ch <= 0xffff) {
		c[2] = (baseMask | (lowMask&((char) ch)));
		c[1] = (baseMask | (lowMask&((char) (ch >> 6))));
		c[0] = (0xe0 | ((char)(ch >> 12)));
		_stream.put(c[0]);
		_stream.put(c[1]);
		_stream.put(c[2]);
	}
	if (_stream.fail()) {
		stringstream errmsg;
		errmsg << "trouble writing to " << string(_file.begin(), _file.end());
		throw UnrecoverableException("UTF8OutputStream::put()", errmsg.str().c_str());
	}

	return *this;
}

UTF8OutputStream& UTF8OutputStream::write(const wchar_t* str, size_t size)
{

	for (size_t i = 0; i < size; i++) {
		if (str[i] == 0)
			throw UnexpectedInputException(
				"UTF8OutputStream::write()",
				"reached EOS before declared end");
		this->put(str[i]);
	}
	return *this;
}

const std::wstring& UTF8OutputStream::getFileName() const {
	return _file;
}

UTF8OutputStream& UTF8OutputStream::operator<< (std::string str)
{
	_stream << str.c_str();
	return *this;
}


UTF8OutputStream& UTF8OutputStream::operator<< (const char* str)
{
	size_t size = strlen(str);
	wchar_t wstr[5001];
	if (size > 5000)
		size = 5000;
	size_t i;
	for (i = 0; i < size; i++)
		wstr[i] = static_cast<wchar_t>(str[i]);
	wstr[i] = 0;
	return this->write(wstr, size);
}

UTF8OutputStream& UTF8OutputStream::operator<< (int i)
{
	_stream << i;
	return *this;
}

UTF8OutputStream& UTF8OutputStream::operator<< (unsigned int i)
{
	_stream << i;
	return *this;
}

UTF8OutputStream& UTF8OutputStream::operator<< (unsigned long int i)
{
	_stream << i;
	return *this;
}

UTF8OutputStream& UTF8OutputStream::operator<< (unsigned long long int i)
{
	_stream << i;
	return *this;
}

UTF8OutputStream& UTF8OutputStream::operator<< (double i)
{
	_stream << i;
	return *this;
}
UTF8OutputStream& UTF8OutputStream::operator<< (char c){
	wchar_t wstr[2];
	wstr[0] = (wchar_t)c;
	wstr[1] = 0;
	return this->write(wstr, 1);
}

//////////////////////////////////////////////////////////////////////
// Factory Support.
UTF8OutputStream* UTF8OutputStream::Factory::build(const char* filename, 
												 bool encrypted) {
	if (encrypted) {
		std::ostringstream err;
		err << "This version of SERIF does not support encrypted output files "
			<< "(while trying to read: " << filename << ").";
		throw UnexpectedInputException("UTF8OutputStream::build",
									   err.str().c_str());
	}
	return new UTF8OutputStream(filename);
}

boost::shared_ptr<UTF8OutputStream::Factory> &UTF8OutputStream::_factory() {
	static boost::shared_ptr<Factory> factory(new Factory());
	return factory;
}

void UTF8OutputStream::setFactory(boost::shared_ptr<Factory> factory) {
	_factory() = factory; 
}

// Build methods
UTF8OutputStream *UTF8OutputStream::build(const char* filename, bool encrypted) {
	return _factory()->build(filename, encrypted); }
UTF8OutputStream *UTF8OutputStream::build(std::string filename, bool encrypted) {
	return build(filename.c_str(), encrypted); }
UTF8OutputStream *UTF8OutputStream::build(std::wstring filename, bool encrypted) {
	return build(UnicodeUtil::toUTF8StdString(filename), encrypted); }
UTF8OutputStream *UTF8OutputStream::build(const wchar_t* filename, bool encrypted) {
	return build(std::wstring(filename), encrypted); }
