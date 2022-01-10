#ifndef es_P1RELATION_FEATURE_TYPES_H
#define es_P1RELATION_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/relations/discmodel/P1RelationFeatureTypes.h"


/** Home of the feature type class instances */
class SpanishP1RelationFeatureTypes : public P1RelationFeatureTypes {
private:
	friend class SpanishP1RelationFeatureTypesFactory;

	SpanishP1RelationFeatureTypes();
};


// RelationModel factory
class SpanishP1RelationFeatureTypesFactory: public P1RelationFeatureTypes::Factory {

	SpanishP1RelationFeatureTypes* types;
	virtual void ensureFeatureTypesInstantiated() { 
		if (types == 0) {
			types = _new SpanishP1RelationFeatureTypes(); 
		} 		
	}

public:
	SpanishP1RelationFeatureTypesFactory() { types = 0;}

};

#endif
