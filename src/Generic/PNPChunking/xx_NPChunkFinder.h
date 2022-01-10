#ifndef XX_NPCHUNK_FINDER_H
#define XX_NPCHUNK_FINDER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/PNPChunking/DefaultNPChunkFinder.h"

class DocTheory;
class NPChunkTheory;
class TokenSequence;
class Parse;
class PartOfSpeechSequence;
class NameTheory;
class PNPChunkSentence;

class GenericNPChunkFinder: public NPChunkFinder {
private:
	friend class GenericNPChunkFinderFactory;

public:
	~GenericNPChunkFinder() { delete _decoder; };
	
	virtual void resetForNewSentence() { _decoder->resetForNewSentence(); };
	virtual void resetForNewDocument(class DocTheory *docTheory = 0) { 
		_decoder->resetForNewDocument(docTheory); 
	};

	// This does the work. It populates an array of pointers to NPChunkheorys
	// specified by results arg with up to max_theories NPChunkTheory pointers,
	// and returns the number of theories actually created, or 0 if
	// something goes wrong. The client is responsible for deleting the
	// NameTheorys.
	//we're cheating for now and using the parser to produce pos labels
	virtual int getNPChunkTheories(	NPChunkTheory **results,
									const int max_theories,
									const TokenSequence *tokenSequence,
									const Parse *parse,
									const NameTheory* nt)
	{
		return _decoder->getNPChunkTheories(results, max_theories, tokenSequence, parse, nt);
	};
	virtual int getNPChunkTheories(NPChunkTheory **results,
	                               const int max_theories,
	                               const TokenSequence *tokenSequence,
	                               const PartOfSpeechSequence *poss,
	                               const NameTheory* nt) 
	{
		return _decoder->getNPChunkTheories(results, max_theories, tokenSequence, poss, nt);
	};
	
	virtual void cleanUpAfterDocument() {  _decoder->cleanUpAfterDocument(); };

private:
	GenericNPChunkFinder() { _decoder = _new DefaultNPChunkFinder(); };
	DefaultNPChunkFinder* _decoder;
};

class GenericNPChunkFinderFactory: public NPChunkFinder::Factory {
	virtual NPChunkFinder *build() { return _new GenericNPChunkFinder(); }
};



#endif
