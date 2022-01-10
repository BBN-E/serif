// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_DESCLINK_FEATURE_FUNCTIONS_H
#define AR_DESCLINK_FEATURE_FUNCTIONS_H

#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/edt/xx_DescLinkFeatureFunctions.h"

class ArabicDescLinkFeatureFunctions: public GenericDescLinkFeatureFunctions {
public:
	// See DescLinkFeatureFunctions.h for method documentation.
	static const SynNode* getNumericMod(const Mention *ment);
	static std::vector<const Mention*> getModNames(const Mention *ment);
	static bool hasMod(const Mention *ment);
	static std::vector<Symbol> getMods(const Mention* ment);
};
#endif
