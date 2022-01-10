// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DEFAULT_PARSER_H
#define DEFAULT_PARSER_H

#include "Generic/parse/ParserBase.h"
#include "Generic/common/IDGenerator.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/DebugStream.h"
#include "Generic/parse/Constraint.h"
#include <vector>

class Parse;
class NameTheory;
class NestedNameTheory;
class ValueMentionSet;
class DocTheory;
class TokenSequence;
class ChartDecoder;
class ParseNode;
class SynNode;
class PartOfSpeechSequence;
class PartOfSpeechTable;


class DefaultParser: public ParserBase {
public:
	DefaultParser();
	virtual ~DefaultParser();

	void resetForNewDocument(DocTheory *docTheory = 0);
	void setToLowerCase() { if (_lowerCaseDecoder != 0) _decoder = _lowerCaseDecoder; }
	void setToMixedCase() { _decoder = _defaultDecoder; }

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

	ChartDecoder *getDecoder() { return _decoder; }

	void writeCaches();
	void cleanup();

private:
	bool use_hyphen_constraints;
	bool skip_unimportant_sentences;
	int unimportant_token_limit;

	Symbol* _sentence;
	std::vector<Constraint> _constraints;
	ChartDecoder *_decoder;
	ChartDecoder *_lowerCaseDecoder;
	ChartDecoder *_upperCaseDecoder;
	ChartDecoder *_defaultDecoder;
	PartOfSpeechTable* _auxPosTable; 

	DebugStream _debug;
};

#endif

