// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/parse/HeadProbDeriver.h"

const int HeadProbDeriver::initial_table_size_small = 1000;

HeadProbDeriver::HeadProbDeriver(UTF8InputStream& in)
{
	headTransitions_pwt = _new NgramScoreTableGen<4>(in);

	int size = headTransitions_pwt->get_size();

	headTransitions_pt = _new NgramScoreTableGen<3>(initial_table_size_small);
	headTransitions_p  = _new NgramScoreTableGen<2>(initial_table_size_small);
	headHistories_pwt  = _new NgramScoreTableGen<3>(size);
	headHistories_pt   = _new NgramScoreTableGen<2>(initial_table_size_small);
	headHistories_p    = _new NgramScoreTableGen<1>(initial_table_size_small);

	unique_multiplier_pwt = 3;
	unique_multiplier_pt  = 1;
}

HeadProbDeriver::~HeadProbDeriver() {
	delete headTransitions_pt;
	delete headTransitions_p;
	delete headHistories_pwt;
	delete headHistories_pt;
	delete headHistories_p;
}

void 
HeadProbDeriver::derive_tables()
{
	derive_counts_and_probs();
	derive_lambdas();

}

void 
HeadProbDeriver::derive_counts_and_probs()
{

  //NgramScoreTableGen::Table::iterator iter;
	float count;
	float history_count;
	float transition_count;
	Symbol ngram[4];

	for (NgramScoreTableGen<4>::Table::iterator iter = headTransitions_pwt->get_start(); iter != headTransitions_pwt->get_end(); ++iter) {
		count = (*iter).second;
		ngram[0] = (*iter).first[0]; //future
		ngram[1] = (*iter).first[1]; //P
		ngram[2] = (*iter).first[2]; //hw
		ngram[3] = (*iter).first[3]; //ht
		headHistories_pwt->add(ngram + 1, count); //P, hw, ht
		ngram[2] = ngram[3]; 
		headHistories_pt->add(ngram + 1, count); //P, ht
		headHistories_p->add(ngram + 1, count); //P
		headTransitions_pt->add(ngram, count); //future, P, ht
		headTransitions_p->add(ngram, count); //future, P
	}

	uniqueHeadTransitions_pwt = _new NgramScoreTableGen<3>(headHistories_pwt->get_size());
	for (NgramScoreTableGen<4>::Table::iterator iter = headTransitions_pwt->get_start(); iter != headTransitions_pwt->get_end(); ++iter) {
		count = (*iter).second;
		ngram[0] = (*iter).first[1]; //P
		ngram[1] = (*iter).first[2]; //hw
		ngram[2] = (*iter).first[3]; //ht
		uniqueHeadTransitions_pwt->add(ngram, 1);
	}

	uniqueHeadTransitions_pt  = _new NgramScoreTableGen<2>(headHistories_pt->get_size());
	for (NgramScoreTableGen<3>::Table::iterator iter = headTransitions_pt->get_start(); iter != headTransitions_pt->get_end(); ++iter) {
		ngram[0] = (*iter).first[1]; //P
		ngram[1] = (*iter).first[2]; //ht
		uniqueHeadTransitions_pt->add(ngram, 1);
	}

	for (NgramScoreTableGen<4>::Table::iterator iter = headTransitions_pwt->get_start(); iter != headTransitions_pwt->get_end(); ++iter) {
		transition_count = (*iter).second;
		ngram[0] = (*iter).first[1]; //P
		ngram[1] = (*iter).first[2]; //hw
		ngram[2] = (*iter).first[3]; //ht
		history_count = headHistories_pwt->lookup(ngram);
		(*iter).second = transition_count / history_count;
	}

	for (NgramScoreTableGen<3>::Table::iterator iter = headTransitions_pt->get_start(); iter != headTransitions_pt->get_end(); ++iter) {
		transition_count = (*iter).second;
		ngram[0] = (*iter).first[1]; //P
		ngram[1] = (*iter).first[2]; //ht
		history_count = headHistories_pt->lookup(ngram);
		(*iter).second = transition_count / history_count;
	}
		
	for (NgramScoreTableGen<2>::Table::iterator iter = headTransitions_p->get_start(); iter != headTransitions_p->get_end(); ++iter) {
		transition_count = (*iter).second;
		ngram[0] = (*iter).first[1]; //P
		history_count = headHistories_p->lookup(ngram);
		(*iter).second = transition_count / history_count;
	}
}

void 
HeadProbDeriver::derive_lambdas() 
{

  //NgramScoreTableGen::Table::iterator iter;
	float history_count;
	float unique_count;
	Symbol ngram[4];

	for (NgramScoreTableGen<3>::Table::iterator iter = headHistories_pwt->get_start(); iter != headHistories_pwt->get_end(); ++iter) {
		history_count = (*iter).second;
		ngram[0] = (*iter).first[0];
		ngram[1] = (*iter).first[1];
		ngram[2] = (*iter).first[2];
		unique_count = uniqueHeadTransitions_pwt->lookup(ngram);
		(*iter).second = history_count / (history_count + unique_multiplier_pwt * unique_count);
	}

	for (NgramScoreTableGen<2>::Table::iterator iter = headHistories_pt->get_start(); iter != headHistories_pt->get_end(); ++iter) {
		history_count = (*iter).second;
		ngram[0] = (*iter).first[0];
		ngram[1] = (*iter).first[1];
		unique_count = uniqueHeadTransitions_pt->lookup(ngram);
		(*iter).second = history_count / (history_count + unique_multiplier_pt * unique_count);
	}
}

void 
HeadProbDeriver::print_tables(const char* filename)
{
	UTF8OutputStream out;
	out.open(filename);

	headHistories_pwt->print_to_open_stream(out);
	headHistories_pt->print_to_open_stream(out);
	headTransitions_pwt->print_to_open_stream(out);
	headTransitions_pt->print_to_open_stream(out);
	headTransitions_p->print_to_open_stream(out);

	out.close();
}


/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////
// for k estimation //
//////////////////////

void 
HeadProbDeriver::pre_estimation()
{
	derive_counts_and_probs();

	storage_pwt = headHistories_pwt;
	storage_pt = headHistories_pt;
	
	headHistories_pwt  = _new NgramScoreTableGen<3>(headTransitions_pwt->get_size());
	headHistories_pt   = _new NgramScoreTableGen<2>(initial_table_size_small);

}

void 
HeadProbDeriver::derive_tables_for_estimation()
{
	headHistories_pwt->reset();
	headHistories_pt->reset();

	//NgramScoreTableGen::Table::iterator iter;
	float history_count;
	float unique_count;
	Symbol ngram[4];

	for (NgramScoreTableGen<3>::Table::iterator iter = storage_pwt->get_start(); iter != storage_pwt->get_end(); ++iter) {
		history_count = (*iter).second;
		ngram[0] = (*iter).first[0];
		ngram[1] = (*iter).first[1];
		ngram[2] = (*iter).first[2];
		unique_count = uniqueHeadTransitions_pwt->lookup(ngram);
		headHistories_pwt->add((*iter).first, 
			history_count / (history_count + unique_multiplier_pwt * unique_count));
	}

	for (NgramScoreTableGen<2>::Table::iterator iter = storage_pt->get_start(); iter != storage_pt->get_end(); ++iter) {
		history_count = (*iter).second;
		ngram[0] = (*iter).first[0];
		ngram[1] = (*iter).first[1];
		unique_count = uniqueHeadTransitions_pt->lookup(ngram);
		headHistories_pt->add((*iter).first, 
			history_count / (history_count + unique_multiplier_pt * unique_count));
	}
}
