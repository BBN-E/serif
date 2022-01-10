// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MT_DOCUMENT_READER_H
#define MT_DOCUMENT_READER_H

#include "Generic/reader/DocumentReader.h"
#include "Generic/theories/DocTheory.h"

#include <vector>

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED MTDocumentReader : public DocumentReader {

public:
	MTDocumentReader(LanguageAttribute lang = SerifVersion::getSerifLanguage());
	MTDocumentReader(std::string segment_input_field, std::string segment_input_attribute = "", LanguageAttribute lang = SerifVersion::getSerifLanguage());
	MTDocumentReader(std::wstring segment_input_field, std::wstring segment_input_attribute = L"", LanguageAttribute lang = SerifVersion::getSerifLanguage());
	MTDocumentReader(Symbol defaultInputType);

	// Reads segments until the given input stream is exhausted
	void readSegments( InputStream & stream, std::vector< WSegment > & segments );
	
	// Given segments and an input field name, this method returns a LocatedString constructed by combining the given field in all segments.
	// If the 'regions' parameter is non-null, LocatedString regions bounding each segment are additionally returned by argument reference.
	std::auto_ptr<LocatedString> constructLocatedStringsFromField( const std::vector< WSegment > & segments, std::wstring field_name, std::vector< LocatedString * > * regions = NULL, std::vector< EDTOffset > * region_offsets = NULL, bool preserve_offsets = false );

	Document* readDocument(InputStream & strm, const wchar_t * filename);

private:
	std::wstring _segment_input_field;
	std::wstring _segment_input_attribute;
	LanguageAttribute _languageAttribute;
};
#endif


