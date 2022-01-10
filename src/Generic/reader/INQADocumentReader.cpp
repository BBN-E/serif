// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// MS considers std::copy deprecated, for their strange reasons. It's safely
// used here, and there's no cross-platform alternative.
#pragma warning(disable: 4996)

#include "Generic/common/leak_detection.h"
#include "Generic/common/LocatedString.h"

#include "Generic/reader/INQADocumentReader.h"

#include "Generic/theories/Region.h"

INQADocumentReader::INQADocumentReader() {
	//Define the valid date/time tags
	//  INQA documents have several possible dates,
	//  currently we're using the date associated with the source,
	//  which may not be the date in the document ID.
	this->datetimeTag = Symbol(L"SOURCEDATE");

	//Define the valid document ID tags
	//  INQA documents have several possible identifiers,
	//  but only ingestedID is guaranteed to be corpus-unique.
	this->docidTag = Symbol(L"INGESTEDID");

	//Define the text-containing HTML block in the <text> XML element
	this->regionContainer = Symbol(L"BODY");

	//Define the region breaking tags
	//  Both open and close tags indicate a region boundary
	this->regionBreakers[0] = Symbol(L"P");
	this->regionBreakers[1] = Symbol(L"BR");

	//Tags we currently don't think are important, but are in these docs
	/*
	this->regionBreakers[2] = Symbol(L"A");
	this->regionBreakers[3] = Symbol(L"B");
	this->regionBreakers[4] = Symbol(L"DIV");
	this->regionBreakers[5] = Symbol(L"EM");
	this->regionBreakers[6] = Symbol(L"FONT");
	this->regionBreakers[7] = Symbol(L"I");
	this->regionBreakers[8] = Symbol(L"IMG");
	this->regionBreakers[9] = Symbol(L"LI");
	this->regionBreakers[10] = Symbol(L"OL");
	this->regionBreakers[11] = Symbol(L"PRE");
	this->regionBreakers[12] = Symbol(L"REFERENCE");
	this->regionBreakers[13] = Symbol(L"STRONG");
	this->regionBreakers[14] = Symbol(L"SUP");
	this->regionBreakers[15] = Symbol(L"TABLE");
	this->regionBreakers[16] = Symbol(L"TD");
	this->regionBreakers[17] = Symbol(L"TR");
	this->regionBreakers[18] = Symbol(L"U");
	this->regionBreakers[19] = Symbol(L"UL");
	*/

	//Define the list of ignorable whitespace characters
	this->whitespace = std::wstring(L" \n\r\t");
}

INQADocumentReader::INQADocumentReader(Symbol defaultInputType) {
	throw UnexpectedInputException("INQADocumentReader::INQADocumentReader", 
		"defaultInputType not supported by this factory");
}

std::wstring INQADocumentReader::cleanDocument(std::wstring text) {
	//Since this modifies the source before it is a located string,
	//we need to either save this source version or perform the same
	//transformations each time we read.

	//Get the bounds of the document content
	size_t textStart = text.find(L"<text>");

	//Unescape HTML tags in the text content by evaluating XML entities
	// do this one first so we can find all the others
	text = replace_recursive(text, std::wstring(L"&amp;"), std::wstring(L"&"), textStart);

	text = replace_all(text, std::wstring(L"&lt;"), std::wstring(L"<"), textStart);
	text = replace_all(text, std::wstring(L"&gt;"), std::wstring(L">"), textStart);

	//Make normal spaces instead of non-breaking spaces in the HTML
	text = replace_all(text, std::wstring(L"&nbsp;"), std::wstring(L" "), textStart);

	//Done
	return text;
}

void INQADocumentReader::cleanRegion(LocatedString* region) {
	//The current SGML break tag being checked
	SGMLTag tag;

	//The offsets of the current tag
	int tagStart = 0;
	int tagEnd = 0;

	//Loop until we hit the end of the content
	while (!(tag = SGMLTag::findNextSGMLTag(*region, 0)).notFound()) {
		//Get the tag offsets
		tagStart = tag.getStart();
		tagEnd = tag.getEnd();

		//Remove the tag from the region's located string
		//std::wcout << "Removed: " << tag.getName() << " [" << tagStart << ", " << tagEnd << "]" << std::endl;
		region->remove(tagStart, tagEnd);
	}
}

LocatedString* INQADocumentReader::getDocumentDate(const LocatedString source) {
	//Return the extracted date/time
	return this->getTextElementContents(source, this->datetimeTag);
}

