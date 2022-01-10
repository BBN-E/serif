// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NAMELINKER_H
#define NAMELINKER_H

#include "Generic/edt/MentionLinker.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/edt/EntityGuess.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/ProbModel.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntityConstants.h"
#include "Generic/common/DebugStream.h"

class NameLinker: public MentionLinker {
public:
	//NameLinker ();
	NameLinker ();
	~NameLinker();

	virtual int linkMention (LexEntitySet * currSolution, int currMentionUID, Entity::Type linkType, LexEntitySet *results[], int max_results);
	void cleanUpAfterDocument();
	//void setBranchFactor(int branch_factor);
private:
	void loadWeights();
	int guessEntity(LexEntitySet * currSolution, Mention * currMention, Entity::Type linkType, EntityGuess *results[], int max_results);

	ProbModel *_genericModel;
	ProbModel *_newOldModel;

	Symbol _symOld, _symNew;
	Symbol _symNumbers[5];
	
	double* _generic_unseen_weights;

	static const double LOG_OF_ZERO;
	static DebugStream &_debugOut;
};

#endif
