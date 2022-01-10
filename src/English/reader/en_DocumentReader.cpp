// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/reader/en_DocumentReader.h"
#include "Generic/reader/SGMLTag.h"
#include "Generic/theories/Document.h"
#include "Generic/common/InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/ParamReader.h"
#include <wchar.h>

LocatedString* EnglishDocumentReader::_regions[MAX_DOC_REGIONS];


EnglishDocumentReader::EnglishDocumentReader() { 
	Symbol input = ParamReader::getParam(Symbol(L"input_type"));
	if (input.is_null()) {
		_inputType = Symbol(L"text");
	} else {
		_inputType = input;
	} 
	_rawtext_doc_count = 0;
}

EnglishDocumentReader::~EnglishDocumentReader() { }

Document* EnglishDocumentReader::readDocument(InputStream &stream) {
	SGMLTag tag;
	LocatedString *source;

	if (_inputType == Symbol(L"rawtext")) {
		source = _new LocatedString(stream, true);
		processRawText(source);
	} else 
		source = _new LocatedString(stream);

	// For Ace2004, some of the annotation files use DOCID instead... sigh...
	// Find the file ID.
	bool docno_tag = false;
	bool docid_tag = false;
	tag = SGMLTag::findOpenTag(source, Symbol(L"DOCNO"), 0);
    if (!tag.notFound()) {
		docno_tag = true;
	}
	if (!docno_tag) {	
		tag = SGMLTag::findOpenTag(source, Symbol(L"DOCID"), 0);
		if (!tag.notFound()) {
			docid_tag = true;
		}
	}

	Symbol name = Symbol(L"0");
	if (docno_tag || docid_tag) {
		int end_open_docno = tag.getEnd();

		if (docno_tag)
			tag = SGMLTag::findCloseTag(source, Symbol(L"DOCNO"), end_open_docno);
		else tag = SGMLTag::findCloseTag(source, Symbol(L"DOCID"), end_open_docno);
		
		if (tag.notFound()) {
			throw UnexpectedInputException("EnglishDocumentReader::readDocument()", 
				"DOCNO or DOCID tag not terminated");
		}
		int start_close_docno = tag.getStart();

		LocatedString *nameString = source->substring(end_open_docno, start_close_docno);
		nameString->trim();
		name = nameString->toSymbol();
		delete nameString;
	}

	// Find the document text.
	tag = SGMLTag::findOpenTag(source, Symbol(L"TEXT"), 0);
	int start_text = tag.getEnd();
	tag = SGMLTag::findCloseTag(source, Symbol(L"TEXT"), tag.getEnd());
	int end_text = tag.getStart();

	source->remove(end_text, source->length());
	source->remove(0, start_text);

	// Divide the text into regions.
	int start_turn = 0;
	int num_regions = 0;
	do {
		tag = SGMLTag::findOpenTag(source, Symbol(L"TURN"), start_turn);
		if (tag.notFound()) {
			_regions[num_regions] = source->substring(start_turn);
			cleanRegion(_regions[num_regions]);
			start_turn = source->length();
		}
		else {
			_regions[num_regions] = source->substring(start_turn, tag.getStart());
			cleanRegion(_regions[num_regions]);
			start_turn = tag.getEnd();
		}
		num_regions++;
	} while (!tag.notFound() && (num_regions < MAX_DOC_REGIONS));

	// TODO: give a warning instead of throwing an exception here:

	// If there were still more tags found then we exceeded the max.
	if (!tag.notFound()) {
		throw UnexpectedInputException("EnglishDocumentReader::readDocument", "too many regions");
	}

	Document *result = _new Document(name, num_regions, _regions);
	for (int i = 0; i < num_regions; i++) {
		delete _regions[i];
	}
	delete source;

	return result;
}

void EnglishDocumentReader::cleanRegion(LocatedString *region) {
	// Remove any annotations.
	SGMLTag openTag;
	Symbol W_SYM = Symbol(L"W");
	Symbol ASR_SYM = Symbol(L"asr");
	do {
		openTag = SGMLTag::findOpenTag(region, Symbol(L"ANNOTATION"), 0);
		if (!openTag.notFound()) {
			SGMLTag closeTag = SGMLTag::findCloseTag(region, Symbol(L"ANNOTATION"), openTag.getEnd());
			if (closeTag.notFound()) {
				region->remove(openTag.getStart(), region->length());
				return;
			}
			// Remove the annotation.
			region->remove(openTag.getStart(), closeTag.getEnd());
		}
	} while (!openTag.notFound());

	// Remove any other tags whatsoever.
	SGMLTag next;
	do {
		next = SGMLTag::findNextSGMLTag(region, 0);
		if (_inputType == ASR_SYM && next.getName() == W_SYM) {
			Symbol startSymbol = next.getAttributeValue(L"Bsec");
			Symbol durSymbol = next.getAttributeValue(L"Dur");

			if (startSymbol.is_null() || durSymbol.is_null()) {
				throw UnexpectedInputException("EnglishDocumentReader::cleanRegion",
					"Could not find Bsec and Dur attribute in sgml tag");
			}
			
			// changed because Intel's compiler doesn't recognize _wtof -JSM
			float start = (float)atof(startSymbol.to_debug_string());
			//float start = (float)_wtof(startSymbol.to_string());
			float dur = (float)atof(durSymbol.to_debug_string());
			//float dur = (float)_wtof(durSymbol.to_string());
			float end = start + dur;

			for (int i = next.getEnd() + 1; i < region->length(); i++) {
				region->setAsrStartAt(i, start);
				region->setAsrEndAt(i, end);
			}
		}
		region->remove(next.getStart(), next.getEnd());
	} while (!next.notFound());

	// Replace recognized SGML entities.
	region->replace(L"&LR;", L"", 0);
	region->replace(L"&UR;", L"", 0);
	region->replace(L"&MD;", L"--", 2);
	region->replace(L"&AMP;", L"&", 1);
}

void EnglishDocumentReader::processRawText(LocatedString *document)
{
	wchar_t num[10];
	_itow(_rawtext_doc_count++, num, 10);
	wchar_t buffer[100] = L"<DOC>\n<DOCNO>RAWTEXT-";
	wcscat(buffer, num);
	wcscat(buffer, L"</DOCNO>\n<TEXT>\n");

	document->replace(L">", L"&gt;");
	document->replace(L"<", L"&lt;");
	document->insert(buffer, 0);
	document->append(L"</TEXT>\n</DOC>");
}


