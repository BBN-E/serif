#ifndef XX_P1RELATION_FEATURE_TYPES_H
#define XX_P1RELATION_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/relations/discmodel/P1RelationFeatureTypes.h"


/** Home of the feature type class instances */
class DefaultP1RelationFeatureTypes : public P1RelationFeatureTypes {
private:
	friend class DefaultP1RelationFeatureTypesFactory;

	DefaultP1RelationFeatureTypes() {};
};


// RelationModel factory
class DefaultP1RelationFeatureTypesFactory: public P1RelationFeatureTypes::Factory {

	DefaultP1RelationFeatureTypes* types;
	virtual void ensureFeatureTypesInstantiated() { 
		if (types == 0) {
			types = _new DefaultP1RelationFeatureTypes(); 
		} 		
	}

public:
	DefaultP1RelationFeatureTypesFactory() { types = 0;}

};

#endif
