// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DT_LC_NAMELIST_FEATURE_TYPE_H
#define DT_LC_NAMELIST_FEATURE_TYPE_H

#include "Generic/discTagger/DTWordNameListFeatureType.h"
class DTWordLCNameListFeatureType : public DTWordNameListFeatureType {
public:
	DTWordLCNameListFeatureType(Symbol model, Symbol dict_name, Symbol dict_filename)
		: DTWordNameListFeatureType(model, dict_name, dict_filename, true) {}
};
#endif
