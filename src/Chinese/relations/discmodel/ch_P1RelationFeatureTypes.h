#ifndef CH_P1RELATION_FEATURE_TYPES_H
#define CH_P1RELATION_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/relations/discmodel/P1RelationFeatureTypes.h"


/** Home of the feature type class instances */
class ChineseP1RelationFeatureTypes : public P1RelationFeatureTypes {
private:
	friend class ChineseP1RelationFeatureTypesFactory;

	ChineseP1RelationFeatureTypes();
};


// RelationModel factory
class ChineseP1RelationFeatureTypesFactory: public P1RelationFeatureTypes::Factory {

	ChineseP1RelationFeatureTypes* types;
	virtual void ensureFeatureTypesInstantiated() { 
		if (types == 0) {
			types = _new ChineseP1RelationFeatureTypes(); 
		} 		
	}

public:
	ChineseP1RelationFeatureTypesFactory() { types = 0;}

};

#endif