LocatedString* INQADocumentReader::getDocumentID(const LocatedString source) {
	//Return the extracted date/time
	return this->getTextElementContents(source, this->docidTag);
}

LocatedString* INQADocumentReader::getTextElementContents(const LocatedString source, const Symbol elementName) {
	//The text contents extracted from the first instance of the element in the source
	LocatedString* elementLocatedString = NULL;

	//The current SGML tag being extracted
	SGMLTag tag;

	//Try to find an opening tag for the specified element
	tag = SGMLTag::findOpenTag(source, elementName, 0);
	if (!tag.notFound()) {
		//Get the string position of the end of the opening tag
		int openingTagEnd = tag.getEnd();

		//Find the closing tag for this element
		tag = SGMLTag::findCloseTag(source, elementName, openingTagEnd);
		if (tag.notFound()) {
			//Malformed SGML
			throw UnexpectedInputException("INQADocumentReader::getTextElementContents()", 
										   "Tag not terminated");
		}

		//Get the string position of the start of the closing tag
		int closingTagStart = tag.getStart();

		//Extract the text content of the element
		elementLocatedString = source.substring(openingTagEnd, closingTagStart);
		elementLocatedString->trim();
	}

	//Return the extracted text
	return elementLocatedString;
}

std::wstring INQADocumentReader::replace_all(std::wstring text, const std::wstring search, const std::wstring replace, const size_t start = 0) {
	//Start at the specified offset
	size_t offset = start;
	size_t result = std::wstring::npos;

	//Determine the potential length of the replaced string
	size_t slen = search.length();
	size_t rlen = replace.length();
	size_t rtextLength = text.length();
	if (rlen > slen) {
		rtextLength *= (size_t) ceil(((double) rlen)/slen);
	}

	//Create the result string
	//std::wstring rtext = std::wstring(rtextLength, wchar_t);

	//Loop through the string, finding instances of the search string
	while ((result = text.find(search, offset)) != std::wstring::npos) {
		//Do the replace
		//std::wcout << result << ": '" << search << "' -> '" << replace << "'" << std::endl;
		text = text.replace(result, search.length(), replace, 0, replace.length());

		//Continue looking
		offset = result	+ 1;
	}

	//Return the modified text
	return text;
}
/// Used to allow replacing "&amp;amp;amp;" with "&" by recursively replacing "&amp;" with "&"
std::wstring INQADocumentReader::replace_recursive(std::wstring text, const std::wstring search, const std::wstring replace, const size_t start = 0){
	//Start at the specified offset
	size_t result;
	size_t offset = start;
	//Loop through the string, finding instances of the search string
	while ((result = text.find(search, offset)) != std::wstring::npos) {
		text = text.replace(result, search.length(), replace, 0, replace.length());
		//Continue looking, including looking at the newly installed text
		offset = result;
	}

	//Return the modified text
	return text;
}
void INQADocumentReader::identifyRegions(Document* doc, LocatedString* source) {
	//Initialize the region metadata
	Metadata* metadata = doc->getMetadata();
	Symbol regionSpanSymbol = Symbol(L"REGION_SPAN");
	metadata->addSpanCreator(regionSpanSymbol, _new RegionSpanCreator());

	//The current SGML break tag being checked
	SGMLTag tag;

	//store the regions
	std::vector<Region*> regions;
	int numRegions = 0;

	//Jump to the beginning of the content
	tag = SGMLTag::findOpenTag(*source, this->regionContainer, 0);
	if (tag.notFound()) {
		throw UnexpectedInputException("INQADocumentReader::readRegions()", 
									   "No region container");
	}

	//The offsets of the current region
	int regionStart = tag.getEnd();
	int regionEnd = regionStart;

	//Loop until we hit the end of the content
	while (!(tag = SGMLTag::findNextSGMLTag(*source, regionEnd)).notFound()) {
		//std::wcout << "Tag: " << tag.getName() << std::endl;

		//Check if this is a region breaking tag
		int breaker = 0;
		for (breaker = 0; breaker < this->nRegionBreakers; breaker++) {
			if (tag.getName() == this->regionBreakers[breaker]) {
				//Region boundary found
				break;
			}
		}

		//Check for some other tag
		if (breaker >= this->nRegionBreakers) {
			//Check if this is the end of the content container
			if (!(tag.isCloseTag() && tag.getName() == this->regionContainer)) {
				//Keep looking past this non-breaking tag
				regionEnd = tag.getEnd();
				continue;
			}
		}

		//Update the region bounds, since both open and close tags denote a region boundary
		regionEnd = tag.getStart();

		//Get the region and clean it up
		LocatedString* regionStr = source->substring(regionStart, regionEnd);
		this->cleanRegion(regionStr);

		//Check if there is non-whitespace content
		std::wstring content = regionStr->toString();
		//std::wcout << "[" << regionStart << ", " << regionEnd << "]: " << content << std::endl;
		if (content.find_first_not_of(this->whitespace, 0) == std::wstring::npos) {
			//Keep looking past this repeated breaking tag/empty region wrapper
			delete regionStr;
			regionStart = tag.getEnd();
			regionEnd = regionStart;
			continue;
		}

		//Store the region
		//std::wcout << "  KEEP!" << std::endl;
		Symbol name = tag.getName();
		regions.push_back(_new Region(doc, name, static_cast<int>(regions.size()), regionStr));
		//as of right now, I do not think we know if this region stores speaker information or reciever information, so mark both as false
		regions.back()->setSpeakerRegion(false);
		regions.back()->setReceiverRegion(false);

		delete regionStr;
		
		//Add a region span to the metadata for this breaking tag/region element
		metadata->newSpan(regionSpanSymbol, EDTOffset(regionStart), EDTOffset(regionEnd), &name);

		//Start looking at the next region, starting after the current region wrapper/breaker tag
		regionStart = tag.getEnd();
		regionEnd = regionStart;
	}

	//Done - give the regions to the document
	doc->takeRegions(regions);
}

