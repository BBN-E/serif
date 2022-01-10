// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_DESCLINK_FEATURE_FUNCTIONS_H
#define XX_DESCLINK_FEATURE_FUNCTIONS_H

#include "Generic/edt/DescLinkFeatureFunctions.h"

class GenericDescLinkFeatureFunctions {
	// Note: this class is intentionally not a subclass of
	// DescLinkFeatureFunctions.  See DescLinkFeatureFunctions.h for
	// an explanation.
public:
	// See DescLinkFeatureFunctions.h for method documentation.
	static const SynNode* getNumericMod(const Mention *ment) {return 0;};
	
	static std::vector<const Mention*> getModNames(const Mention *ment) { return std::vector<const Mention*>(); };
	
	static bool hasMod(const Mention *ment) {return false;};
	
	static std::vector<Symbol> getMods(const Mention* ment) { return std::vector<Symbol>(); };

	static Symbol getLinkSymbol() { return OC_LINK; }

 private:
	static Symbol OC_LINK;
};
#endif
