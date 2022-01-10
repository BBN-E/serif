// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PARSER_H
#define PARSER_H

#include <boost/shared_ptr.hpp>


class Parse;
class NameTheory;
class NestedNameTheory;
class DocTheory;
class TokenSequence;
class ValueMentionSet;
class Constraint;
class PartOfSpeechSequence;


class Parser {
public:
	/** Create and return a new Parser. */
	static Parser *build() { return _factory()->build(); }
	/** Hook for registering new Parser factories */
	struct Factory { virtual Parser *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

public:
	virtual ~Parser() {}

	virtual void resetForNewSentence() = 0;
	virtual void resetForNewDocument(DocTheory *docTheory = 0) = 0;

	// This does the work. It fills in the array of pointers to Parses
	// where specified by results arg, and returns its size. The maximum
	// number of desired parses is specified by max_num_parses. It returns
	// 0 if something goes wrong. The client is responsible both for 
	// allocating and deleting the array of parses; the client is also
	// responsible for deleting the Parses themselves.
	virtual int getParses(Parse **results, int max_num_parses,
						  TokenSequence *tokenSequence,
						  PartOfSpeechSequence *partOfSpeech,
						  NameTheory *nameTheory,
						  NestedNameTheory *nestedNameTheory,
						  ValueMentionSet *valueMentionSet) = 0;
	virtual int getParses(Parse **results, int max_num_parses,
						  TokenSequence *tokenSequence,
						  PartOfSpeechSequence *partOfSpeech,
						  NameTheory *nameTheory,
						  NestedNameTheory *nestedNameTheory,
						  ValueMentionSet *valueMentionSet,
						  Constraint *constraints, int n_constraints) { return 0; }

    virtual void writeCaches() {}
    virtual void cleanup() = 0;
	virtual void setMaxParserSeconds(int maxsecs) = 0;

protected:
	Parser() {}

private:
	static boost::shared_ptr<Factory> &_factory();
};

//#if defined(ARABIC_LANGUAGE)
//	#include "Arabic/parse/ar_Parser.h"
//#elif defined(KOREAN_LANGUAGE)
//	#include "Korean/parse/kr_Parser.h"
//#else
//	#include "Generic/parse/xx_Parser.h"
//#endif

#endif

