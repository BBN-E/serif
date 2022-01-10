// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef UTF8_INPUT_STREAM_H
#define UTF8_INPUT_STREAM_H


/* */

// these defines interact with utf8_codecvt_facet.c/hpp,
// allowing us to control the included code's namespace
#define BOOST_UTF8_BEGIN_NAMESPACE \
namespace boost { namespace utf8 {
#define BOOST_UTF8_END_NAMESPACE \
} }
#define BOOST_UTF8_DECL

// utf8-codec declarations
#include <boost/detail/utf8_codecvt_facet.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>
#include <istream>
#include <fstream>
#include <string>
#include <vector>
#include <streambuf>

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

// This class wraps around a basic_ifstream, imbuing it with boost's utf8 functionality
// via constructor. Additionally, because the old UTF8InputStream was non-standards conforming
// in a number of aspects, there are various fixups and delegates to mimic the old behavior.
class SERIF_EXPORTED UTF8InputStream : public std::wifstream {
public:
	struct Factory {
		virtual UTF8InputStream *build(const char* filename=0, 
									   bool encrypted=false);
	};
	static void setFactory(boost::shared_ptr<Factory> factory);
	static UTF8InputStream *build(const char* filename=0, bool encrypted=false);
	// These build methods are provided for convenience
	static UTF8InputStream *build(std::string filename, bool encrypted=false);
	static UTF8InputStream *build(std::wstring filename, bool encrypted=false);
	static UTF8InputStream *build(const wchar_t* filename, bool encrypted=false);


	
	// The c++ std dictates that open filestreams must be explicitly closed before
	// they may open another file. However, the old UTF8InputStream interface would
	// close as required on a call to open(), so we must mimic that behavior.
	// We additionally throw if the file DNE/fails to open.
	virtual void open( const char * file );
	virtual void open( const wchar_t * file );

	// delegate methods to support the old UTF8InputStream interface
	//  ( old interface used capital letters, std methods do not )
	virtual UTF8InputStream& getLine(std::wstring & str){
		std::getline( *this, str );

		// old getLine interface stripped carriage returns... sigh.
		if( !str.empty() && str[ str.size() - 1 ] == L'\r' )
			str.resize( str.size() - 1 );

		return *this;
	}

	virtual UTF8InputStream& getLine(wchar_t* str, int size){
		this->getline( str, size );

		size_t len = wcslen( str );
		if( len && str[len-1] == L'\r' )
			str[len-1] = L'\0';

		return *this;
	}

	virtual UTF8InputStream& putBack(wchar_t ch){
		this->putback(ch);
		return *this;
	}

	/** Turn on tracking for opened files.  When tracking is turned on, any 
	  * opened files will be appended to a list that is returned by 
	  * UTF8InputStream::getOpenedFiles(). */
	static void startTrackingFileOpens();
	static void stopTrackingFileOpens();

	/** Return a list of files that have been opened while open-file tracking
	  * was turned on.  See trackFileOpens(). */
	static const std::vector<std::string> &getOpenedFiles();

	/** Set a static variable that will tell UTF8InputStream to retry opening
	  * a file if it fails, a specified number of times before giving up.
	  * UTF8InputStream will wait 1 second before the first retry, and will
	  * double the delay for each subsequent retry. */
	static void setOpenFileRetries(size_t n);

	// Make these methods virtual (at least if they're accessed via
	// UTF8InputStream, and not one of its bases):
	virtual bool is_open();
	virtual std::wstreambuf* rdbuf();

	// These must be called whenever a UT8InputStream is opened -- i.e., subclasses
	// must ensure that their open() method, as well as any constructors that take
	// a filename, call one of these two methods.  
	static void registerFileOpen(const char* filename);
	static void registerFileOpen(const wchar_t* filename);


protected:
	// To build a new UTF8InputStream, use UTF8InputStream::build.
	//
	// The std filestream types don't throw exceptions; instead, one calls is_open upon them.
	// However, the old UTF8InputStream throws UnexpectedInputExceptions on non-existent or
	// failing files, so we must as well.
	UTF8InputStream();
	UTF8InputStream( const std::string filename );
	UTF8InputStream( const std::wstring filename );
	UTF8InputStream( const char * filename );
	UTF8InputStream( const wchar_t * filename );

private:
	static boost::shared_ptr<Factory> &_factory();
	static size_t _openFileRetries;
};


#endif
