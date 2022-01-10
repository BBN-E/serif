// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/Symbol.h"
#include "Generic/parse/LexicalProbDeriver.h"
#include "Generic/common/UTF8OutputStream.h"

const int LexicalProbDeriver::initial_table_size_small = 7000;

LexicalProbDeriver::LexicalProbDeriver(UTF8InputStream& in, int MHC)
{
	lexicalTransitions_original = _new NgramScoreTableGen<7>(in);

	int size = lexicalTransitions_original->get_size();

	lexicalTransitions_MtPHwt = _new NgramScoreTableGen<7>(size);
	lexicalTransitions_MtPHt  = _new NgramScoreTableGen<6>((int) size / 2);
	lexicalTransitions_Mt     = _new NgramScoreTableGen<3>((int) size / 4);
	lexicalTransitions_t      = _new NgramScoreTableGen<2>((int) size / 4);
	lexicalHistories_MtPHwt   = _new NgramScoreTableGen<6>((int) size / 2);
	lexicalHistories_MtPHt    = _new NgramScoreTableGen<5>(initial_table_size_small);
	lexicalHistories_Mt       = _new NgramScoreTableGen<2>(initial_table_size_small);
	lexicalHistories_t        = _new NgramScoreTableGen<1>(initial_table_size_small);

	unique_multiplier_MtPHwt = 3;
	unique_multiplier_MtPHt  = 1;
	unique_multiplier_Mt     = 1;

	min_history_count = MHC;
}

void 
LexicalProbDeriver::derive_tables()
{

	derive_counts_and_probs();
	derive_lambdas();

}

