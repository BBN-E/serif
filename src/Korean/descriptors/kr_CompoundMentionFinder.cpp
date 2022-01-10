// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "descriptors/CompoundMentionFinder.h"

CompoundMentionFinder* CompoundMentionFinder::_instance = 0;

GenericCompoundMentionFinder* CompoundMentionFinder::getInstance() {
	if (_instance == 0) {
		_instance = _new CompoundMentionFinder();
	}
	return _instance;
}

void CompoundMentionFinder::deleteInstance() {
	delete _instance;
	_instance = 0;
}
