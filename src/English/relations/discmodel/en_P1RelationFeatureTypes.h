#ifndef EN_P1RELATION_FEATURE_TYPES_H
#define EN_P1RELATION_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/relations/discmodel/P1RelationFeatureTypes.h"


/** Home of the feature type class instances */
class EnglishP1RelationFeatureTypes : public P1RelationFeatureTypes {
private:
	friend class EnglishP1RelationFeatureTypesFactory;

	EnglishP1RelationFeatureTypes();
};


// RelationModel factory
class EnglishP1RelationFeatureTypesFactory: public P1RelationFeatureTypes::Factory {

	EnglishP1RelationFeatureTypes* types;
	virtual void ensureFeatureTypesInstantiated() { 
		if (types == 0) {
			types = _new EnglishP1RelationFeatureTypes(); 
		} 		
	}

public:
	EnglishP1RelationFeatureTypesFactory() { types = 0;}

};

#endif
