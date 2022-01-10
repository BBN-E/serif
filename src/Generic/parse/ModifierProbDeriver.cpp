// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/parse/ModifierProbDeriver.h"

const int ModifierProbDeriver::initial_table_size_small = 17000;
const int ModifierProbDeriver::initial_table_size_smaller = 5000;

ModifierProbDeriver::ModifierProbDeriver(UTF8InputStream& in)
{
	modifierTransitions_PHpwt = _new NgramScoreTableGen<7>(in);

	int size = modifierTransitions_PHpwt->get_size();

	modifierTransitions_PHpt = _new NgramScoreTableGen<6>(initial_table_size_small);
	modifierTransitions_PHp  = _new NgramScoreTableGen<5>(initial_table_size_small);
	modifierHistories_PHpwt  = _new NgramScoreTableGen<5>(size);
	modifierHistories_PHpt   = _new NgramScoreTableGen<4>(initial_table_size_smaller);
	modifierHistories_PHp    = _new NgramScoreTableGen<3>(initial_table_size_smaller);
	
	unique_multiplier_PHpwt = 2;
	unique_multiplier_PHpt  = 1;
}

void 
ModifierProbDeriver::derive_tables()
{
	derive_counts_and_probs();
	derive_lambdas();

}

void 
ModifierProbDeriver::derive_counts_and_probs()
{

  //NgramScoreTableGen::Table::iterator iter;
	float count;
	float history_count;
	float transition_count;
	Symbol ngram[7];

	for (NgramScoreTableGen<7>::Table::iterator iter = modifierTransitions_PHpwt->get_start(); iter != modifierTransitions_PHpwt->get_end(); ++iter) {
		count = (*iter).second;
		ngram[0] = (*iter).first[0]; //future (chain)
		ngram[1] = (*iter).first[1]; //future (tag)
		ngram[2] = (*iter).first[2]; //P
		ngram[3] = (*iter).first[3]; //H
		ngram[4] = (*iter).first[4]; //prev
		ngram[5] = (*iter).first[5]; //hw
		ngram[6] = (*iter).first[6]; //ht
		modifierHistories_PHpwt->add(ngram + 2, count); 
		ngram[5] = ngram[6]; 
		modifierHistories_PHpt->add(ngram + 2, count);
		modifierHistories_PHp->add(ngram + 2, count);
		modifierTransitions_PHpt->add(ngram, count); 
		modifierTransitions_PHp->add(ngram, count); 
	}

	uniqueModifierTransitions_PHpwt = _new NgramScoreTableGen<5>(modifierHistories_PHpwt->get_size());
	for (NgramScoreTableGen<7>::Table::iterator iter = modifierTransitions_PHpwt->get_start(); iter != modifierTransitions_PHpwt->get_end(); ++iter) {
		count = (*iter).second;
		ngram[0] = (*iter).first[2]; //P
		ngram[1] = (*iter).first[3]; //H
		ngram[2] = (*iter).first[4]; //prev
		ngram[3] = (*iter).first[5]; //hw
		ngram[4] = (*iter).first[6]; //ht
		uniqueModifierTransitions_PHpwt->add(ngram, 1);
	}
	
	uniqueModifierTransitions_PHpt  = _new NgramScoreTableGen<4>(modifierHistories_PHpt->get_size());
	for (NgramScoreTableGen<6>::Table::iterator iter = modifierTransitions_PHpt->get_start(); iter != modifierTransitions_PHpt->get_end(); ++iter) {
		ngram[0] = (*iter).first[2]; //P
		ngram[1] = (*iter).first[3]; //H
		ngram[2] = (*iter).first[4]; //prev
		ngram[3] = (*iter).first[5]; //ht
		uniqueModifierTransitions_PHpt->add(ngram, 1);
	}

	for (NgramScoreTableGen<7>::Table::iterator iter = modifierTransitions_PHpwt->get_start(); iter != modifierTransitions_PHpwt->get_end(); ++iter) {
		transition_count = (*iter).second;
		ngram[0] = (*iter).first[2]; //P
		ngram[1] = (*iter).first[3]; //H
		ngram[2] = (*iter).first[4]; //prev
		ngram[3] = (*iter).first[5]; //hw
		ngram[4] = (*iter).first[6]; //ht
		history_count = modifierHistories_PHpwt->lookup(ngram);
		(*iter).second = transition_count / history_count;
	}

	for (NgramScoreTableGen<6>::Table::iterator iter = modifierTransitions_PHpt->get_start(); iter != modifierTransitions_PHpt->get_end(); ++iter) {
		transition_count = (*iter).second;
		ngram[0] = (*iter).first[2]; //P
		ngram[1] = (*iter).first[3]; //H
		ngram[2] = (*iter).first[4]; //prev
		ngram[3] = (*iter).first[5]; //ht
		history_count = modifierHistories_PHpt->lookup(ngram);
		(*iter).second = transition_count / history_count;
	}
		
	for (NgramScoreTableGen<5>::Table::iterator iter = modifierTransitions_PHp->get_start(); iter != modifierTransitions_PHp->get_end(); ++iter) {
		transition_count = (*iter).second;
		ngram[0] = (*iter).first[2]; //P
		ngram[1] = (*iter).first[3]; //H
		ngram[2] = (*iter).first[4]; //prev
		history_count = modifierHistories_PHp->lookup(ngram);
		(*iter).second = transition_count / history_count;
	}
}

