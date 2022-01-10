#ifndef XX_PARSER_H
#define XX_PARSER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#include "Generic/parse/Parser.h"
#include "Generic/common/ParamReader.h"

#include "Generic/parse/ChartDecoder.h"
#include "Generic/parse/DefaultParser.h"
#include "Generic/parse/FlatParser.h"

class GenericParser : public Parser {
private:
	friend class GenericParserFactory;

public:
	~GenericParser() {
		delete _parser;
	}

	virtual void resetForNewSentence() {
		_parser->resetForNewSentence();
	}
	
	virtual void resetForNewDocument(DocTheory *docTheory = 0) {
		_parser->resetForNewDocument(docTheory);
	}

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
						  ValueMentionSet *valueMentionSet)
	{
		return _parser->getParses(results, max_num_parses,
								 tokenSequence, partOfSpeech,
								 nameTheory, nestedNameTheory,
								 valueMentionSet);
	}

	virtual int getParses(Parse **results, int max_num_parses,
						  TokenSequence *tokenSequence,
						  PartOfSpeechSequence *partOfSpeech,
						  NameTheory *nameTheory,
						  NestedNameTheory *nestedNameTheory,
						  ValueMentionSet *valueMentionSet,
						  Constraint *constraints, int n_constraints)
	{
		return _parser->getParses(results, max_num_parses,
								 tokenSequence, partOfSpeech, 
								 nameTheory, nestedNameTheory,
								 valueMentionSet,
								 constraints, n_constraints);
	}

	virtual void setMaxParserSeconds(int maxsecs) {
		if (!use_flat_parser)
			static_cast<DefaultParser*>(_parser)->getDecoder()->setMaxParserSeconds(maxsecs);
	}

        virtual void writeCaches() {
          _parser->writeCaches();
        }
		virtual void cleanup() {
			_parser->cleanup();
		}


private:
	ParserBase *_parser;
	bool use_flat_parser;

	GenericParser(): _parser(0) {
		use_flat_parser = ParamReader::isParamTrue("use_flat_parser");
		if (use_flat_parser) {
			_parser = _new FlatParser();
		} else {
			_parser = _new DefaultParser();
		}
	}

};

class GenericParserFactory: public Parser::Factory {
	virtual Parser *build() { return _new GenericParser(); }
};



#endif
