// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef INQA_DOCUMENT_READER_H
#define INQA_DOCUMENT_READER_H

#include "Generic/common/UnexpectedInputException.h"
#include "Generic/reader/DocumentReader.h"
#include "Generic/reader/SGMLTag.h"
#include "Generic/reader/RegionSpanCreator.h"
#include "Generic/theories/Metadata.h"

#include <vector>

class INQADocumentReader : public DocumentReader {
public:
	INQADocumentReader();
	INQADocumentReader(Symbol defaultInputType);
	Document* readDocument(InputStream & strm, const wchar_t * filename);

protected:
	//Entry point
	void identifyRegions(Document* doc, LocatedString* source);

	//Extract specific metadata fields from the XML
	LocatedString* getDocumentDate(const LocatedString source);
	LocatedString* getDocumentID(const LocatedString source);

	//Extract the text value of an SGML element
	LocatedString* getTextElementContents(const LocatedString source, const Symbol elementName);

	//Preprocess the document string to handle INQA's HTML-in-XML
	std::wstring cleanDocument(std::wstring text);
	static std::wstring replace_all(std::wstring text, const std::wstring search, const std::wstring replace, const size_t start);
	static std::wstring replace_recursive(std::wstring text, const std::wstring search, const std::wstring replace, const size_t start);
	//Postprocess a region in-place to remove other HTML tags
	virtual void cleanRegion(LocatedString* region);

private:
	//The document ID element name
	//  <ingestedID>
	Symbol docidTag;

	//The date/time text element name
	//  <sourceDate>
	Symbol datetimeTag;

	//The HTML-in-XML element containing all regions
	//  <BODY>
	Symbol regionContainer;

	//Valid region wrapper tags
	//  <P> - This divides the doc into actual regions
	//  <BR> - In some docs, one or more of these indicates a paragraph break
	//
	//  The rest we more-or-less ignore but need to keep out of regions?
	//  <A>, <B>, <DIV>, <EM>, <FONT>, <I>, <IMG>, <LI>,
	//  <OL>, <PRE>, <REFERENCE>, <STRONG>, <SUP>, <TABLE>,
	//  <TD>, <TR>, <U>, <UL>
	static const int nRegionBreakers = 2;
	Symbol regionBreakers[nRegionBreakers];

	//The search string of valid whitespace characters
	std::wstring whitespace;
};
#endif
