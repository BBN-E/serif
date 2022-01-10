// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ___SFOREST_UTILITIES_H___
#define ___SFOREST_UTILITIES_H___

#include "Generic/SPropTrees/SPropTreeInfo.h"
#include "Generic/SPropTrees/SNodeContent.h"

class SForestUtilities {
public:
	static void initializeMemoryPools();
	static void clearMemoryPools();
	static size_t populatePropForest(const DocTheory* dt, const SentenceTheory* st, const Proposition* prop, SPropForest& forest, SNodeContent::Language lang);
	static size_t populatePropForest(const DocTheory* dt, const SentenceTheory* st, const Mention* men, SPropForest& forest, SNodeContent::Language lang);
	static size_t populatePropForest(const DocTheory* dt, const SentenceTheory* st, SPropForest& forest, SNodeContent::Language lang);

	static size_t getPropForestFromSentence(const DocTheory* dt, const Sentence* sent, SPropForest& forest, SNodeContent::Language lang);
	static size_t getPropForestFromDocument(const DocTheory* dt, SPropForest& forest, SNodeContent::Language lang);
	static size_t resurectPropForest(const std::string& filename, const DocTheory* dt, SPropForest& forest, SNodeContent::Language lang);
	static void printForest(const SPropForest&, std::wostream&, bool=true);
	static void cleanPropForest(SPropForest&);
};

#endif