Document * INQADocumentReader::readDocument(InputStream &stream, const wchar_t * filename) {
	//std::wcout << "INQADocumentReader::readDocument()" << std::endl;

	//Use the extensionless filename as the document ID
	//  INQA files contain a truncated document ID, but there is additional
	//  numbering in the filename we may need.
	//std::wcout << "  Extracting document ID from filename..." << std::endl;
	std::wstring docid(filename);
	docid = docid.substr(docid.rfind(L"\\") + 1);
	docid = docid.substr(docid.rfind(L"/") + 1);
	docid = docid.substr(0, docid.rfind(L"."));
	//std::wcout << "    Filename ID: " << docid << std::endl;

	//Read the entire document stream into a regular string
	//std::wcout << "  Reading entire document..." << std::endl;
	stream.seekg(0, std::ios_base::end);
	long streamLength = (long)stream.tellg();
	wchar_t* docCharacters = _new wchar_t[streamLength + 1];
	wmemset(docCharacters, L'\0', streamLength + 1);
	stream.seekg(0, std::ios_base::beg);
	stream.read(docCharacters, streamLength);
	std::wstring docString(docCharacters);

	//Check the document
	if (docString.length() <= 0) {
		std::wstring filename_as_wstring(filename);		
		std::string filename_as_string(filename_as_wstring.begin(), filename_as_wstring.end());
		throw UnexpectedInputException("INQADocumentReader::readDocument()", 
									   "Empty document in file ", filename_as_string.c_str());
	}

	//Preprocess the document string, cleaning up the HTML-in-XML
	std::wstring docCleanedString = cleanDocument(docString);

	//create a string for the source text
	LocatedString* docLocatedString = _new LocatedString(docCleanedString.c_str());

	//Get the document ID as specified in the file
	//std::wcout << "  Reading document ID from file..." << std::endl;
	LocatedString* docidLocatedString = this->getDocumentID(*docLocatedString);
	Symbol docidSymbol;
	if (docidLocatedString != NULL) {
		//Use the document ID read from the file's XML metadata
		docidSymbol = docidLocatedString->toSymbol();
		delete docidLocatedString;
		//std::wcout << "    File document ID: " << docidSymbol << std::endl;
	} else {
		//Use the document ID extracted from the filename
		docidSymbol = Symbol(docid.c_str());
	}

	//create the actual Document object
	Document* result = _new Document(docidSymbol);

	//Add the read document date, if any
	//std::wcout << "  Storing document date..." << std::endl;
	LocatedString* dateLocatedString = this->getDocumentDate(*docLocatedString);
	if (dateLocatedString != NULL) {
		//std::wcout << "    " << dateLocatedString->toString() << std::endl;
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
