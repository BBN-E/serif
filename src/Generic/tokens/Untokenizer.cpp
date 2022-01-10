// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/tokens/Untokenizer.h"
#include "Generic/tokens/xx_Untokenizer.h"




boost::shared_ptr<Untokenizer::Factory> &Untokenizer::_factory() {
	static boost::shared_ptr<Untokenizer::Factory> factory(new GenericUntokenizerFactory());
	return factory;
}

