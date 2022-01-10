// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// MS considers std::copy deprecated, for their strange reasons. It's safely
// used here, and there's no cross-platform alternative.
#pragma warning(disable: 4996)

#include <boost/regex.hpp>

#include "Generic/common/leak_detection.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/reader/DTRADocumentReader.h"
#include "Generic/theories/Region.h"
#include "Generic/preprocessors/NamedSpanCreator.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <stack>

DTRADocumentReader::DTRADocumentReader() {
	//Define the valid date/time tags
	//  INQA documents have several possible dates,
	//  currently we're using the date associated with the source,
	//  which may not be the date in the document ID.
	this->datetimeTag = Symbol(L"SOURCEDATE");

	//Define the valid document ID tags
	//  INQA documents have several possible identifiers,
	//  but only ingestedID is guaranteed to be corpus-unique.
	this->docidTag = Symbol(L"INGESTEDID");

	//Define the list of ignorable whitespace characters
	this->whitespace = std::wstring(L" \n\r\t");

	//Get the optional list of SGML-like elements whose contents should be ignored
	//  We can ignore whole regions, which breaks sentences, or just spans within a region
	std::vector<std::wstring> regions_to_ignore = ParamReader::getWStringVectorParam("doc_reader_regions_to_ignore");
	BOOST_FOREACH(std::wstring region_to_ignore, regions_to_ignore) {
		boost::to_upper(region_to_ignore);
		this->regionsToIgnore.insert(Symbol(region_to_ignore));
	}
	std::vector<std::wstring> spans_to_ignore = ParamReader::getWStringVectorParam("doc_reader_spans_to_ignore");
	BOOST_FOREACH(std::wstring span_to_ignore, spans_to_ignore) {
		boost::to_upper(span_to_ignore);
		this->spansToIgnore.insert(Symbol(span_to_ignore));
	}
	if (this->spansToIgnore.empty())
		//If no span ignore tags were specified, use the default <NOPARSE>
		//  There is no default region ignore tag
		this->spansToIgnore.insert(Symbol(L"NOPARSE"));

	//Get the optional list of SGML tags that force a region break
	std::vector<std::wstring> region_break_tags = ParamReader::getWStringVectorParam("doc_reader_region_breaks");
	BOOST_FOREACH(std::wstring region_break_tag, region_break_tags) {
		boost::to_upper(region_break_tag);
		this->regionBreakTags.insert(Symbol(region_break_tag));
	}
	if (this->regionBreakTags.empty())
		//If no region break tags were specified, use the default <BR>
		this->regionBreakTags.insert(Symbol(L"BR"));

	//Read the optional table that maps prontex URI namespaces
	std::string prontex_namespace_map_file = ParamReader::getParam("prontex_namespace_map_file");
	if (!prontex_namespace_map_file.empty()) {
		boost::scoped_ptr<UTF8InputStream> stream(UTF8InputStream::build(prontex_namespace_map_file));
		std::wstring line;
		std::vector<std::wstring> tokens;
		while (getline(*stream, line)) {
			boost::trim(line);
			boost::split(tokens, line, boost::is_any_of("\t"));
			if (tokens.size() != 2) {
				std::wstringstream error;
				error << "Invalid prontex map line '" << line << "'";
				throw UnexpectedInputException("DTRADocumentReader::DTRADocumentReader()", error);
			}
			prontexNamespaceMap[Symbol(tokens[0])] = Symbol(tokens[1]);
		}
	}
}

DTRADocumentReader::DTRADocumentReader(Symbol defaultInputType) {
	throw UnexpectedInputException("DTRADocumentReader::DTRADocumentReader", 
		"defaultInputType not supported by this factory");
}

LocatedString* DTRADocumentReader::getDocumentDate(const LocatedString* source) {
	static const boost::wregex date_re(L"Date:\\s*(\\S+)");	
	const std::wstring text = source->toWString();
	boost::match_results<std::wstring::const_iterator> results;	
	if (boost::regex_search(text, results, date_re)) {
		LocatedString* date = source->substring(static_cast<int>(results[1].first - text.begin()), static_cast<int>(results[1].second - text.begin()));
		return date;
	}
	return NULL;
}

