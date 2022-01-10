// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/StringTransliterator.h"
#include "Generic/common/xx_StringTransliterator.h"




boost::shared_ptr<StringTransliterator::Factory> &StringTransliterator::_factory() {
	static boost::shared_ptr<StringTransliterator::Factory> factory(new GenericStringTransliteratorFactory());
	return factory;
}

