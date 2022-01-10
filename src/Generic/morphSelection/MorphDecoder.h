// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MORPH_DECODER_H
#define MORPH_DECODER_H

#include <boost/shared_ptr.hpp>
#include <vector>

#include "Generic/morphSelection/MorphDecoder.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/limits.h"
#include "Generic/common/Offset.h"
#include "Generic/common/UnrecoverableException.h"

class MorphModel;
class TokenSequence;
class ParseSeeder;
class LocatedString;

#define MAX_LETTERS_PER_WORD 100
#define MAX_POSSIBILITIES 50

class MorphDecoder  {
public:
	/** Create and return a new MorphDecoder. */
	static MorphDecoder *build() { return _factory()->build(); }
	/** Hook for registering new MorphDecoder factories */
	struct Factory { virtual MorphDecoder *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

protected:
	/** A morphological subcomponent of a token (aka a morpheme) */
	struct Chunk {
		Symbol symbol;
		OffsetGroup start;
		OffsetGroup end;
		Chunk(const Symbol &symbol, const OffsetGroup &start, const OffsetGroup &end)
			: symbol(symbol), start(start), end(end) {}
	};

	/** A morphological decomposition of a single token. */
	typedef std::vector<Chunk> TokenChunking;

	/** A set of possible morphological decompositions for a single
	 * token. */
	struct TokenChunkPossibilities {
		int tok_num;
		std::vector<TokenChunking> possibilities;
	};

	TokenChunkPossibilities _atomized_sentence[MAX_SENTENCE_TOKENS];
	int _num_tokens; // length of _atomized_sequence.

	MorphModel * model;
	ParseSeeder * ps;

	// core functions
	int walkForwardThroughSentence();
	virtual void putTokenSequenceInAtomizedSentence(const LocatedString &sentenceString, TokenSequence* ts) = 0;
	virtual void putTokenSequenceInAtomizedSentence(const LocatedString &sentenceString, TokenSequence *ts, 
									CharOffset *constraints, int n_constraints) = 0;

	Symbol makeWordFromChunks(Symbol* chunks, int num);
	void printTrellis();

private:
	typedef struct {
		std::vector<Symbol> word_structure;
		std::vector<Symbol> words_used; //put word features here
		int prev_poss_index;
		double alphaProb;
		double scoreParts[4];
	} MorphChartEntry;

	int _num_words[MAX_SENTENCE_TOKENS]; // length of _words[i]
	MorphChartEntry _words[MAX_SENTENCE_TOKENS][MAX_POSSIBILITIES];

public:	

	virtual ~MorphDecoder();

	int getBestWordSequence(const LocatedString &sentenceString, TokenSequence*ts, Symbol* word_sequence, int max_words);
	int getBestWordSequence(const LocatedString &sentenceString, TokenSequence*ts, Symbol* word_sequence, int* map, int max_words);
	int getBestWordSequence(const LocatedString &sentenceString, TokenSequence*ts, Symbol* word_sequence, int* map, 
		                    OffsetGroup* start, OffsetGroup* end, int max_words);
	int getBestWordSequence(const LocatedString &sentenceString, TokenSequence*ts, Symbol* word_sequence, int* map, 
		                    OffsetGroup* start, OffsetGroup* end, CharOffset* constraints, int n_constraints, int max_words);

	void printTrellis(UTF8OutputStream &out);

protected:
	MorphDecoder();

private:
	static boost::shared_ptr<Factory> &_factory();
};

//#if defined(ARABIC_LANGUAGE)
//	#include "Arabic/morphSelection/MorphDecoder.h"
//#elif defined(KOREAN_LANGUAGE)
//	#include "Korean/morphology/kr_MorphDecoder.h"
//#else
//	#include "Generic/morphSelection/xx_MorphDecoder.h"
//#endif

#endif
