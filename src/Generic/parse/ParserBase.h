// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PARSER_BASE_H
#define PARSER_BASE_H


#include "Generic/common/IDGenerator.h"
#include "Generic/common/Symbol.h"

class Parse;
class NameTheory;
class NestedNameTheory;
class DocTheory;
class TokenSequence;
class ValueMentionSet;
class Constraint;
class PartOfSpeechSequence;
class SynNode;
class ParseNode;

class ParserBase {
public:
	ParserBase() {}
	virtual ~ParserBase() {}

	virtual void resetForNewSentence();
	virtual void resetForNewDocument(DocTheory *docTheory = 0) = 0;

	// This does the work. It fills in the array of pointers to Parses
	// where specified by results arg, and returns its size. The maximum
	// number of desired parses is specified by max_num_parses. It returns
	// 0 if something goes wrong. The client is responsible both for 
	// allocating and deleting the array of parses; the client is also
	// responsible for deleting the Parses themselves.
	virtual int getParses(Parse **results, int max_num_parses,
						  const TokenSequence *tokenSequence,
						  const PartOfSpeechSequence *partOfSpeech,
						  const NameTheory *nameTheory,
						  const NestedNameTheory *nestedNameTheory,
						  const ValueMentionSet *valueMentionSet) = 0;
	virtual int getParses(Parse **results, int max_num_parses,
						  const TokenSequence *tokenSequence,
						  const PartOfSpeechSequence *partOfSpeech,
						  const NameTheory *nameTheory,
						  const NestedNameTheory *nestedNameTheory,
						  const ValueMentionSet *valueMentionSet,
						  Constraint *constraints, int n_constraints) { return 0; }

	virtual void writeCaches() {}
	virtual void cleanup() {}

protected:
	
	IDGenerator _nodeIDGenerator;

	virtual SynNode *convertParseToSynNode(ParseNode* parse, 
							SynNode* parent, const NameTheory *nameTheory, 
							const NestedNameTheory *nestedNameTheory,
							const TokenSequence *tokenSequence,
							int start_token);

	virtual SynNode *convertListParseToSynNode(ParseNode* parse, 
							SynNode* parent, const NameTheory *nameTheory, 
							const NestedNameTheory *nestedNameTheory,
							const TokenSequence *tokenSequence);

	virtual SynNode *convertFlatParseWithNamesToSynNode(SynNode* parent, 
														const NameTheory *nameTheory,
														const NestedNameTheory *nestedNameTheory,
														const TokenSequence *tokenSequence,
														int start_token, int end_token,
														Symbol tag, Symbol headTag); 
};

#endif

