// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STATNAMELINKER_H
#define STATNAMELINKER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/ProbModel.h"
#include "Generic/common/DebugStream.h"
#include "Generic/edt/MentionLinker.h"
#include "Generic/edt/EntityGuess.h"
#include "Generic/edt/SimpleRuleNameLinker.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntityType.h"

class StatNameLinker: public MentionLinker {
public:
	//StatNameLinker ();
	StatNameLinker ();
	~StatNameLinker();

	virtual int linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, EntityType linkType, LexEntitySet *results[], int max_results);
	//void setBranchFactor(int branch_factor);
private:
	void loadWeights();
	int guessEntity(LexEntitySet * currSolution, Mention * currMention, EntityType linkType, EntityGuess *results[], int max_results);

	bool _filter_by_entity_subtype;
	
	ProbModel *_genericModel;
	ProbModel *_newOldModel;

	bool useRules;
	SimpleRuleNameLinker *_simpleRuleNameLinker;

	Symbol _symOld, _symNew;
	Symbol _symNumbers[5];
	
	double* _generic_unseen_weights;

	static const double LOG_OF_ZERO;
	static DebugStream &_debugOut;
};

#endif
