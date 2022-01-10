// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_XX_EVALSPECIFICRULES_H
#define EN_XX_EVALSPECIFICRULES_H

#include "Generic/common/EvalSpecificRules.h"

class PIdFSentence;
class DTTagSet;
class SynNode;
class Mention;
class EntityType;

class EnglishEvalSpecificRules : public EvalSpecificRules {
private:
	friend class EnglishEvalSpecificRulesFactory;

public:
	static void NamesToNominals(const SynNode* root, Mention *ment, EntityType &etype);
};

class EnglishEvalSpecificRulesFactory: public EvalSpecificRules::Factory {
	virtual void NamesToNominals(const SynNode* root, Mention *ment, EntityType &etype) {  EnglishEvalSpecificRules::NamesToNominals(root, ment, etype); }
};


#endif
