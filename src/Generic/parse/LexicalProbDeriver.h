// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LEXICAL_PROB_DERIVER_H
#define LEXICAL_PROB_DERIVER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/UTF8InputStream.h"

class LexicalProbDeriver
{
private:
	float unique_multiplier_MtPHwt;
	float unique_multiplier_MtPHt;
	float unique_multiplier_Mt;

	int min_history_count;
	NgramScoreTableGen<7>* lexicalTransitions_original;
	NgramScoreTableGen<6>* lexicalHistories_MtPHwt;
	NgramScoreTableGen<5>* lexicalHistories_MtPHt;
	NgramScoreTableGen<2>* lexicalHistories_Mt;
	NgramScoreTableGen<1>* lexicalHistories_t;
	NgramScoreTableGen<7>* lexicalTransitions_MtPHwt;
	NgramScoreTableGen<6>* lexicalTransitions_MtPHt;
	NgramScoreTableGen<3>* lexicalTransitions_Mt;
	NgramScoreTableGen<2>* lexicalTransitions_t;
	NgramScoreTableGen<6>* uniqueLexicalTransitions_MtPHwt;
	NgramScoreTableGen<5>* uniqueLexicalTransitions_MtPHt;
	NgramScoreTableGen<2>* uniqueLexicalTransitions_Mt;

	// for k_estimation
	NgramScoreTableGen<6>* storage_MtPHwt;
	NgramScoreTableGen<5>* storage_MtPHt;
	NgramScoreTableGen<2>* storage_Mt;

	static const int initial_table_size_small;

public:
	LexicalProbDeriver(UTF8InputStream& in, int MHC);
	void derive_tables();
	void derive_counts_and_probs();
	void derive_lambdas();
	void print_tables(const char* filename);

	NgramScoreTableGen<6>* get_lexicalHistories_MtPHwt() { return lexicalHistories_MtPHwt; }
	NgramScoreTableGen<5>* get_lexicalHistories_MtPHt() { return lexicalHistories_MtPHt; }
	NgramScoreTableGen<2>* get_lexicalHistories_Mt() { return lexicalHistories_Mt; }
	NgramScoreTableGen<7>* get_lexicalTransitions_MtPHwt() { return lexicalTransitions_MtPHwt; }
	NgramScoreTableGen<6>* get_lexicalTransitions_MtPHt() { return lexicalTransitions_MtPHt; }
	NgramScoreTableGen<3>* get_lexicalTransitions_Mt() { return lexicalTransitions_Mt; }
	NgramScoreTableGen<2>* get_lexicalTransitions_t() { return lexicalTransitions_t; }

	void set_unique_multiplier_MtPHwt(float f) { unique_multiplier_MtPHwt = f; }
	void set_unique_multiplier_MtPHt(float f) { unique_multiplier_MtPHt = f; }
	void set_unique_multiplier_Mt(float f) { unique_multiplier_Mt = f; }

	void derive_tables_for_estimation();
	void pre_estimation();

private:

        template <size_t NN> 
        NgramScoreTableGen<NN> * prune_table(NgramScoreTableGen<NN> * table) {
          NgramScoreTableGen<NN> *tmp_table = table;
          table = tmp_table->prune(min_history_count);
          delete tmp_table;
          return table;
        }

};

#endif
