// Copyright 2008-2010 by BBN Technologies Corp.
// All Rights Reserved.

/* ---------------
 * ParseNameReader
 * ---------------
 *
 * This class is for loading Parse and Name theories from a combined
 *  s-expression format. The file format this reads is:
 *
 *    (TOP (S (NPA (DT The) (NPP::Name_GPE (JJ Russian)) (NN airline) (NPP::Name_ORG (NNP Aeroflot))) ... ))
 *    (TOP (S (NP (NPA (NPP::Name_ORG (NNP Aeroflot)) ... ))))
 *
 * There is one Parse and one NameTheory per sentence.  If there are
 * no names in a sentence then the appropriate NameTheory is empty.
 * So parses[i] and names[i] go together.
 *
 */
#ifndef PARSE_NAME_READER_H
#define PARSE_NAME_READER_H

#include "Generic/theories/Parse.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/NameSpan.h"
#include "Generic/common/GrowableArray.h"

class ParseNameReader {
public:
   static void readAllParses(const char *file_name,
							 GrowableArray <Parse *> & parses,
							 GrowableArray <NameTheory *> & names,
							 GrowableArray <TokenSequence *> & tokenseqs);
private:
   static Symbol toLower(Symbol s);
   static SynNode* tagsToNameSpans(const SynNode* parseNode,
								   SynNode* parent,
								   GrowableArray <NameSpan *> & nameSpans,
								   Token *token_arr[]);
};

#endif
