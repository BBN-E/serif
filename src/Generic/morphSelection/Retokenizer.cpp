// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/morphSelection/Retokenizer.h"
#include "Generic/morphSelection/xx_Retokenizer.h"

Retokenizer* Retokenizer::_instance = 0;
Retokenizer &Retokenizer::getInstance() {
	if (!_instance) {
		_instance = _factory()->build();
	}
	// Do an integrity check; and if it fails, then destroy and recreate
	// the retokenizer;
	if (!_instance->getInstanceIntegrityCheck()) {
		delete _instance;
		_instance = _factory()->build();
	}
	return *_instance;
}

void Retokenizer::destroy() {
	delete _instance;
	_instance = 0;
}

boost::shared_ptr<Retokenizer::Factory> &Retokenizer::_factory() {
	static boost::shared_ptr<Retokenizer::Factory> factory(new FactoryFor<GenericRetokenizer>());
	return factory;
}
