#ifndef AR_P1RELATION_FEATURE_TYPES_H
#define AR_P1RELATION_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/relations/discmodel/P1RelationFeatureTypes.h"


/** Home of the feature type class instances */
class ArabicP1RelationFeatureTypes : public P1RelationFeatureTypes {
private:
	friend class ArabicP1RelationFeatureTypesFactory;

	ArabicP1RelationFeatureTypes();
};


// RelationModel factory
class ArabicP1RelationFeatureTypesFactory: public P1RelationFeatureTypes::Factory {

	ArabicP1RelationFeatureTypes* types;
	virtual void ensureFeatureTypesInstantiated() { 
		if (types == 0) {
			types = _new ArabicP1RelationFeatureTypes(); 
		} 		
	}

public:
	ArabicP1RelationFeatureTypesFactory() { types = 0;}

};

#endif
