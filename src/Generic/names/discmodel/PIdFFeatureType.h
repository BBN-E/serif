// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PIDF_FEATURE_TYPE_H
#define PIDF_FEATURE_TYPE_H
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/common/Symbol.h"
#include <map>

class PIdFFeatureType : public DTFeatureType {
public:
	static Symbol modeltype;

	//These should only get called at most once each, and only if
	//the plan is to limit lexical features to in-vocab words.
	static void setUnigramVocab(const std::string vocabFile);
	static void setBigramVocab(const std::string bigramFile);

	//Should return true if their respective sets are empty
	//(i.e. all words are "in-vocab" if we aren't limiting vocab)
	static bool isVocabWord(const Symbol word);
	static bool isVocabBigram(const Symbol word1, const Symbol word2);

	PIdFFeatureType(Symbol name, InfoSource infoSource=InfoSource::OBSERVATION | InfoSource::PREV_TAG) : DTFeatureType(modeltype, name, infoSource) {}
private:
	static std::set<Symbol> _unigramVocab;
	static std::map<Symbol,std::set<Symbol> > _bigramVocab;
	static const int VOCAB_MIN_FREQUENCY = 1;
};

#endif