void DTRADocumentReader::cleanRegion(LocatedString* region, Metadata* metadata) {
	// Find any span ignore tags in this region
	SGMLTag tag;
	int search_start = 0;
	std::stack<SGMLTag> tags;
	std::deque<RegionBound> bounds; // deque instead of vector so we remove in reverse order
	while (!(tag = SGMLTag::findNextSGMLTag(*region, search_start)).notFound()) {
		// Is this an element whose contents we want to ignore?
		if (spansToIgnore.find(tag.getName()) != spansToIgnore.end()) {
			if (tag.isOpenTag() && !tag.isCloseTag())
				// If it's an opening tag, keep it
				tags.push(tag);
			else if (tag.isCloseTag() && !tag.isOpenTag() && tags.size() > 0 && tag.getName() == tags.top().getName()) {
				// If it's the matching closing tag, close it and check if we have bounds to skip
				SGMLTag open = tags.top();
				tags.pop();
				if (tags.empty()) {
					// We've closed an outermost element that we care about, so add span bounds around it
					bounds.push_front(RegionBound(open.getStart(), tag.getEnd()));
				}
			}
		}

		// Next iteration will start after this tag
		search_start = tag.getEnd();
	}

	// Actually remove ignored spans
	BOOST_FOREACH(RegionBound bound, bounds) {
		region->remove(bound.first, bound.second);
	}

	// Now find any name tags in this region and convert them to spans
	DataPreprocessor::NamedSpanCreator* nameCreator = _new DataPreprocessor::NamedSpanCreator();
	metadata->addSpanCreator(nameCreator->getIdentifier(), nameCreator);
	search_start = 0;
	Symbol prontexTagName = Symbol(L"PRONTEX");
	while (!tags.empty())
		tags.pop();
	while (!(tag = SGMLTag::findNextSGMLTag(*region, search_start)).notFound()) {
		// Is this an element whose contents we want to ignore?
		if (tag.getName() == prontexTagName) {
			if (tag.isOpenTag() && !tag.isCloseTag())
				// If it's an opening tag, keep it
				tags.push(tag);
			else if (tag.isCloseTag() && !tag.isOpenTag() && tags.size() > 0 && tag.getName() == tags.top().getName()) {
				// If it's the matching closing tag, close it and check if we have bounds to skip
				SGMLTag open = tags.top();
				tags.pop();
				if (tags.empty()) {
					// We've closed an outermost element that contains a name, so create a span for it
					Symbol entityTypes[3];
					entityTypes[0] = open.getAttributeValue(L"class");
					cleanEntityType(entityTypes);
					EDTOffset name_start = region->firstStartOffsetStartingAt<EDTOffset>(open.getEnd());
					EDTOffset name_end = region->lastEndOffsetEndingAt<EDTOffset>(tag.getStart() - 1);
					metadata->newSpan(nameCreator->getIdentifier(), name_start, name_end, entityTypes);
				}
			}
		}

		// Next iteration will start after this tag
		search_start = tag.getEnd();
	}

	// Remove any remaining markup
	SGMLTag::removeSGMLTags(*region);
}

void DTRADocumentReader::cleanEntityType(Symbol* entityType) {
	// Separate the namespace and local name
	std::wstring entityTypeString(entityType[0].to_string());
	if (!boost::starts_with(entityTypeString, L"http://"))
		return;
	std::vector<std::wstring> parts;
	Symbol nameSpace;
	Symbol localName;
	boost::split(parts, entityTypeString, boost::is_any_of(L"#"));
	if (parts.size() == 2) {
		nameSpace = parts[0];
		localName = parts[1];
	} else {
		// Maybe the local name is just last part
		size_t lastSlash = entityTypeString.find_last_of(L"/");
		if (lastSlash != std::wstring::npos) {
			nameSpace = entityTypeString.substr(0, lastSlash);
			localName = entityTypeString.substr(lastSlash + 1);
		} else {
			return;
		}
	}

	// Map the namespace
	NamespaceMap::const_iterator iter = prontexNamespaceMap.find(nameSpace);
	if (iter != prontexNamespaceMap.end()) {
		entityType[1] = (*iter).second;
		entityType[2] = localName;
	}
}

