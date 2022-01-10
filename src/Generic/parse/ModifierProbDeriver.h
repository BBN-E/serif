// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MODIFIER_PROB_DERIVER_H
#define MODIFIER_PROB_DERIVER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/UTF8InputStream.h"

class ModifierProbDeriver
{
private:
	float unique_multiplier_PHpwt;
	float unique_multiplier_PHpt;

	NgramScoreTableGen<5>* modifierHistories_PHpwt;
	NgramScoreTableGen<4>* modifierHistories_PHpt;
	NgramScoreTableGen<3>* modifierHistories_PHp;
	NgramScoreTableGen<7>* modifierTransitions_PHpwt;
	NgramScoreTableGen<6>* modifierTransitions_PHpt;
	NgramScoreTableGen<5>* modifierTransitions_PHp;
	NgramScoreTableGen<5>* uniqueModifierTransitions_PHpwt;
	NgramScoreTableGen<4>* uniqueModifierTransitions_PHpt;

	NgramScoreTableGen<5>* storage_PHpwt;
	NgramScoreTableGen<4>* storage_PHpt;

	static const int initial_table_size_small;
	static const int initial_table_size_smaller;

public:
	ModifierProbDeriver(UTF8InputStream& in);
	void derive_tables();
	void derive_counts_and_probs();
	void derive_lambdas();
	void print_tables(const char* filename);

	NgramScoreTableGen<5>* get_modifierHistories_PHpwt () { return modifierHistories_PHpwt; }
	NgramScoreTableGen<4>* get_modifierHistories_PHpt () { return modifierHistories_PHpt; };
	NgramScoreTableGen<7>* get_modifierTransitions_PHpwt () { return modifierTransitions_PHpwt; }
	NgramScoreTableGen<6>* get_modifierTransitions_PHpt () { return modifierTransitions_PHpt; }
	NgramScoreTableGen<5>* get_modifierTransitions_PHp () { return modifierTransitions_PHp; }

	void set_unique_multiplier_PHpwt(float f) { unique_multiplier_PHpwt = f; }
	void set_unique_multiplier_PHpt(float f) { unique_multiplier_PHpt = f; }

	void derive_tables_for_estimation();
	void pre_estimation();

};

#endif
