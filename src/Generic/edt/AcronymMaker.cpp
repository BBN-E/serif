// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/AcronymMaker.h"
#include "Generic/edt/xx_AcronymMaker.h"


boost::shared_ptr<AcronymMaker::AcronymMakerFactory> &AcronymMaker::_factory() {
	static boost::shared_ptr<AcronymMakerFactory> factory(_new AcronymMakerFactoryFor<GenericAcronymMaker>());
	return factory;
}

AcronymMaker& AcronymMaker::getSingleton() {
	static boost::shared_ptr<AcronymMaker> instance(_factory()->build());
	return *instance;
}

int AcronymMaker::generateAcronyms(const Symbol symArray[], int nSymArray, Symbol results[], int max_results) {
	std::vector<Symbol> words;
	for (int i=0; i<nSymArray; ++i)
		words.push_back(symArray[i]);
	std::vector<Symbol> resultVector = generateAcronyms(words);
	int num_results = std::min(max_results, static_cast<int>(resultVector.size()));
	for (int i=0; i<num_results; ++i)
		results[i] = resultVector[i];
	return num_results;
}
