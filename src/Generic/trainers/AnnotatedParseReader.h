// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ANNOTATEDPARSEREADER_H
#define ANNOTATEDPARSEREADER_H

#include "Generic/common/SexpReader.h"
#include "Generic/theories/Parse.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/trainers/CorefDocument.h"
#include "Generic/trainers/CorefItem.h"
#include "Generic/trainers/HeadFinder.h"
#include "Generic/theories/Mention.h"


class AnnotatedParseReader : public SexpReader {
public:
	AnnotatedParseReader();
	AnnotatedParseReader(const char *file_name);
	void readAllDocuments(GrowableArray <CorefDocument *> & results);

	//virtual UTF8Token getToken(int specifier);

private:
	// Note: in Chinese, the HeadFinder constructor reads in a head table, so I
	// made this a class member to avoid repeated construction and 
	// destruction of the table
	HeadFinder* _headFinder;

	CorefDocument * readOptionalDocument();
	//tries to extract mention information, or returns NULL if there is no mention information
	//also populates tag with the non-augmented tag
	Mention *processAugmentedTag(const wchar_t *augmented_tag, Symbol &tag);

	// tries to read coref information, or returns CorefItem::NO_ID if there is no coref information
	int readOptionalID();

	Parse *readOptionalParse(GrowableArray <CorefItem *> & corefItemResults);
	SynNode *readOptionalNode(GrowableArray <CorefItem *> & corefItemResults);
	SynNode *readOptionalNodeWithTokens(GrowableArray <CorefItem *> & corefItemResults, int& tok_num);

};

#endif