void 
LexicalProbDeriver::derive_counts_and_probs()
{

  //NgramScoreTableGen<7>::Table::iterator iter;
	float count;
	float history_count;
	float transition_count;
	Symbol ngram[7];

	for (NgramScoreTableGen<7>::Table::iterator iter = lexicalTransitions_original->get_start(); iter != lexicalTransitions_original->get_end(); ++iter) {
		count = (*iter).second;
		ngram[0] = (*iter).first[0]; //future (mw)
		ngram[1] = (*iter).first[1]; //M
		ngram[2] = (*iter).first[2]; //mt
		ngram[3] = (*iter).first[3]; //P
		ngram[4] = (*iter).first[4]; //H
		ngram[5] = (*iter).first[5]; //hw
		ngram[6] = (*iter).first[6]; //ht
		lexicalHistories_MtPHwt->add(ngram + 1, count); 
		ngram[5] = ngram[6]; 
		lexicalHistories_MtPHt->add(ngram + 1, count); 
		lexicalHistories_Mt->add(ngram + 1, count); 
		ngram[1] = ngram[2];
		lexicalHistories_t->add(ngram + 1, count); 
	}

        lexicalHistories_MtPHwt = prune_table<6>(lexicalHistories_MtPHwt);
	lexicalHistories_MtPHt = prune_table<5>(lexicalHistories_MtPHt);
	lexicalHistories_Mt = prune_table<2>(lexicalHistories_Mt);
	lexicalHistories_t = prune_table<1>(lexicalHistories_t);

	uniqueLexicalTransitions_MtPHwt = _new NgramScoreTableGen<6>(lexicalHistories_MtPHwt->get_size());
	uniqueLexicalTransitions_MtPHt  = _new NgramScoreTableGen<5>(lexicalHistories_MtPHt->get_size());
	uniqueLexicalTransitions_Mt     = _new NgramScoreTableGen<2>(lexicalHistories_Mt->get_size());

	for (NgramScoreTableGen<7>::Table::iterator iter = lexicalTransitions_original->get_start(); iter != lexicalTransitions_original->get_end(); ++iter) {
		count = (*iter).second;
		ngram[0] = (*iter).first[0]; //future (mw)
		ngram[1] = (*iter).first[1]; //M
		ngram[2] = (*iter).first[2]; //mt
		ngram[3] = (*iter).first[3]; //P
		ngram[4] = (*iter).first[4]; //H
		ngram[5] = (*iter).first[5]; //hw
		ngram[6] = (*iter).first[6]; //ht
		if (lexicalHistories_MtPHwt->lookup(ngram + 1))
			lexicalTransitions_MtPHwt->add(ngram, count); 
		ngram[5] = ngram[6]; 
		if (lexicalHistories_MtPHt->lookup(ngram + 1))
			lexicalTransitions_MtPHt->add(ngram, count);
		if (lexicalHistories_Mt->lookup(ngram + 1))
			lexicalTransitions_Mt->add(ngram, count); 
		ngram[1] = ngram[2];
		if (lexicalHistories_t->lookup(ngram + 1))
			lexicalTransitions_t->add(ngram, count);
	}

	for (NgramScoreTableGen<7>::Table::iterator iter = lexicalTransitions_MtPHwt->get_start(); iter != lexicalTransitions_MtPHwt->get_end(); ++iter) {
		ngram[0] = (*iter).first[1]; //M
		ngram[1] = (*iter).first[2]; //mt
		ngram[2] = (*iter).first[3]; //P
		ngram[3] = (*iter).first[4]; //H
		ngram[4] = (*iter).first[5]; //hw
		ngram[5] = (*iter).first[6]; //ht
		uniqueLexicalTransitions_MtPHwt->add(ngram, 1);
	}
	
	for (NgramScoreTableGen<6>::Table::iterator iter = lexicalTransitions_MtPHt->get_start(); iter != lexicalTransitions_MtPHt->get_end(); ++iter) {
		ngram[0] = (*iter).first[1]; //M
		ngram[1] = (*iter).first[2]; //mt
		ngram[2] = (*iter).first[3]; //P
		ngram[3] = (*iter).first[4]; //H
		ngram[4] = (*iter).first[5]; //ht
		uniqueLexicalTransitions_MtPHt->add(ngram, 1);
	}

	for (NgramScoreTableGen<3>::Table::iterator iter = lexicalTransitions_Mt->get_start(); iter != lexicalTransitions_Mt->get_end(); ++iter) {
		ngram[0] = (*iter).first[1]; //M
		ngram[1] = (*iter).first[2]; //mt
		uniqueLexicalTransitions_Mt->add(ngram, 1);
	}

	for (NgramScoreTableGen<7>::Table::iterator iter = lexicalTransitions_MtPHwt->get_start(); iter != lexicalTransitions_MtPHwt->get_end(); ++iter) {
		transition_count = (*iter).second;
		ngram[0] = (*iter).first[1]; //M
		ngram[1] = (*iter).first[2]; //mt
		ngram[2] = (*iter).first[3]; //P
		ngram[3] = (*iter).first[4]; //H
		ngram[4] = (*iter).first[5]; //hw
		ngram[5] = (*iter).first[6]; //ht
		history_count = lexicalHistories_MtPHwt->lookup(ngram);
		(*iter).second = transition_count / history_count;
	}

	for (NgramScoreTableGen<6>::Table::iterator iter = lexicalTransitions_MtPHt->get_start(); iter != lexicalTransitions_MtPHt->get_end(); ++iter) {
		transition_count = (*iter).second;
		ngram[0] = (*iter).first[1]; //M
		ngram[1] = (*iter).first[2]; //mt
		ngram[2] = (*iter).first[3]; //P
		ngram[3] = (*iter).first[4]; //H
		ngram[4] = (*iter).first[5]; //ht
		history_count = lexicalHistories_MtPHt->lookup(ngram);
		(*iter).second = transition_count / history_count;
	}
		
	for (NgramScoreTableGen<3>::Table::iterator iter = lexicalTransitions_Mt->get_start(); iter != lexicalTransitions_Mt->get_end(); ++iter) {
		transition_count = (*iter).second;
		ngram[0] = (*iter).first[1]; //M
		ngram[1] = (*iter).first[2]; //mt
		history_count = lexicalHistories_Mt->lookup(ngram);
		(*iter).second = transition_count / history_count;
	}

	for (NgramScoreTableGen<2>::Table::iterator iter = lexicalTransitions_t->get_start(); iter != lexicalTransitions_t->get_end(); ++iter) {
		transition_count = (*iter).second;
		ngram[0] = (*iter).first[1]; //M
		history_count = lexicalHistories_t->lookup(ngram);
		(*iter).second = transition_count / history_count;
	}

}


