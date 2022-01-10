// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef FLAT_PARSER_H
#define FLAT_PARSER_H

#include "Generic/parse/ParserBase.h"
#include "Generic/common/IDGenerator.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/DebugStream.h"

class Parse;
class NameTheory;
class NestedNameTheory;
class ValueMentionSet;
class DocTheory;
class TokenSequence;
class Constraint;
class ChartDecoder;
class ParseNode;
class SynNode;
class PartOfSpeechSequence;

class FlatParser: public ParserBase {
public:
	FlatParser();
	~FlatParser();

	void resetForNewDocument(DocTheory *docTheory = 0);

	// This does the work. It fills in the array of pointers to Parses
	// where specified by results arg, and returns its size. The maximum
	// number of desired parses is specified by max_num_parses. It returns
	// 0 if something goes wrong. The client is responsible both for 
	// allocating and deleting the array of parses; the client is also
	// responsible for deleting the Parses themselves.
	int getParses(Parse **results, int max_num_parses,
			      const TokenSequence *tokenSequence,
				  const PartOfSpeechSequence *partOfSpeech,
				  const NameTheory *nameTheory,
				  const NestedNameTheory *nestedNameTheory,
				  const ValueMentionSet *valueMentionSet);
	
	int getParses(Parse **results, int max_num_parses,
					  const TokenSequence *tokenSequence,
					  const PartOfSpeechSequence *partOfSpeech,
					  const NameTheory *nameTheory,
					  const NestedNameTheory *nestedNameTheory,
					  const ValueMentionSet *valueMentionSet,
					  Constraint *constraints,
					  int n_constraints);

private:
	ParseNode *getDefaultFlatParse(const TokenSequence *tokenSequence,
								   const PartOfSpeechSequence *partOfSpeech);

	DebugStream _debug;
};

#endif