void 
ModifierProbDeriver::derive_lambdas()
{

  //NgramScoreTableGen::Table::iterator iter;
	float history_count;
	float unique_count;
	Symbol ngram[7];

	for (NgramScoreTableGen<5>::Table::iterator iter = modifierHistories_PHpwt->get_start(); iter != modifierHistories_PHpwt->get_end(); ++iter) {
		history_count = (*iter).second;
		ngram[0] = (*iter).first[0]; //P
		ngram[1] = (*iter).first[1]; //H
		ngram[2] = (*iter).first[2]; //prev
		ngram[3] = (*iter).first[3]; //hw
		ngram[4] = (*iter).first[4]; //ht
		unique_count = uniqueModifierTransitions_PHpwt->lookup(ngram);
		(*iter).second = history_count / (history_count + unique_multiplier_PHpwt * unique_count);
	}

	for (NgramScoreTableGen<4>::Table::iterator iter = modifierHistories_PHpt->get_start(); iter != modifierHistories_PHpt->get_end(); ++iter) {
		history_count = (*iter).second;
		ngram[0] = (*iter).first[0]; //P
		ngram[1] = (*iter).first[1]; //H
		ngram[2] = (*iter).first[2]; //prev
		ngram[3] = (*iter).first[3]; //ht
		unique_count = uniqueModifierTransitions_PHpt->lookup(ngram);
		(*iter).second = history_count / (history_count + unique_multiplier_PHpt * unique_count);
	}
}

void 
ModifierProbDeriver::print_tables(const char* filename)
{
	UTF8OutputStream out;
	out.open(filename);

	modifierHistories_PHpwt->print_to_open_stream(out);
	modifierHistories_PHpt->print_to_open_stream(out);
	modifierTransitions_PHpwt->print_to_open_stream(out);
	modifierTransitions_PHpt->print_to_open_stream(out);
	modifierTransitions_PHp->print_to_open_stream(out);

	out.close();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////
// for k estimation //
//////////////////////

void 
ModifierProbDeriver::pre_estimation()
{
	derive_counts_and_probs();

	storage_PHpwt = modifierHistories_PHpwt;
	storage_PHpt = modifierHistories_PHpt;

	modifierHistories_PHpwt  = _new NgramScoreTableGen<5>(modifierTransitions_PHpwt->get_size());
	modifierHistories_PHpt   = _new NgramScoreTableGen<4>(initial_table_size_smaller);
	
}

void 
ModifierProbDeriver::derive_tables_for_estimation()
{
	modifierHistories_PHpwt->reset();
	modifierHistories_PHpt->reset();
	
	//NgramScoreTableGen::Table::iterator iter;
	float history_count;
	float unique_count;
	Symbol ngram[7];

	for (NgramScoreTableGen<5>::Table::iterator iter = storage_PHpwt->get_start(); iter != storage_PHpwt->get_end(); ++iter) {
		history_count = (*iter).second;
		ngram[0] = (*iter).first[0]; //P
		ngram[1] = (*iter).first[1]; //H
		ngram[2] = (*iter).first[2]; //prev
		ngram[3] = (*iter).first[3]; //hw
		ngram[4] = (*iter).first[4]; //ht
		unique_count = uniqueModifierTransitions_PHpwt->lookup(ngram);
		modifierHistories_PHpwt->add((*iter).first,
			history_count / (history_count + unique_multiplier_PHpwt * unique_count));
	}

	for (NgramScoreTableGen<4>::Table::iterator iter = storage_PHpt->get_start(); iter != storage_PHpt->get_end(); ++iter) {
		history_count = (*iter).second;
		ngram[0] = (*iter).first[0]; //P
		ngram[1] = (*iter).first[1]; //H
		ngram[2] = (*iter).first[2]; //prev
		ngram[3] = (*iter).first[3]; //ht
		unique_count = uniqueModifierTransitions_PHpt->lookup(ngram);
		modifierHistories_PHpt->add((*iter).first,
			history_count / (history_count + unique_multiplier_PHpt * unique_count));
	}
}
