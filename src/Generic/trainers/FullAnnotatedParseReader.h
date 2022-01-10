// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef FULL_ANNOTATEDPARSEREADER_H
#define FULL_ANNOTATEDPARSEREADER_H

#include "Generic/common/SexpReader.h"
#include "Generic/theories/Parse.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/trainers/CorefDocument.h"
#include "Generic/trainers/CorefItem.h"
#include "Generic/trainers/AnnotatedParseReader.h"
#include "Generic/trainers/HeadFinder.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/EntityType.h"


class FullAnnotatedParseReader : public SexpReader {
public:
	FullAnnotatedParseReader();
	FullAnnotatedParseReader(const char *file_name);
	void readAllParses(GrowableArray <Parse *> & results);


	//virtual UTF8Token getToken(int specifier);

private:
	// Note: in Chinese, the HeadFinder constructor reads in a head table, so I
	// made this a class member to avoid repeated construction and 
	// destruction of the table
	HeadFinder* _headFinder;


	CorefDocument * readOptionalDocument();

	SynNode *readOptionalNode();
	SynNode* readOptionalNodeWithTokens(int& tok_num);
	//tries to extract mention information, or returns NULL if there is no mention information
	//also populates tag with the non-augmented tag
	Mention *processAugmentedTag(const wchar_t *augmented_tag, Symbol &tag);

	// tries to read coref information, or returns CorefItem::NO_ID if there is no coref information
	int readOptionalID();

	Parse *readOptionalParse();

};

#endif
