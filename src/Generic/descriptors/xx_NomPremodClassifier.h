// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_NOM_PREMOD_CLASSIFIER_H
#define xx_NOM_PREMOD_CLASSIFIER_H

#include "Generic/descriptors/NomPremodClassifier.h"
#include "Generic/theories/EntityType.h"

class DefaultNomPremodClassifier : public NomPremodClassifier {
private:
	friend class DefaultNomPremodClassifierFactory;
	
public:


	virtual int classifyNomPremod(MentionSet *currSolution, const SynNode* node, EntityType types[], 
									double scores[], int maxResults) 
	{ 
		types[0] = EntityType::getOtherType();
		scores[0] = 0;
		return 1;
	}

private:
	DefaultNomPremodClassifier() {}

};

class DefaultNomPremodClassifierFactory: public NomPremodClassifier::Factory {
	virtual NomPremodClassifier *build() { return _new DefaultNomPremodClassifier(); }
};


#endif
