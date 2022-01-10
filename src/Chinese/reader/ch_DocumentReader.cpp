// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/reader/ch_DocumentReader.h"
#include "Generic/theories/Document.h"
#include "Generic/common/InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/Symbol.h"
#include <wchar.h>


LocatedString* ChineseDocumentReader::_regions[MAX_DOC_REGIONS];

class ChineseDocumentReader::XMLTag;
using ChineseDocumentReader::XMLTag;

ChineseDocumentReader::~ChineseDocumentReader() { }

Document* ChineseDocumentReader::readDocument(InputStream &stream) {
	XMLTag tag;
	Symbol name;

	LocatedString source(stream);

	// Find the file ID.
	tag = findOpenTag(&source, Symbol(L"DOCID"), 0);
	if (tag.notFound()) {
		
		tag = findOpenTag(&source, Symbol(L"DOCNO"), 0);

		if (tag.notFound()) {
			name = cleanFileName(stream.getFileName());
		}
		else {
			int end_open_docno = tag.getEnd();

			tag = findCloseTag(&source, Symbol(L"DOCNO"), end_open_docno);
			if (tag.notFound()) {
				throw UnexpectedInputException("ChineseDocumentReader::readDocument()", "DOCNO tag not terminated");
			}
			int start_close_docno = tag.getStart();

			LocatedString *nameString = source.substring(end_open_docno, start_close_docno);
			nameString->trim();
			name = nameString->toSymbol();
			delete nameString;
		}
	}
	else {
		int end_open_docno = tag.getEnd();

		tag = findCloseTag(&source, Symbol(L"DOCID"), end_open_docno);
		if (tag.notFound()) {
			throw UnexpectedInputException("ChineseDocumentReader::readDocument()", "DOCID tag not terminated");
		}
		int start_close_docno = tag.getStart();

		LocatedString *nameString = source.substring(end_open_docno, start_close_docno);
		nameString->trim();
		name = nameString->toSymbol();
		delete nameString;
	}

	int start_next_search = 0;
	int num_regions = 0;

	// Find the document text.
	while (true) {
		
		tag = findOpenTag(&source, Symbol(L"TEXT"), start_next_search); 
		if (tag.notFound()) 
			break;

		int start_text = tag.getEnd();
		tag = findCloseTag(&source, Symbol(L"TEXT"), tag.getEnd());
		int end_text = tag.getStart();
		start_next_search = tag.getEnd();
	
		// Divide the text into regions.
		LocatedString* text = source.substring(start_text, end_text);
		int start_turn = 0;

		do {
			tag = findOpenTag(text, Symbol(L"TURN"), start_turn);
			if (tag.notFound()) {
				_regions[num_regions] = text->substring(start_turn);
				cleanRegion(_regions[num_regions]);
				start_turn = text->length();
			}
			else {
				_regions[num_regions] = text->substring(start_turn, tag.getStart());
				cleanRegion(_regions[num_regions]);
				start_turn = tag.getEnd();
			}
			num_regions++;
		} while (!tag.notFound() && (num_regions < MAX_DOC_REGIONS));

		// If there were still more tags found then we exceeded the max.
		if (!tag.notFound()) {
			throw UnexpectedInputException("ChineseDocumentReader::readDocument", "too many regions");
		}

		delete text;
	} 

	Document *result = _new Document(name, num_regions, _regions);
	for (int i = 0; i < num_regions; i++) {
		delete _regions[i];
	}
	return result;
}

XMLTag ChineseDocumentReader::findNextXMLTag(const LocatedString *input, int start) {
	const int len = input->length();

	// TODO: handle close tags with space between slash and name

	// Find the next open tag.
	int start_tag = start;
	while ((start_tag < len) && (input->charAt(start_tag) != L'<')) {
		start_tag++;
	}

	// If there wasn't one, return a null tag.
	if (start_tag >= len) {
		return XMLTag(input);
	}

	// Find the beginning of the tag name.
	int start_name = start_tag + 1;
	while ((start_name < len) && iswspace(input->charAt(start_name))) {
		start_name++;
	}

	// If there wasn't a name, return a null tag.
	if (start_name >= len) {
		return XMLTag(input);
	}

	// Find the end of the tag name.
	int end_name = start_name;
	while ((end_name < len) &&
		   !iswspace(input->charAt(end_name)) &&
		   (input->charAt(end_name) != L'>'))
	{
		end_name++;
	}

	// Find the end of the tag.
	int end_tag = end_name;
	while ((end_tag < len) && (input->charAt(end_tag) != L'>')) {
		end_tag++;
	}

	// If there was no closing bracket, this wasn't really a tag.
	if (end_tag >= len) {
		return XMLTag(input);
	}

	// Put end_tag at one beyond the end.
	end_tag++;

	// Is it a close tag?
	bool close = false;
	if (input->charAt(start_name) == L'/') {
		start_name++;
		close = true;
	}


	// Get the tag name.
	LocatedString *nameString = input->substring(start_name, end_name);
	Symbol name = nameString->toSymbol();
	delete nameString;

	return XMLTag(input, name, start_tag, end_tag, close);
}

XMLTag ChineseDocumentReader::findCloseTag(const LocatedString *input, Symbol name, int start) {
	XMLTag tag;
	do {
		tag = findNextXMLTag(input, start);
		start = tag.getEnd();
	} while (!tag.notFound() && ((tag.getName() != name) || !(tag.isCloseTag())));
	return tag;
}

XMLTag ChineseDocumentReader::findOpenTag(const LocatedString *input, Symbol name, int start) {
	XMLTag tag;
	do {
		tag = findNextXMLTag(input, start);
		start = tag.getEnd();
	} while (!tag.notFound() && ((tag.getName() != name) || !(tag.isOpenTag())));
	return tag;
}

void ChineseDocumentReader::cleanRegion(LocatedString *region) {
	// Remove any annotations.
	XMLTag openTag;
	do {
		openTag = findOpenTag(region, Symbol(L"ANNOTATION"), 0);
		if (!openTag.notFound()) {
			XMLTag closeTag = findCloseTag(region, Symbol(L"ANNOTATION"), openTag.getEnd());
			if (closeTag.notFound()) {
				region->remove(openTag.getStart(), region->length());
				return;
			}
			// Remove the annotation.
			region->remove(openTag.getStart(), closeTag.getEnd());
		}
	} while (!openTag.notFound());

	// Remove any other tags whatsoever.
	XMLTag next;
	do {
		next = findNextXMLTag(region, 0);
		region->remove(next.getStart(), next.getEnd());
	} while (!next.notFound());

	// TODO: interpret known SGML entities (&amp;, etc)
}

Symbol ChineseDocumentReader::cleanFileName(const char* file) const {
	int i, j;
	wchar_t* w_file = _new wchar_t[strlen(file)+1];
	
	for (i = static_cast<int>(strlen(file)); i > 1; i--) {
		if (file[i-1] == '/' || file[i-1] == '\\') 
			break;
	}
	for (j = 0; j + i < static_cast<int>(strlen(file)); j++)
		w_file[j] = (wchar_t)file[j+i];
	w_file[j] = L'\0';
	
	Symbol name = Symbol(w_file);
	delete [] w_file;

	return name;
}
