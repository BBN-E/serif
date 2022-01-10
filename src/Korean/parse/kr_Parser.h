// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KR_KOREAN_PARSER_H
#define KR_KOREAN_PARSER_H

#include "common/IDGenerator.h"
#include "common/Symbol.h"
#include "common/DebugStream.h"
#include "parse/KoreanParser.h"
#include "Korean/parse/kr_ParseSeeder.h"

class Parse;
class NameTheory;
class TokenSequence;
class Token;
class Constraint;
class DefaultParser;
class ParseNode;
class SynNode;
class PartOfSpeechSequence;

class KoreanParser : public Parser {
private:
	friend class KoreanParserFactory;

public:
	~KoreanParser();
	virtual void cleanup();

	void resetForNewSentence();
	void resetForNewDocument(DocTheory *docTheory = 0);

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
				  ValueMentionSet *valueMentionSet);

	int getParses(Parse **results, int max_num_parses,
				  TokenSequence *tokenSequence,
				  PartOfSpeechSequence *partOfSpeech,
				  NameTheory *nameTheory,
				  ValueMentionSet *valueMentionSet, 
				  Constraint *constraints,
				  int n_constraints);

	virtual void setMaxParserSeconds(int maxsecs);

private:
	IDGenerator _nodeIDGenerator;

	Symbol* _sentence;
	Constraint* _constraints;
	int _max_constraints;
	int _max_sentence_length;
	DefaultParser* _standardParser;

	DebugStream _debug;
	static const int DBG = 1;

	KoreanParseSeeder::SameSpanParseToken _lookupChart[MAX_SENTENCE_TOKENS][MAX_POSSIBILITIES_PER_WORD];
	KoreanParseSeeder::SameSpanParseToken* _parserSeedingChart[MAX_SENTENCE_TOKENS*3];
	int _lookupCount[MAX_SENTENCE_TOKENS];
	KoreanParseSeeder* _ps;

	KoreanParser();
	int _putTokenSequenceInLookupChart(const TokenSequence* ts, const NameTheory* nt);

	ParseNode *getDefaultFlatParse(const TokenSequence *tokenSequence,
								   const PartOfSpeechSequence *partOfSpeech);
	ParseNode *getDefaultFlatParse(const TokenSequence *tokenSequence);

	SynNode *convertParseToSynNode(ParseNode* parse, 
		SynNode* parent, const NameTheory *nameTheory, 
		const TokenSequence *tokenSequence,
		int start_token);

	SynNode *convertParseToSynNode(ParseNode* parse, SynNode* parent, 
								   int *old_token,
								   const TokenSequence *tokenSequence,
								   int *new_token, Token **newTokens,
								   int *offset,
								   int *token_mapping);
	
};

class KoreanParserFactory: public Parser::Factory {
	virtual Parser *build() { return _new KoreanParser(); }
};


#endif
