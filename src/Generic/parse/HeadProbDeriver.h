// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HEAD_PROB_DERIVER_H
#define HEAD_PROB_DERIVER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/UTF8InputStream.h"

class HeadProbDeriver
{
private:
	float unique_multiplier_pwt;
	float unique_multiplier_pt;

	NgramScoreTableGen<1>* headHistories_p;
	NgramScoreTableGen<2>* headHistories_pt;
	NgramScoreTableGen<3>* headHistories_pwt;
	NgramScoreTableGen<2>* headTransitions_p;
	NgramScoreTableGen<3>* headTransitions_pt;
	NgramScoreTableGen<4>* headTransitions_pwt;
	NgramScoreTableGen<3>* uniqueHeadTransitions_pwt;
	NgramScoreTableGen<2>* uniqueHeadTransitions_pt;

	NgramScoreTableGen<3>* storage_pwt;
	NgramScoreTableGen<2>* storage_pt;

	static const int initial_table_size;
	static const int initial_table_size_small;

public:
	HeadProbDeriver(UTF8InputStream& in);
	~HeadProbDeriver();
	void derive_tables();
	void derive_counts_and_probs();
	void derive_lambdas();
	void derive_tables_for_estimation();
	void pre_estimation();

	void print_tables(const char* filename);
	void set_unique_multiplier_pwt(float f) { unique_multiplier_pwt = f; }
	void set_unique_multiplier_pt(float f) { unique_multiplier_pt = f; }

	NgramScoreTableGen<2>* get_headHistories_pt() { return headHistories_pt; }
	NgramScoreTableGen<3>* get_headHistories_pwt() { return headHistories_pwt; }
	NgramScoreTableGen<2>* get_headTransitions_p() { return headTransitions_p; }
	NgramScoreTableGen<3>* get_headTransitions_pt() { return headTransitions_pt; }
	NgramScoreTableGen<4>* get_headTransitions_pwt() { return headTransitions_pwt; }

};

#endif
