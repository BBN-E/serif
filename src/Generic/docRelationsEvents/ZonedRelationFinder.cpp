// Copyright 2015 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/docRelationsEvents/ZonedRelationFinder.h"
#include "Generic/docRelationsEvents/DefaultZonedRelationFinder.h"

namespace { 
	template<typename T> struct ZonedRelationFinderFactoryFor: public ZonedRelationFinder::Factory { 
		virtual ZonedRelationFinder *build() { return _new T(); }
	};
}

boost::shared_ptr<ZonedRelationFinder::Factory> &ZonedRelationFinder::_factory() {
	static boost::shared_ptr<ZonedRelationFinder::Factory> factory(_new ZonedRelationFinderFactoryFor<DefaultZonedRelationFinder>());
	return factory;
}
