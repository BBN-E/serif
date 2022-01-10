// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/descriptors/CompoundMentionFinder.h"
#include "Generic/descriptors/xx_CompoundMentionFinder.h"

CompoundMentionFinder* CompoundMentionFinder::_instance = 0;

CompoundMentionFinder* CompoundMentionFinder::getInstance() {
	if (_instance == 0)
		_instance = _factory()->build();
	return _instance;
}

void CompoundMentionFinder::deleteInstance() {
	delete _instance;
	_instance = 0;
}

boost::shared_ptr<CompoundMentionFinder::Factory> &CompoundMentionFinder::_factory() {
	static boost::shared_ptr<CompoundMentionFinder::Factory> factory(
		_new FactoryFor<GenericCompoundMentionFinder>());
	return factory;
}
