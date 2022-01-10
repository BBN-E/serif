// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/eeml/GroupFnGuesser.h"
#include "Generic/eeml/xx_GroupFnGuesser.h"




boost::shared_ptr<GroupFnGuesser::Factory> &GroupFnGuesser::_factory() {
	static boost::shared_ptr<GroupFnGuesser::Factory> factory(new GenericGroupFnGuesserFactory());
	return factory;
}

