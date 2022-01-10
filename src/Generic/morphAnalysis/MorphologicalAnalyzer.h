// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MORPHOLOGICALANALYZER_H
#define MORPHOLOGICALANALYZER_H

#include <boost/shared_ptr.hpp>

class TokenSequence;

class MorphologicalAnalyzer {
public:
	/** Create and return a new MorphologicalAnalyzer. */
	static MorphologicalAnalyzer *build() { return _factory()->build(); }
	/** Hook for registering new MorphologicalAnalyzer factories */
	struct Factory { virtual MorphologicalAnalyzer *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~MorphologicalAnalyzer(){};
	virtual void resetForNewSentence() =0;//you could set the dictionaries here
	virtual int getMorphTheories(TokenSequence *origTS) =0;
	virtual int getMorphTheoriesDBG(TokenSequence *origTS) =0;
	virtual void readNewVocab(const char* dict_file) =0;	//if the dictinary changes, use this for serialization
	virtual void resetDictionary() =0;

protected:
	MorphologicalAnalyzer(){};

private:
	static boost::shared_ptr<Factory> &_factory();
};
//#if defined(ARABIC_LANGUAGE)
//	#include "Arabic/BuckWalter/ar_MorphologicalAnalyzer.h"
//#elif defined(KOREAN_LANGUAGE)
//	#include "Korean/morphology/kr_MorphologicalAnalyzer.h"
//#else
//	#include "Generic/morphAnalysis/xx_MorphologicalAnalyzer.h"
//#endif
#endif
