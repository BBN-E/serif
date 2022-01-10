// Copyright 2015 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/reader/SectionHeadersDocumentZoner.h"
#include "Generic/common/InputUtil.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/LocatedString.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Region.h"

#include <boost/foreach.hpp>

SectionHeadersDocumentZoner::SectionHeadersDocumentZoner() : _last_section_max_length(0) {
	// Read the section header patterns file
	std::set<std::vector<std::wstring> > patterns = InputUtil::readColumnFileIntoSet(ParamReader::getRequiredParam("document_zoner_section_headers_patterns"), false, L"\t");
	for (std::set<std::vector<std::wstring> >::const_iterator pattern_i = patterns.begin(); pattern_i != patterns.end(); ++pattern_i) {
		Symbol sectionName(pattern_i->at(0).c_str());
		boost::wregex pattern(pattern_i->at(1), boost::regex::icase);
		_patterns.insert(std::make_pair<Symbol, boost::wregex>(sectionName, pattern));
	}

	// Read the optional threshold for the length of the last detected section
	_last_section_max_length = ParamReader::getOptionalIntParamWithDefaultValue("document_zoner_last_section_max_length", 0);
}

void SectionHeadersDocumentZoner::process(DocTheory* docTheory) {
	// Get the full document text
	Document* document = docTheory->getDocument();
	const LocatedString* contents = document->getOriginalText();

	// Loop through the document's content, finding section headers
	std::vector<SectionHeader> headers;
	for (int index = 0; index < contents->length() - 1;) {
		// Find the bounds of this line
		SectionHeader header;
		header.start = contents->startOfLine(index);
		header.end = contents->endOfLine(index);
		if (header.start == header.end || header.end - header.start > 40) {
			// Skip empty lines and long lines
			index = contents->startOfNextNonEmptyLine(header.end);
			continue;
		}

		// Get section header candidate line
		std::wstring possibleSectionHeaderLine = contents->substringAsWString(header.start, header.end);

		// Match section header using patterns
		for (Symbol::HashMap<boost::wregex>::const_iterator pattern_i = _patterns.begin(); pattern_i != _patterns.end(); ++pattern_i) {
			if (boost::regex_match(possibleSectionHeaderLine, pattern_i->second)) {
				// Store this section header and its offsets
				header.tag = pattern_i->first;
				SessionLogger::dbg("document-zoner") << "Line '" << possibleSectionHeaderLine << "' matched section header pattern '" << header.tag << "' at [" << header.start << "," << header.end << "]\n";
				headers.push_back(header);
				break;
			}
		}

		// Next line
		index = contents->startOfNextNonEmptyLine(header.end);
	}

	// Create regions in between each section header (assumption: these are the named sections)
	std::vector<Region*> regions;
	int start = 0;
	Symbol tag = Symbol(L"document-start");
	BOOST_FOREACH(SectionHeader header, headers) {
		// Check that this header does not occur immediately after previous header
		int end = contents->endOfPreviousNonEmptyLine(header.start);
		if (end > start) {
			// Create a region from this substring (and free it: Region constructor makes a copy)
			LocatedString* regionString = contents->substring(start, end);
			Region* region = _new Region(document, tag, static_cast<int>(regions.size()), regionString);
			SessionLogger::dbg("document-zoner") << "Created Region " << regions.size() << ": " << tag << "[" << start << "," << end << "]\n";
			regions.push_back(region);
			delete regionString;
		}

		// Move to the end of this section header, the start of the next section
		start = contents->startOfNextNonEmptyLine(header.end);
		tag = header.tag;
	}

	// The last section goes to the end of the document, unless the section header is the last line
	if (start < contents->length() - 1) {
		LocatedString* regionString = contents->substring(start);
		if (_last_section_max_length > 0 && regionString->length() > _last_section_max_length) {
			// We don't consider this a reliable section detection, so don't tag it with whatever matched
			tag = Symbol(L"document-end");
		}
		Region* region = _new Region(document, tag, static_cast<int>(regions.size()), regionString);
		SessionLogger::dbg("document-zoner") << "Created Region " << regions.size() << ": " << tag << "[" << start << "," << contents->length() << "]\n";
		regions.push_back(region);
		delete regionString;
	}

	// Update the document regions
	document->takeRegions(regions);
}
