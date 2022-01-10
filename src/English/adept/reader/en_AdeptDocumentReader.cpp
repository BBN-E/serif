// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/adept/reader/en_AdeptDocumentReader.h"
#include "Generic/reader/SGMLTag.h"
#include "Generic/theories/Document.h"
#include "Generic/common/InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/Symbol.h"
#include <wchar.h>
#include <iostream>
#include <stdio.h>


using namespace std;

//LocatedString* EnglishDocumentReader::_regions[MAX_DOC_REGIONS];


AdeptDocumentReader::AdeptDocumentReader() {}
AdeptDocumentReader::~AdeptDocumentReader() {}

Document* AdeptDocumentReader::readDocument(InputStream &stream) {
	SGMLTag tag;

	LocatedString source(stream);


	// Divide the text into regions.
	int start_region = 0;
	int num_regions = 0;

	//cout << "LEN - " << source.length();

	tag = SGMLTag::findOpenTag(source, Symbol(L"DOC"), start_region);
	Symbol name = tag.getAttributeValue(L"ADEPTID");

	if(name.is_null())
		throw UnexpectedInputException("AdeptDocumentReader::readDocument", "ADEPTID attribute not found!");

	//printf("NAME : %S :", name.to_string());
	//printf("NUM : %d %d :", tag.getStart(), tag.getEnd());
	//printf("TAG : %S :", tag.toSymbol().to_string());
	

	do {
		tag = SGMLTag::findOpenTag(source, Symbol(L"ADEPT_TEXT"), start_region);
		if (tag.notFound()) {
			//do nothing
		}
		else {
			//set region's start as right after the open tag
			start_region = tag.getEnd();

			//find close tag
			SGMLTag closeTag = SGMLTag::findCloseTag(source, Symbol(L"ADEPT_TEXT"), start_region);
			int end_region = closeTag.getStart();

			//add a new region comprising what's between; clean it
			_regions[num_regions] = source.substring(start_region, end_region);
			//cout << "\nregion: (" << start_region << ", " << end_region << ")\n";
			cleanRegion(_regions[num_regions]);

			//move pointer to just past this element
			start_region = closeTag.getEnd();
			num_regions++;
		}
	} while (!tag.notFound() && (num_regions < MAX_DOC_REGIONS));

	// TODO: give a warning instead of throwing an exception here:

	// If there were still more tags found then we exceeded the max.
	if (!tag.notFound()) {
		throw UnexpectedInputException("AdeptDocumentReader::readDocument", "too many regions");
	}

	//if there were zero regions then there is nothing for us to process
	if (num_regions == 0) {
		throw UnexpectedInputException("AdeptDocumentReader::readDocument", "No text regions found!");
	}

	Document *result = _new Document(name, num_regions, _regions);
	for (int i = 0; i < num_regions; i++) {
		delete _regions[i];
	}
	return result;
}


void AdeptDocumentReader::cleanRegion(LocatedString *region) {
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
	region->replace(L"&LR;", L"", 0);
	region->replace(L"&lr;", L"", 0);

	region->replace(L"&UR;", L"", 0);
	region->replace(L"&ur;", L"", 0);

	region->replace(L"&MD;", L"--", 2);
	region->replace(L"&md;", L"--", 2);

	region->replace(L"&AMP;", L"&", 1);
	region->replace(L"&amp;", L"&", 1);

	region->replace(L"&QUOT;", L"\"", 1);
	region->replace(L"&quot;", L"\"", 1);

	region->replace(L"&APOS;", L"\'", 1);
	region->replace(L"&apos;", L"\'", 1);

	region->replace(L"&LT;", L"<", 1);
	region->replace(L"&lt;", L"<", 1);

	region->replace(L"&GT;", L">", 1);
	region->replace(L"&gt;", L">", 1);
}
