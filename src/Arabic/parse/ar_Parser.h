#ifndef AR_PARSER_H
#define AR_PARSER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/IDGenerator.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/DebugStream.h"
#include "Generic/parse/DefaultParser.h"
#include "Generic/parse/Parser.h"
#include "Arabic/BuckWalter/ar_ParseSeeder.h"
#include "Generic/theories/DocTheory.h"
class Parse;
class NameTheory;
class NestedNameTheory;
class TokenSequence;
class Token;
class Constraint;
class BWArabicChartDecoder;
class ParseNode;
class SynNode;
class PartOfSpeechSequence;


class ArabicParser : public Parser {
private:
	friend class ArabicParserFactory;

public:

	void resetForNewSentence();
	void resetForNewDocument(DocTheory *docTheory = 0) { _docTheory = docTheory; }
	virtual void cleanup() {
		_standardParser->cleanup();
	}

	// This does the work. It fills in the array of pointers to Parses
	// where specified by results arg, and returns its size. The maximum
	// number of desired parses is specified by max_num_parses. It returns
	// 0 if something goes wrong. The client is responsible both for 
	// allocating and deleting the array of parses; the client is also
	// responsible for deleting the Parses themselves.
	int getParses(Parse **results, int max_num_parses,
			      TokenSequence *tokenSequence,
				  PartOfSpeechSequence *partOfSpeech,
				  NameTheory *nameTheory,
				  NestedNameTheory *nestedNameTheory,
				  ValueMentionSet *valueMentionSet);
	int getParses(Parse **results, int max_num_parses,
					  TokenSequence *tokenSequence,
					  PartOfSpeechSequence *partOfSpeech,
					  NameTheory *nameTheory,
					  NestedNameTheory *nestedNameTheory,
					  ValueMentionSet *valueMentionSet, 
					  Constraint *constraints,
					  int n_constraints);
	virtual void setMaxParserSeconds(int maxsecs);

        virtual void writeCaches() {
          _standardParser->writeCaches();
        }


private:
	ArabicParser();
	IDGenerator _nodeIDGenerator;

	Symbol* _sentence;
	int _max_sentence_length;
	std::vector<Constraint> _constraints;
	BWArabicChartDecoder *_decoder;
	DefaultParser* _standardParser;	//hack to avoid clitic stuff when using the
									//annotatedParsePreProcessor
	DebugStream _debug;
	static const int DBG = 0;

	DocTheory *_docTheory;

	SynNode *convertParseToSynNode(ParseNode* parse, SynNode* parent, 
								   int *old_token,
								   const TokenSequence *tokenSequence,
								   int *new_token, Token **newTokens,
								   CharOffset *offset,
								   int *token_mapping);
	void fixNameTheory(NameTheory *nameTheory, int *token_mapping);
	

//for buckwalter
	ArabicParseSeeder::SameSpanParseToken _lookupChart[MAX_SENTENCE_TOKENS][MAX_POSSIBILITIES_PER_WORD];
	ArabicParseSeeder::SameSpanParseToken* _parserSeedingChart[MAX_SENTENCE_TOKENS*3];
	int _lookupCount[MAX_SENTENCE_TOKENS];
	ParseSeeder* _ps;
	int _putTokenSequenceInLookupChart(const TokenSequence* ts, const NameTheory* nt);
	SynNode* convertParseToSynNode(ParseNode* parse, 
									   SynNode* parent, 
									   int *old_token,
									   const TokenSequence *tokenSequence,
									   int *new_token,
									   Token **newTokens,
									   CharOffset *offset,
									   int *token_mapping, int* start_place);



};

class ArabicParserFactory: public Parser::Factory {
	virtual Parser *build() { return _new ArabicParser(); }
};




#endif