void 
LexicalProbDeriver::derive_lambdas()
{
  //NgramScoreTableGen::Table::iterator iter;
	float history_count;
	float unique_count;
	Symbol ngram[7];

	for (NgramScoreTableGen<6>::Table::iterator iter = lexicalHistories_MtPHwt->get_start(); iter != lexicalHistories_MtPHwt->get_end(); ++iter) {
		history_count = (*iter).second;
		ngram[0] = (*iter).first[0]; //M
		ngram[1] = (*iter).first[1]; //mt
		ngram[2] = (*iter).first[2]; //P
		ngram[3] = (*iter).first[3]; //H
		ngram[4] = (*iter).first[4]; //hw
		ngram[5] = (*iter).first[5]; //ht
		unique_count = uniqueLexicalTransitions_MtPHwt->lookup(ngram);
		(*iter).second = history_count / (history_count + unique_multiplier_MtPHwt * unique_count);
	}

	for (NgramScoreTableGen<5>::Table::iterator iter = lexicalHistories_MtPHt->get_start(); iter != lexicalHistories_MtPHt->get_end(); ++iter) {
		history_count = (*iter).second;
		ngram[0] = (*iter).first[0]; //M
		ngram[1] = (*iter).first[1]; //mt
		ngram[2] = (*iter).first[2]; //P
		ngram[3] = (*iter).first[3]; //H
		ngram[4] = (*iter).first[4]; //ht
		unique_count = uniqueLexicalTransitions_MtPHt->lookup(ngram);
		(*iter).second = history_count / (history_count + unique_multiplier_MtPHt * unique_count);
	}

	for (NgramScoreTableGen<2>::Table::iterator iter = lexicalHistories_Mt->get_start(); iter != lexicalHistories_Mt->get_end(); ++iter) {
		history_count = (*iter).second;
		ngram[0] = (*iter).first[0]; //M
		ngram[1] = (*iter).first[1]; //mt
		unique_count = uniqueLexicalTransitions_Mt->lookup(ngram);
		(*iter).second = history_count / (history_count + unique_multiplier_Mt * unique_count);
	}
}

void 
LexicalProbDeriver::print_tables(const char* filename)
{
	UTF8OutputStream out;
	out.open(filename);

	lexicalHistories_MtPHwt->print_to_open_stream(out);
	lexicalHistories_MtPHt->print_to_open_stream(out);
	lexicalHistories_Mt->print_to_open_stream(out);
	lexicalTransitions_MtPHwt->print_to_open_stream(out);
	lexicalTransitions_MtPHt->print_to_open_stream(out);
	lexicalTransitions_Mt->print_to_open_stream(out);
	lexicalTransitions_t->print_to_open_stream(out);

	out.close();
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////
// for k estimation //
//////////////////////


void 
LexicalProbDeriver::pre_estimation()
{
	derive_counts_and_probs();

	storage_MtPHwt = lexicalHistories_MtPHwt;
	storage_MtPHt = lexicalHistories_MtPHt;
	storage_Mt = lexicalHistories_Mt;

	lexicalHistories_MtPHwt   = _new NgramScoreTableGen<6>((int) lexicalTransitions_original->get_size() / 2);
	lexicalHistories_MtPHt    = _new NgramScoreTableGen<5>(initial_table_size_small);
	lexicalHistories_Mt       = _new NgramScoreTableGen<2>(initial_table_size_small);

}

void 
LexicalProbDeriver::derive_tables_for_estimation()
{
	lexicalHistories_MtPHwt->reset();
	lexicalHistories_MtPHt->reset();
	lexicalHistories_Mt->reset();

	//NgramScoreTableGen::Table::iterator iter;
	float history_count;
	float unique_count;
	Symbol ngram[7];

	for (NgramScoreTableGen<6>::Table::iterator iter = storage_MtPHwt->get_start(); iter != storage_MtPHwt->get_end(); ++iter) {
		history_count = (*iter).second;
		ngram[0] = (*iter).first[0]; //M
		ngram[1] = (*iter).first[1]; //mt
		ngram[2] = (*iter).first[2]; //P
		ngram[3] = (*iter).first[3]; //H
		ngram[4] = (*iter).first[4]; //hw
		ngram[5] = (*iter).first[5]; //ht
		unique_count = uniqueLexicalTransitions_MtPHwt->lookup(ngram);
		lexicalHistories_MtPHwt->add((*iter).first, 
			history_count / (history_count + unique_multiplier_MtPHwt * unique_count));
	}

	for (NgramScoreTableGen<5>::Table::iterator iter = storage_MtPHt->get_start(); iter != storage_MtPHt->get_end(); ++iter) {
		history_count = (*iter).second;
		ngram[0] = (*iter).first[0]; //M
		ngram[1] = (*iter).first[1]; //mt
		ngram[2] = (*iter).first[2]; //P
		ngram[3] = (*iter).first[3]; //H
		ngram[4] = (*iter).first[4]; //ht
		unique_count = uniqueLexicalTransitions_MtPHt->lookup(ngram);
		lexicalHistories_MtPHt->add((*iter).first, 
			history_count / (history_count + unique_multiplier_MtPHt * unique_count));
	}

	for (NgramScoreTableGen<2>::Table::iterator iter = storage_Mt->get_start(); iter != storage_Mt->get_end(); ++iter) {
		history_count = (*iter).second;
		ngram[0] = (*iter).first[0]; //M
		ngram[1] = (*iter).first[1]; //mt
		unique_count = uniqueLexicalTransitions_Mt->lookup(ngram);
		lexicalHistories_Mt->add((*iter).first, 
			history_count / (history_count + unique_multiplier_Mt * unique_count));
	}
}
