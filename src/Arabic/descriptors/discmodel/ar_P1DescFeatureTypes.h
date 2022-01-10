#ifndef AR_P1DESC_FEATURE_TYPES_H
#define AR_P1DESC_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

///** Home of the feature type class instances */
//class ArabicP1DescFeatureTypes : public GenericP1DescFeatureTypes {
//public:
//	static void ensureFeatureTypesInstantiated();
//private:
//	static bool _instantiated;
//};
//

#include "Generic/descriptors/discmodel/P1DescFeatureTypes.h"


/** Home of the feature type class instances */
class ArabicP1DescFeatureTypes : public P1DescFeatureTypes {
private:
	friend class ArabicP1DescFeatureTypesFactory;


public:
	static void ensureFeatureTypesInstantiated();
private:
	static bool _instantiated;

//	ArabicP1RelationFeatureTypes();
};


// RelationModel factory
class ArabicP1DescFeatureTypesFactory: public P1DescFeatureTypes::Factory {

	//ArabicP1RelationFeatureTypes* types;
	virtual void ensureFeatureTypesInstantiated() { 
		ArabicP1DescFeatureTypes::ensureFeatureTypesInstantiated();
	//	if (types == 0) {
	//		types = _new ArabicP1RelationFeatureTypes(); 
	//	} 		
	}

//public:
//	ArabicP1RelationFeatureTypesFactory() { types = 0;}

};



#endif
