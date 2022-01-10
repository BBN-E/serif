// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef NPCHUNK_RECOGNIZER_H
#define NPCHUNK_RECOGNIZER_H

#include <boost/shared_ptr.hpp>

class NPChunkTheory;
class TokenSequence;
class Parse;
class NameTheory;
class PartOfSpeechSequence;

class NPChunkFinder {
public:
	/** Create and return a new NPChunkFinder. */
	static NPChunkFinder *build() { return _factory()->build(); }
	/** Hook for registering new NPChunkFinder factories */
	struct Factory { virtual NPChunkFinder *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~NPChunkFinder() {};
	
	virtual void resetForNewSentence() = 0;
	virtual void resetForNewDocument(class DocTheory *docTheory = 0) = 0;


	// This does the work. It populates an array of pointers to NPChunkheorys
	// specified by results arg with up to max_theories NPChunkTheory pointers,
	// and returns the number of theories actually created, or 0 if
	// something goes wrong. The client is responsible for deleting the
	// NameTheorys.
	//we're cheating for now and using the parser to produce pos labels
    virtual int getNPChunkTheories(NPChunkTheory **results, 
                                       const int max_theories,								   
                                       const TokenSequence *tokenSequence, 
                                       const Parse *parse, 
                                       const NameTheory* nt) = 0;
    virtual int getNPChunkTheories(NPChunkTheory **results, 
                                       const int max_theories, 
                                       const TokenSequence* tokenSequence, 
                                       const PartOfSpeechSequence* poss, 
                                       const NameTheory* nt) = 0;

	virtual void cleanUpAfterDocument() = 0;
protected:
	NPChunkFinder() {};
private:
	static boost::shared_ptr<Factory> &_factory();
};

////#if defined(ENGLISH_LANGUAGE)
////	#include "English/npChunking/en_NPChunkFinder.h"
////#else
//#include "Generic/PNPChunking/xx_NPChunkFinder.h"
////#endif

#endif
