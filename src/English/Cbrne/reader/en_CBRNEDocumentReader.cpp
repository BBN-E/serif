// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/Cbrne/reader/en_CBRNEDocumentReader.h"
#include "Generic/reader/DefaultDocumentReader.h"
#include "Generic/reader/SGMLTag.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Region.h"
#include "Generic/common/InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/LocatedString.h"
#include <wchar.h>
#include <iostream>
#include <stdio.h>


using namespace std;

class DefaultDocumentReader;

CBRNEDocumentReader::CBRNEDocumentReader() {}
CBRNEDocumentReader::~CBRNEDocumentReader() {}

Document* CBRNEDocumentReader::readDocument(InputStream &stream) {
	_regions.clear();
	SGMLTag tag;

	LocatedString source(stream);


	// Divide the text into regions.
	int start_region = 0;
	int num_regions = 0;

	//cout << "LEN - " << source.length();

	tag = SGMLTag::findOpenTag(source, Symbol(L"DOC"), start_region);
	Symbol name = tag.getAttributeValue(L"DOCID");

	if(name.is_null())
		throw UnexpectedInputException("CBRNEDocumentReader::readDocument", "DOCID attribute not found!");

	//printf("NAME : %S :", name.to_string());
	//printf("NUM : %d %d :", tag.getStart(), tag.getEnd());
	//printf("TAG : %S :", tag.toSymbol().to_string());
	
	Document *result = _new Document(name);

	do {
		tag = SGMLTag::findOpenTag(source, Symbol(L"SERIF_TEXT"), start_region);
		if (tag.notFound()) {
			//do nothing
		}
		else {
			//set region's start as right after the open tag
			start_region = tag.getEnd();

			//find close tag
			SGMLTag closeTag = SGMLTag::findCloseTag(source, Symbol(L"SERIF_TEXT"), start_region);
			int end_region = closeTag.getStart();

			//add a new region comprising what's between; clean it
			LocatedString * region_str = source.substring(start_region, end_region);
			//cout << "\nregion: (" << start_region << ", " << end_region << ")\n";
			cleanRegion(region_str, _inputType);
			_regions.push_back(_new Region(result, tag.getName(), num_regions, region_str));
			num_regions++;
			delete region_str;

			//move pointer to just past this element
			start_region = closeTag.getEnd();
		}
	} while (!tag.notFound());

	// TODO: give a warning instead of throwing an exception here:

	// If there were still more tags found then we exceeded the max.
	if (!tag.notFound()) {
		throw UnexpectedInputException("CBRNEDocumentReader::readDocument", "too many regions");
	}

	//if there were zero regions then there is nothing for us to process
	if (num_regions == 0) {
		throw UnexpectedInputException("CBRNEDocumentReader::readDocument", "No text regions found!");
	}

	// Give ownership of the resulting array of Regions to the Document object
	result->takeRegions(_regions);

	return result;
}


void CBRNEDocumentReader::cleanRegion(LocatedString *region, Symbol inputType) {
	// Remove any annotations.
	SGMLTag openTag;
	do {
		openTag = SGMLTag::findOpenTag(*region, Symbol(L"ANNOTATION"), 0);
		if (!openTag.notFound()) {
			SGMLTag closeTag = SGMLTag::findCloseTag(*region, Symbol(L"ANNOTATION"), openTag.getEnd());
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
		next = SGMLTag::findNextSGMLTag(*region, 0);
		region->remove(next.getStart(), next.getEnd());
	} while (!next.notFound());

	// Replace recognized SGML entities.
	region->replace(L"&LR;", L"");
	region->replace(L"&lr;", L"");

	region->replace(L"&UR;", L"");
	region->replace(L"&ur;", L"");

	region->replace(L"&MD;", L"--");
	region->replace(L"&md;", L"--");

	region->replace(L"&AMP;", L"&");
	region->replace(L"&amp;", L"&");

	region->replace(L"&QUOT;", L"\"");
	region->replace(L"&quot;", L"\"");

	region->replace(L"&APOS;", L"\'");
	region->replace(L"&apos;", L"\'");

	region->replace(L"&LT;", L"<");
	region->replace(L"&lt;", L"<");

	region->replace(L"&GT;", L">");
	region->replace(L"&gt;", L">");
}
