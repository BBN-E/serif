#ifndef ES_P1DESC_FEATURE_TYPES_H
#define ES_P1DESC_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

///** Home of the feature type class instances */
//class SpanishP1DescFeatureTypes : public GenericP1DescFeatureTypes {
//public:
//	static void ensureFeatureTypesInstantiated();
//private:
//	static bool _instantiated;
//};
//

#include "Generic/descriptors/discmodel/P1DescFeatureTypes.h"


/** Home of the feature type class instances */
class SpanishP1DescFeatureTypes : public P1DescFeatureTypes {
private:
	friend class SpanishP1DescFeatureTypesFactory;


public:
	static void ensureFeatureTypesInstantiated();
private:
	static bool _instantiated;

//	SpanishP1RelationFeatureTypes();
};


// RelationModel factory
class SpanishP1DescFeatureTypesFactory: public P1DescFeatureTypes::Factory {

	//SpanishP1RelationFeatureTypes* types;
	virtual void ensureFeatureTypesInstantiated() { 
		SpanishP1DescFeatureTypes::ensureFeatureTypesInstantiated();
	//	if (types == 0) {
	//		types = _new SpanishP1RelationFeatureTypes(); 
	//	} 		
	}

//public:
//	SpanishP1RelationFeatureTypesFactory() { types = 0;}

};



#endif
