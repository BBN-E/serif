// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DTRA_DOCUMENT_READER_H
#define DTRA_DOCUMENT_READER_H

#include "Generic/common/UnexpectedInputException.h"
#include "Generic/reader/DocumentReader.h"
#include "Generic/reader/SGMLTag.h"
#include "Generic/reader/RegionSpanCreator.h"
#include "Generic/theories/Metadata.h"

#include <set>

typedef std::pair<int, int> RegionBound;

class DTRADocumentReader : public DocumentReader {
public:
	DTRADocumentReader();
	DTRADocumentReader(Symbol defaultInputType);
	virtual Document* readDocument(InputStream & strm, const wchar_t * filename);
	virtual Document* readDocument(const wchar_t * docString, const wchar_t * filename);
	virtual Document* readDocument(const LocatedString *source, const wchar_t * filename);

protected:
	//Entry point
	void identifyRegions(Document* doc, const LocatedString* source);
	void cleanRegion(LocatedString* region, Metadata* metadata);
	void cleanEntityType(Symbol* entityType);

	//Extract specific metadata fields from the XML
	LocatedString* getDocumentDate(const LocatedString* source);

private:
	//The document ID element name
	//  <ingestedID>
	Symbol docidTag;

	//The date/time text element name
	//  <sourceDate>
	Symbol datetimeTag;

	//The search string of valid whitespace characters
	std::wstring whitespace;

	//The set of SGML tags we want to ignore
	std::set<Symbol> regionsToIgnore;
	std::set<Symbol> spansToIgnore;

	//The set of SGML tags that force a region break
	std::set<Symbol> regionBreakTags;

	// The mapping of URI namespaces to dynamic EntityTypes
	typedef Symbol::HashMap<Symbol> NamespaceMap;
	NamespaceMap prontexNamespaceMap;
};
#endif
