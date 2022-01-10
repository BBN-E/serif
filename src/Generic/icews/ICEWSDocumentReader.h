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

class ICEWSDocumentReader: public DocumentReader {
public:
	ICEWSDocumentReader();
	ICEWSDocumentReader(Symbol defaultInputType);
	Document* readDocument(InputStream &strm, const wchar_t * filename);
	Document* readDocumentFromByteStream(std::istream &strm, const wchar_t * filename);

	// Remember to call xmlStory->release() after you read the document!
	virtual Document* readDocumentFromDom(xercesc::DOMDocument* xmlStory, Symbol docName);

	Document* readDocumentFromDom(xercesc::DOMDocument* xmlStory, Symbol docName, const wchar_t *publicationDate=0,
		const wchar_t *headline=0, const wchar_t *publisher=0, const wchar_t *sourceName=0);

	virtual bool prefersByteStream() { return true; }

private:
	struct RegionSpec {
		int start, end;
		Symbol tag;
		size_t sentno;
		RegionSpec(int start, int end, Symbol tag, size_t sentno=0):
		start(start), end(end), tag(tag), sentno(sentno) {}
	};

	bool _one_region_per_sentence;
	bool _include_headline;
	bool _include_publisher;
	bool _include_source_name;
	size_t _sentence_cutoff;

	RegionSpec addRegion(const wchar_t* text, Symbol tag, std::wstring& storyText);
	std::vector<RegionSpec> addSentences(xercesc::DOMDocument* xmlStory, std::wstring &storyText);
};

#endif
