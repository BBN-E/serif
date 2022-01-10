// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"
#include "names/discmodel/PIdFFeatureTypes.h"


bool PIdFFeatureTypes::_instantiated = false;

void PIdFFeatureTypes::ensureFeatureTypesInstantiated() {
	if (_instantiated)
		return;
	_instantiated = true;
}
