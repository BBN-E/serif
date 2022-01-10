// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DEFAULT_NP_CHUNK_FINDER_H
#define DEFAULT_NP_CHUNK_FINDER_H

#include "Generic/PNPChunking/PNPChunkDecoder.h"

class DocTheory;
class NPChunkTheory;
class TokenSequence;
class Parse;
class PartOfSpeechSequence;
class NameTheory;
class PNPChunkSentence;

class DefaultNPChunkFinder {
public:
	DefaultNPChunkFinder();
	~DefaultNPChunkFinder() { delete _decoder;} ;
	
	virtual void resetForNewSentence(){};
	virtual void resetForNewDocument(class DocTheory *docTheory = 0){};

	// This does the work. It populates an array of pointers to NPChunkTheorys
	// specified by results arg with up to max_theories NPChunkTheory pointers,
	// and returns the number of theories actually created, or 0 if
	// something goes wrong. The client is responsible for deleting the
	// NameTheorys.
	//we're cheating for now and using the parser to produce pos labels
	virtual int getNPChunkTheories(NPChunkTheory **results,
	                               const int max_theories,
	                               const TokenSequence *tokenSequence,
	                               const Parse *parse,
	                               const NameTheory* nt);

	virtual int getNPChunkTheories(NPChunkTheory **results,
	                               const int max_theories,
	                               const TokenSequence *tokenSequence,
	                               const PartOfSpeechSequence *poss,
	                               const NameTheory* nt);

	virtual void cleanUpAfterDocument(){};

private:
	PNPChunkDecoder* _decoder;

	void fillNPChunkTheory(const int nwords, const PNPChunkSentence* sentence,
						   NPChunkTheory* chunkTheory);
	void fillNPChunkTheory(const int nwords, PNPChunkSentence* sentence,
						   NPChunkTheory* chunkTheory, const NameTheory* nt);
	SynNode* getSynNode(const TokenSequence* ts, const Symbol* pos, const NPChunkTheory *chunkTheory, const NameTheory* nt);
};

#endif
