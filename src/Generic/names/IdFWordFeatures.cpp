// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/names/IdFWordFeatures.h"
#include "Generic/names/IdFListSet.h"
#include "Generic/names/DefaultIdFWordFeatures.h"

IdFWordFeatures::~IdFWordFeatures() {
    if (_listSet) {
        delete _listSet;
    }
}

boost::shared_ptr<IdFWordFeatures::Factory> &IdFWordFeatures::_factory() {
	static boost::shared_ptr<IdFWordFeatures::Factory> factory(_new FactoryFor<DefaultIdFWordFeatures>());
	return factory;
}

