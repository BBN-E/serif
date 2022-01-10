// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_DOCUMENT_READER_H
#define ICEWS_DOCUMENT_READER_H

#include "Generic/reader/DocumentReader.h"
#include <xercesc/util/XercesDefs.hpp>

// Forward declarations.
XERCES_CPP_NAMESPACE_BEGIN
	class DOMDocument;
	class DOMElement;
XERCES_CPP_NAMESPACE_END

namespace ICEWS {

	class ICEWSDocumentReader: public DocumentReader {
	public:
		ICEWSDocumentReader();
		Document* readDocument(InputStream &strm, const wchar_t * filename);
		Document* readDocumentFromByteStream(std::istream &strm, const wchar_t * filename);

		// Remember to call xmlStory->release() after you read the document!
		Document* readDocumentFromDom(xercesc::DOMDocument* xmlStory, Symbol docName);

		Document* readDocumentFromDom(xercesc::DOMDocument* xmlStory, Symbol docName, const wchar_t *publicationDate);

		virtual bool prefersByteStream() { return true; }

	private:
		bool _one_region_per_sentence;
	};
}

#endif
