#ifndef XX_RESULT_COLLECTION_UTILITIES_H
#define XX_RESULT_COLLECTION_UTILITIES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/results/ResultCollectionUtilities.h"


class GenericResultCollectionUtilities : public ResultCollectionUtilities {
private:
	friend class GenericResultCollectionUtilitiesFactory;

public:
	static bool isPREmention(const EntitySet *entitySet, Mention *ment) { return false; }
};

class GenericResultCollectionUtilitiesFactory: public ResultCollectionUtilities::Factory {
	virtual bool isPREmention(const EntitySet *entitySet, Mention *ment) {  return GenericResultCollectionUtilities::isPREmention(entitySet, ment); }
};




#endif
