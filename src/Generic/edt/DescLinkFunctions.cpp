// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/edt/DescLinkFunctions.h"
#include "Generic/edt/xx_DescLinkFunctions.h"

Symbol DescLinkFunctions::OC_LINK = Symbol(L"o[link]");
Symbol DescLinkFunctions::OC_NO_LINK = Symbol(L"o[no-link]");
Symbol DescLinkFunctions::HEAD_WORD_MATCH = Symbol(L"HeadWordMatch");


boost::shared_ptr<DescLinkFunctions::Factory> &DescLinkFunctions::_factory() {
	static boost::shared_ptr<DescLinkFunctions::Factory> factory(new GenericDescLinkFunctionsFactory());
	return factory;
}

