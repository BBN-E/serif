// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef VIKTRS_DOCUMENT_READER_H
#define VIKTRS_DOCUMENT_READER_H

#include "Generic/reader/DocumentReader.h"
#include <xercesc/util/XercesDefs.hpp>

// Forward declarations.
XERCES_CPP_NAMESPACE_BEGIN
class DOMDocument;
class DOMElement;
XERCES_CPP_NAMESPACE_END

class VIKTRSDocumentReader : public DocumentReader { 
public:
	VIKTRSDocumentReader();
	VIKTRSDocumentReader(Symbol defaultInputType);
	Document* readDocument(InputStream &strm, const wchar_t * filename);
	Document* readDocumentFromByteStream(std::istream &strm, const wchar_t * filename);

	// Remember to call xmlStory->release() after you read the document!
	virtual Document* readDocumentFromDom(xercesc::DOMDocument* xmlStory, Symbol docName);

	virtual bool prefersByteStream() { return true; }

private:
	struct RegionSpec {
		int start, end;
		Symbol tag;
		size_t sentno;
		size_t stepno;
		RegionSpec(int start, int end, Symbol tag, size_t sentno=0, size_t stepno=0):
		start(start), end(end), tag(tag), sentno(sentno), stepno(stepno) {}
	};

	bool _one_region_per_sentence;

	RegionSpec addRegion(const wchar_t* text, Symbol tag, std::wstring& storyText);
	std::vector<RegionSpec> addSentences(xercesc::DOMDocument* xmlStory, std::wstring &storyText);
};

#endif
