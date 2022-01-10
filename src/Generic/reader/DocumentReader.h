// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DOCUMENT_READER_H
#define DOCUMENT_READER_H

#include <boost/shared_ptr.hpp>
#include <string>
#include <map>
#include <iostream>

#include "Generic/theories/Document.h"
#include "Generic/common/InputStream.h"
#include "Generic/common/UTF8InputStream.h"

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

/** The DocumentReader class defines a separate factory hook for each 
 * "source_format" values.  By default, readers are registered for the
 * following source_formats: segments, inqa, sgm.  (sgm is the default
 * source_format.) */
class SERIF_EXPORTED DocumentReader {
public:
	/** Create and return a new DocumentReader. */
	static DocumentReader *build() { return build("sgm"); }
	static DocumentReader *build(const char* source_format);

	/** Hook for registering new DocumentReader factories */
	struct Factory { 
		virtual ~Factory() {}
		virtual DocumentReader *build() = 0;
	};		
	static void setFactory(const char* source_format, boost::shared_ptr<Factory> factory);
	static bool hasFactory(const char* source_format);

	virtual ~DocumentReader() {}
    
	/**
	 * This does the work. It reads one document from 
	 * the input stream and returns a Document object.
	 * The client is responsible for deleting
	 * the Document.
     **/
	virtual Document* readDocument(InputStream &strm, const wchar_t * filename) = 0;
	virtual Document* readDocumentFromByteStream(std::istream &strm, const wchar_t * filename) {
		throw InternalInconsistencyException("DocumentReader::readDocument()",
			"This document reader can only read character (not byte) streams."); }

	/** Helper method: read from a file.  (Not virtual -- do not override) */
	Document* readDocumentFromFile(const wchar_t* filename);

	/** Helper method: read from a string.  (Not virtual -- do not override) */
	Document* readDocumentFromWString(const std::wstring& contents, const wchar_t *filename);

	/** Return true if this document reader prefers to read from byte streams 
	  * (using readDocumentFromByteStream) rather than from character streams
	  * (using readDocument).  This can be overridden by subclasses in cases
	  * where it's more appropriate to read a byte stream (eg when parsing
	  * XML with xerces). */
	virtual bool prefersByteStream() { return false; }

protected:
	DocumentReader() {}

private:
	typedef std::map<std::string, boost::shared_ptr<Factory> > FactoryMap; 
	static boost::shared_ptr<Factory> &_factory(const char* source_format);
	static FactoryMap &_factoryMap();
};

// language-specific includes determine which implementation is used
///*#ifdef ENGLISH_LANGUAGE
//	#include "English/reader/en_DocumentReader.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/reader/ch_DocumentReader.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/reader/ar_DocumentReader.h"*/
//
//#include "Generic/reader/xx_DocumentReader.h"

#endif