void DTRADocumentReader::identifyRegions(Document* doc, const LocatedString* source) {
	// Initialize the region metadata
	Metadata* metadata = doc->getMetadata();
	Symbol regionSpanSymbol = Symbol(L"REGION_SPAN");
	metadata->addSpanCreator(regionSpanSymbol, _new RegionSpanCreator());

	// Regexes
	static const boost::wregex metadata_re(L"\\$?[A-Za-z]+:.*");
	static const boost::wregex whitespace_re(L"\\s+");

	// Skip past blank lines or lines that match our regular expression
	int region_start = 0;
	while (region_start < source->length()) {
		int newline_index = source->indexOf(L"\n", region_start);
		if (newline_index == -1) {
			break; // No more newlines, must be a content-less document?
		}
		std::wstring text = source->substringAsWString(region_start, newline_index);
		if (!(boost::regex_match(text, metadata_re) || boost::regex_match(text, whitespace_re))) {
			break; // We didn't match one of these, so we break out
		}
		region_start = newline_index+1;
	}

	// If we hit the end of the document without finding a start, keep the whole document
	if (region_start >= source->length())
		region_start = 0;

	// Track SGML tags that we care about
	SGMLTag tag;
	int search_start = region_start;
	std::stack<SGMLTag> tags;
	std::vector<RegionBound> bounds;
	while (!(tag = SGMLTag::findNextSGMLTag(*source, search_start)).notFound()) {
		// Is this an element whose contents we want to ignore?
		if (regionsToIgnore.find(tag.getName()) != regionsToIgnore.end()) {
			if (tag.isOpenTag() && !tag.isCloseTag())
				// If it's an opening tag, keep it
				tags.push(tag);
			else if (tag.isCloseTag() && !tag.isOpenTag() && tag.getName() == tags.top().getName()) {
				// If it's the matching closing tag, close it and check if we have bounds to skip
				SGMLTag open = tags.top();
				tags.pop();
				if (tags.empty()) {
					// We've closed an outermost element that we care about, so add region bounds around it
					if (region_start < open.getStart())
						bounds.push_back(RegionBound(region_start, open.getStart()));
					region_start = tag.getEnd();
				}
			}
		} else if (regionBreakTags.find(tag.getName()) != regionBreakTags.end() && tag.isOpenTag()) {
			// Insert a region break around this tag
			if (region_start < tag.getStart())
				bounds.push_back(RegionBound(region_start, tag.getStart()));
			region_start = tag.getEnd();
		}

		// Next iteration will start after this tag
		search_start = tag.getEnd();
	}
	if (region_start < source->length() - 2)
		bounds.push_back(RegionBound(region_start, source->length() - 1));
	
	// Create and store the regions - unless there are skipped tags, there will be only one
	Symbol name = Symbol(L"Body");
	std::vector<Region*> regions;
	BOOST_FOREACH(RegionBound bound, bounds) {
		LocatedString* regionSubstring = source->substring(bound.first, bound.second);
		cleanRegion(regionSubstring, metadata);
		Region* region = _new Region(doc, name, static_cast<int>(regions.size()), regionSubstring);
		metadata->newSpan(regionSpanSymbol, region->getStartEDTOffset(), region->getEndEDTOffset(), &name);
		region->setSpeakerRegion(false);
		region->setReceiverRegion(false);
		regions.push_back(region);
		delete regionSubstring;
	}

	// Done - give the regions to the document
	doc->takeRegions(regions);
}

Document* DTRADocumentReader::readDocument(InputStream& stream, const wchar_t* filename) {

	//Read the entire document stream into a regular string
	stream.seekg(0, std::ios_base::end);
	long streamLength = (long)stream.tellg();
	wchar_t* docCharacters = _new wchar_t[streamLength + 1];
	wmemset(docCharacters, L'\0', streamLength + 1);
	stream.seekg(0, std::ios_base::beg);
	stream.read(docCharacters, streamLength);

	//Check the document
	if (streamLength <= 0) {
		std::wstring filename_as_wstring(filename);		
		std::string filename_as_string(filename_as_wstring.begin(), filename_as_wstring.end());
		throw UnexpectedInputException("DTRADocumentReader::readDocument()", 
									   "Empty document in file ", filename_as_string.c_str());
	}

	//Read the document as a string
	return readDocument(docCharacters, filename);	
}

Document* DTRADocumentReader::readDocument(const wchar_t* docString, const wchar_t* filename) {
	//create a string for the source text
	LocatedString* docLocatedString = _new LocatedString(docString);
	return readDocument(docLocatedString, filename);
}

Document* DTRADocumentReader::readDocument(const LocatedString* docLocatedString, const wchar_t* filename) {

	//Use the extensionless filename as the document ID	
	std::wstring docid(filename);
	docid = docid.substr(docid.rfind(L"\\") + 1);
	docid = docid.substr(docid.rfind(L"/") + 1);
	docid = docid.substr(0, docid.rfind(L"."));
	//std::wcout << "    Filename ID: " << docid << std::endl;

	Symbol docidSymbol = Symbol(docid.c_str());

	//create the actual Document object
	Document* result = _new Document(docidSymbol);

	//Add the read document date, if any
	SessionLogger::dbg("dtra") << "  Storing document date...";
	LocatedString* dateLocatedString = this->getDocumentDate(docLocatedString);
	if (dateLocatedString != NULL) {
		SessionLogger::dbg("dtra") << "    " << dateLocatedString->toString() << std::endl;
		result->setDateTimeField(dateLocatedString);
		delete dateLocatedString;
	}

	//Store the read document and metadata
	result->setOriginalText(docLocatedString);

	//read the document regions
	identifyRegions(result, docLocatedString);

	//Done reading
	return result;
}
