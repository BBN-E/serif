// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PRONOUNLINKER_H
#define PRONOUNLINKER_H

#include <boost/shared_ptr.hpp>

#include "Generic/edt/LinkGuess.h"
#include "Generic/edt/MentionLinker.h"
#include "Generic/common/DebugStream.h"
class PartOfSpeechSequence;

class PronounLinker : public MentionLinker {
public:
	/** Create and return a new PronounLinker. */
	static PronounLinker *build() { return _factory()->build(); }
	/** Hook for registering new PronounLinker factories */
	struct Factory { virtual PronounLinker *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~PronounLinker() {}
	virtual int linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, EntityType linkType, LexEntitySet *results[], int max_results) = 0;
	virtual void addPreviousParse(const Parse *parse) = 0;
	//the following 2 functions are only implemented in Arabic (where POS includes gender info)
	virtual void addPartOfSpeechSequence(const PartOfSpeechSequence* posSeq){
		;};
	virtual void resetPartOfSpeechSequence() {;};

	virtual void resetPreviousParses() = 0;
	//the following two functions are there to support a temporary hack, and will go away soon
/*
virtual int getLinkGuesses(EntitySet *currSolution, Mention *currMention, LinkGuess results[], int max_results) = 0;
	static DebugStream &getDebugStream () {static DebugStream stream; return stream; }
*/
	static DebugStream &getDebugStream () { return _debugStream; }

protected:
	PronounLinker();
	static DebugStream _debugStream;

private:
	static boost::shared_ptr<Factory> &_factory();
};

//#if defined(ENGLISH_LANGUAGE)
//	#include "English/edt/en_PronounLinker.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/edt/ch_PronounLinker.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/edt/ar_PronounLinker.h"
//#else
//	#include "Generic/edt/xx_PronounLinker.h"
//#endif

#endif
