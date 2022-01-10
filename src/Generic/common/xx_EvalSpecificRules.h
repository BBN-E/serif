// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_EVALSPECIFICRULES_H
#define XX_EVALSPECIFICRULES_H

#include "Generic/common/EvalSpecificRules.h"

class PIdFSentence;
class DTTagSet;
class SynNode;
class Mention;
class EntityType;

class GenericEvalSpecificRules : public EvalSpecificRules {
private:
	friend class GenericEvalSpecificRulesFactory;

public:
	static void fixEuropeanUnion(PIdFSentence &sentence, DTTagSet *tagSet) {};
	static void NamesToNominals(const SynNode* root, Mention *ment, EntityType &etype) {};
};

class GenericEvalSpecificRulesFactory: public EvalSpecificRules::Factory {
	virtual void NamesToNominals(const SynNode* root, Mention *ment, EntityType &etype) {  GenericEvalSpecificRules::NamesToNominals(root, ment, etype); }
};


#endif
