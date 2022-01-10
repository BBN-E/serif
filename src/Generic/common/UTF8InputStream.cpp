// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/UnicodeUtil.h"

#ifdef _WIN32
#include <Windows.h>
#define sleep(x) Sleep((x)*1000)
#else
#include <unistd.h>
#endif

/* */
// utf8-codec definitions
// we only want this in one object file, so it's placed here.
#if defined(_WIN32)
#include <boost/version.hpp>
#if BOOST_VERSION < 105000
# include <libs/detail/utf8_codecvt_facet.cpp>
#else
# include <boost/detail/utf8_codecvt_facet.ipp>
#endif
#else
# include "linuxPort/utf8_codecvt_facet.h"
#endif //!WIN32

#include <sstream>

size_t UTF8InputStream::_openFileRetries = 0;
void UTF8InputStream::setOpenFileRetries(size_t n) { _openFileRetries = n; }

UTF8InputStream::UTF8InputStream() {
	std::locale old_locale;
	std::locale utf8_locale( old_locale, new boost::utf8::utf8_codecvt_facet );
	this->imbue( utf8_locale );

	return;
}

UTF8InputStream::UTF8InputStream( const std::string filename ) {
	std::locale old_locale;
	std::locale utf8_locale( old_locale, new boost::utf8::utf8_codecvt_facet );
	this->imbue( utf8_locale );
	this->open( filename.c_str() );
	return;
}

UTF8InputStream::UTF8InputStream( const std::wstring filename ) {
	std::locale old_locale;
	std::locale utf8_locale( old_locale, new boost::utf8::utf8_codecvt_facet );
	this->imbue( utf8_locale );
	this->open( filename.c_str() );
	return;
}

UTF8InputStream::UTF8InputStream( const char * filename ) {
	std::locale old_locale;
	std::locale utf8_locale( old_locale, new boost::utf8::utf8_codecvt_facet );
	this->imbue( utf8_locale );
	if (filename)
		this->open( filename );
	return;
}

UTF8InputStream::UTF8InputStream( const wchar_t * filename ) {
	std::locale old_locale;
	std::locale utf8_locale( old_locale, new boost::utf8::utf8_codecvt_facet );
	this->imbue( utf8_locale );
	if (filename)
		this->open( filename );
	return;
}	

class CannotOpenFileException : public UnexpectedInputException {
public:
	CannotOpenFileException(const char* loc, const char* msg) :
	  UnexpectedInputException(loc, msg) {}
};

// The c++ std dictates that open filestreams must be explicitly closed before
// they may open another file. However, the old UTF8InputStream interface would
// close as required on a call to open(), so we must mimic that behavior.
void UTF8InputStream::open( const char * file ){

	if( this->is_open() )
		this->close();

	std::wifstream::open( file, std::ios::binary );
	if( (!this->is_open()) || (this->fail()) ){
		if (_openFileRetries > 0) {
			unsigned int delay = 1; // one second
			for (size_t retryNum=0; retryNum<_openFileRetries; retryNum++) {
				std::cerr << "Unable to open \"" << file << "\"; retrying in " << delay << " seconds." << std::endl;
				sleep(delay);
				delay *= 2;
				this->clear();
				std::wifstream::open( file, std::ios::binary );
				if (this->is_open() && (!this->fail()))
					break;
			}
		}
		if( (_openFileRetries==0) || (!this->is_open()) || (this->fail()) ){
			std::stringstream errmsg;
			errmsg << "UTF8InputStream (char) failed to open '" << file << "'";
			throw CannotOpenFileException( "UTF8InputStream::open()", errmsg.str().c_str() );
		}
	}

	registerFileOpen(file);
}

void UTF8InputStream::open( const wchar_t * file ){
	open(OutputUtil::convertToUTF8BitString(file).c_str());
}

namespace {
	bool& fileTracking() {
		static bool _file_tracking = false;
		return _file_tracking;
	}
	std::vector<std::string> &openedFiles() {
		static std::vector<std::string> _openedFiles;
		return _openedFiles;
	}
}

void UTF8InputStream::startTrackingFileOpens() { fileTracking() = true; }
void UTF8InputStream::stopTrackingFileOpens() { fileTracking() = false; }

const std::vector<std::string> &UTF8InputStream::getOpenedFiles() {
	return openedFiles();
}

void UTF8InputStream::registerFileOpen(const char* filename) {
	if (fileTracking())
		openedFiles().push_back(filename);
}
void UTF8InputStream::registerFileOpen(const wchar_t* filename) {
	if (fileTracking())
		openedFiles().push_back(OutputUtil::convertToUTF8BitString(filename));
}

bool UTF8InputStream::is_open() {
	return std::wifstream::is_open();
}
std::wstreambuf* UTF8InputStream::rdbuf() {
	return std::wifstream::rdbuf();
}

//////////////////////////////////////////////////////////////////////
// Factory Support.

UTF8InputStream* UTF8InputStream::Factory::build(const char* filename, 
												 bool encrypted) {
	if (encrypted) {
		std::ostringstream err;
		err << "This version of SERIF does not support encrypted input files "
			<< "(while trying to read: " << filename << ").";
		throw UnexpectedInputException("UTF8InputStream::build",
									   err.str().c_str());
	}
	return _new UTF8InputStream(filename);
}

boost::shared_ptr<UTF8InputStream::Factory> &UTF8InputStream::_factory() {
	static boost::shared_ptr<Factory> factory(new Factory());
	return factory;
}

void UTF8InputStream::setFactory(boost::shared_ptr<Factory> factory) {
	_factory() = factory; 
}

// Build methods
UTF8InputStream *UTF8InputStream::build(const char* filename, bool encrypted) {
	return _factory()->build(filename, encrypted); }
UTF8InputStream *UTF8InputStream::build(std::string filename, bool encrypted) {
	return build(filename.c_str(), encrypted); }
UTF8InputStream *UTF8InputStream::build(std::wstring filename, bool encrypted) {
	return build(UnicodeUtil::toUTF8StdString(filename), encrypted); }
UTF8InputStream *UTF8InputStream::build(const wchar_t* filename, bool encrypted) {
	return build(std::wstring(filename), encrypted); }


