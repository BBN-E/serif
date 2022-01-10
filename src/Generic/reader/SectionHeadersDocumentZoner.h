// Copyright 2015 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SECTION_HEADERS_DOCUMENT_ZONER_H
#define SECTION_HEADERS_DOCUMENT_ZONER_H

#include "Generic/reader/DocumentZoner.h"
#include "Generic/common/Symbol.h"

#include <boost/regex.hpp>

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

typedef Symbol::HashMap<boost::wregex> SymbolRegexMap;

class SERIF_EXPORTED SectionHeadersDocumentZoner : public DocumentZoner {
public:
	// Override the constructor to read the section header patterns
	SectionHeadersDocumentZoner();

	// Override the process method
	virtual void process(DocTheory* docTheory);

	struct SectionHeader {
		Symbol tag;
		int start;
		int end;
	};

private:
	// Store the loaded section header patterns
	SymbolRegexMap _patterns;

	// Store the maximum length of the last section in the document
	int _last_section_max_length;
};

#endif
