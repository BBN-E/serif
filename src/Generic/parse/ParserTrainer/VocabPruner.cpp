// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <cstring>
#include "Generic/common/UTF8InputStream.h"
#include "Generic/parse/ParserTrainer/VocabPruner.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/parse/VocabularyTable.h"
#include "Generic/parse/WordFeatures.h"
#include "math.h"
#include <boost/scoped_ptr.hpp>

// NOTE:
//
// ***keep_all_words == true:
// All words whose counts are lower than the threshold remain in
// the table, but for each event containing a rare word (or two), 
// we store both the original event and the same event with 
// the rare word(s) replaced by their feature vectors. (The same
// is true for the part of speech table.)
//
// All words therefore remain in the vocabulary table -- 
//    print() outputs unprunedVocabularyTable
//
// ***keep_all_words == false:
// All words whose counts are lower than the threshold are pruned.
// For each event containing a rare word (or two), we store the 
// event with the rare word(s) replaced by their feature vectors. 
// (The same is true for the part of speech table.) print() 
// outputs vocabularyTable
//
//
// SMOOTHING:
//
// When keep_all_words == true, the above scheme creates problems 
// when we want to generate pruned events for use in smoothing -- 
// we want one and only one event stored for each actual 
// event in the smoothing corpus. The boolean variable 
// pruning_smoothing_events_with_main_vocab tells us to do 
// exactly that.  We only transform words into feature 
// vectors when they are not found at all in the main corpus vocabulary. 
// pruning_smoothing_events_with_main_vocab is set when
// the program is run with an extra commands line parameter (it doesn't 
// matter what it is; in the training scripts, it is this-is-a-smoothing-run).
// When keep_all_words == false, this parameter is irrelevant.
//


VocabPruner::VocabPruner (char* vocfile, int threshold, bool smooth)
{
	wordFeatures = WordFeatures::build();
	
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);

	in.open(vocfile);
	vocabularyTable = _new VocabularyTable(in, threshold);
	in.close();

	if (smooth)
		pruning_smoothing_events_with_main_vocab = true;
	else pruning_smoothing_events_with_main_vocab = false;

	// SEE NOTE ABOVE
	// this must be changed and the module re-compiled
	// to switch methods of pruning
	keep_all_words = false;

	in.open(vocfile);
	unprunedVocabularyTable = _new VocabularyTable(in, 0);
	in.close();
	
	head_yes = Symbol (L"H1");
	head_no = Symbol (L"H0");
	head_yes_modifier_no = Symbol (L"H1M0");
	head_yes_modifier_yes = Symbol (L"H1M1");
	head_no_modifier_yes = Symbol (L"H0M1");
	head_no_modifier_no = Symbol (L"H0M0");
}

void VocabPruner::print_wordprob(char* word_feat_file, char* outfile){
	NgramScoreTable* allCountsTable = _new NgramScoreTable(1, 70000);
	NgramScoreTable* uniqueFeatureCountsTable = _new NgramScoreTable(1, 700);
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(word_feat_file);
	NgramScoreTable* wordFeatTable = _new NgramScoreTable(2, in);
	in.close();
	NgramScoreTable::Table::iterator iter;
	Symbol ngram[1];
	//each vocab word may occur 2x in the wordFeatTable once where the first word part
	//of the word feature is true, and once where it is false.  
	float num_words = 0;
	for(iter = wordFeatTable->get_start(); iter != wordFeatTable->get_end(); ++iter){
	
		if(vocabularyTable->find((*iter).first[0])){
			ngram[0] = (*iter).first[0];
			allCountsTable->add(ngram, (*iter).second);
			num_words += (*iter).second;
		}
		else{
			//add the actual word
			ngram[0] = (*iter).first[0];
			allCountsTable->add(ngram, (*iter).second);
			num_words += (*iter).second;

			//add the actual feature 
			ngram[0] = (*iter).first[1];
			allCountsTable->add(ngram, (*iter).second);
			//add 1 to the number of unique words for this feature
			uniqueFeatureCountsTable->add(ngram, 1);
		}
	}
	//make the word feature counts avg # words per feature (otherwise generating some rare word will be too likely)
	NgramScoreTable::Table::iterator lookup_iter;
	for(iter = uniqueFeatureCountsTable->get_start(); iter != uniqueFeatureCountsTable->get_end(); ++iter){
		ngram[0] = (*iter).first[0];
		lookup_iter = allCountsTable->get_element(ngram);
		(*lookup_iter).second = ((*lookup_iter).second/(*iter).second) ;
		num_words += (*lookup_iter).second;
	}
	for(iter = allCountsTable->get_start(); iter != allCountsTable->get_end(); ++iter){
		(*iter).second = (float)log(((*iter).second/num_words));
	}
	allCountsTable->print(outfile);
}


	



void VocabPruner::prune_modifiers(char* outfile)
{
	NgramScoreTable* prunedModifierCounts = 
		_new NgramScoreTable (MODIFIER_NGRAM_SIZE, INITIAL_PRUNED_MODIFIER_TABLE_SIZE);

	NgramScoreTable::Table::iterator iter;
	Symbol ngram[MODIFIER_NGRAM_SIZE + 1];

	for (iter = modifierCounts->get_start(); iter != modifierCounts->get_end(); ++iter) {
	
		if (vocabularyTable->find((*iter).first[MODIFIER_HEADWORD_INDEX]))
		{
			prunedModifierCounts->add((*iter).first, (*iter).second);

		} else {
			
			if (keep_all_words) {
				if (pruning_smoothing_events_with_main_vocab) {
					if (unprunedVocabularyTable->find((*iter).first[MODIFIER_HEADWORD_INDEX])) {
						prunedModifierCounts->add((*iter).first, (*iter).second);
						continue;
					}
				} else prunedModifierCounts->add((*iter).first, (*iter).second);
			} 	

			for (int i = 0; i < MODIFIER_NGRAM_SIZE + 1; i++) {
				ngram[i] = (*iter).first[i];
			}
			if (ngram[MODIFIER_NGRAM_SIZE] == head_yes) 
				ngram[MODIFIER_HEADWORD_INDEX] = 
					wordFeatures->features(ngram[MODIFIER_HEADWORD_INDEX], true);
			else ngram[MODIFIER_HEADWORD_INDEX]= 
					wordFeatures->features(ngram[MODIFIER_HEADWORD_INDEX], false);

			prunedModifierCounts->add(ngram, (*iter).second);
		}

	}
		
	prunedModifierCounts->print(outfile);
	
}

void VocabPruner::prune_heads(char* outfile)
{
	NgramScoreTable* prunedHeadCounts = 
		_new NgramScoreTable (HEAD_NGRAM_SIZE, INITIAL_PRUNED_HEAD_TABLE_SIZE);

	NgramScoreTable::Table::iterator iter;
	Symbol ngram[HEAD_NGRAM_SIZE + 1];

	for (iter = headCounts->get_start(); iter != headCounts->get_end(); ++iter) {
	
		if (vocabularyTable->find((*iter).first[HEAD_HEADWORD_INDEX]))
		{
			prunedHeadCounts->add((*iter).first, (*iter).second);

		} else {

			if (keep_all_words) {
				if (pruning_smoothing_events_with_main_vocab) {
					if (unprunedVocabularyTable->find((*iter).first[HEAD_HEADWORD_INDEX])) {
						prunedHeadCounts->add((*iter).first, (*iter).second);
						continue;
					}
				} else prunedHeadCounts->add((*iter).first, (*iter).second);
			} 


			for (int i = 0; i < HEAD_NGRAM_SIZE + 1; i++) {
				ngram[i] = (*iter).first[i];
			}
			if (ngram[HEAD_NGRAM_SIZE] == head_yes) 
				ngram[HEAD_HEADWORD_INDEX] = 
					wordFeatures->features(ngram[HEAD_HEADWORD_INDEX], true);
			else ngram[HEAD_HEADWORD_INDEX]= 
					wordFeatures->features(ngram[HEAD_HEADWORD_INDEX], false);

			prunedHeadCounts->add(ngram, (*iter).second);
		}

	}
		
	prunedHeadCounts->print(outfile);
	
}

void VocabPruner::prune_lexical(char* outfile)
{
	NgramScoreTable* prunedLexicalCounts = 
		_new NgramScoreTable (LEXICAL_NGRAM_SIZE, INITIAL_PRUNED_LEXICAL_TABLE_SIZE);

	NgramScoreTable::Table::iterator iter;
	Symbol ngram[LEXICAL_NGRAM_SIZE + 1];
	bool headword_found;
	bool modifierword_found;

	for (iter = lexicalCounts->get_start(); iter != lexicalCounts->get_end(); ++iter) {
	
		headword_found = vocabularyTable->find((*iter).first[LEXICAL_HEADWORD_INDEX]);
		modifierword_found = vocabularyTable->find((*iter).first[LEXICAL_MODIFIERWORD_INDEX]);

		if (headword_found && modifierword_found) {
			prunedLexicalCounts->add((*iter).first, (*iter).second);
		} else {

			bool headword_found2 = headword_found;
			bool modifierword_found2 = modifierword_found;

			if (keep_all_words) {
				if (pruning_smoothing_events_with_main_vocab) {
		
					// note that if we find a word in the unpruned vocabulary,
					//   it was necessarily in the pruned vocabulary

					headword_found2 =
						unprunedVocabularyTable->find((*iter).first[LEXICAL_HEADWORD_INDEX]);
					modifierword_found2 =
						unprunedVocabularyTable->find((*iter).first[LEXICAL_MODIFIERWORD_INDEX]);
					
					if (headword_found2 && modifierword_found2) {
						prunedLexicalCounts->add((*iter).first, (*iter).second);
						continue;
					}
				} prunedLexicalCounts->add((*iter).first, (*iter).second);
			} 

			for (int i = 0; i < LEXICAL_NGRAM_SIZE + 1; i++) {
					ngram[i] = (*iter).first[i];
			}

			if (headword_found2) {

				if ((ngram[LEXICAL_NGRAM_SIZE] == head_no_modifier_yes) ||
					(ngram[LEXICAL_NGRAM_SIZE] == head_yes_modifier_yes))
					ngram[LEXICAL_MODIFIERWORD_INDEX] 
						= wordFeatures->features(ngram[LEXICAL_MODIFIERWORD_INDEX], true);
				else ngram[LEXICAL_MODIFIERWORD_INDEX]
						= wordFeatures->features(ngram[LEXICAL_MODIFIERWORD_INDEX], false);

			} else if (modifierword_found2) {

				if ((ngram[LEXICAL_NGRAM_SIZE] == head_yes_modifier_no) ||
					(ngram[LEXICAL_NGRAM_SIZE] == head_yes_modifier_yes))
					ngram[LEXICAL_HEADWORD_INDEX] 
						= wordFeatures->features(ngram[LEXICAL_HEADWORD_INDEX], true);
				else ngram[LEXICAL_HEADWORD_INDEX]
						= wordFeatures->features(ngram[LEXICAL_HEADWORD_INDEX], false);

			} else {

				if (ngram[LEXICAL_NGRAM_SIZE] == head_yes_modifier_no) {
					ngram[LEXICAL_HEADWORD_INDEX] 
						= wordFeatures->features(ngram[LEXICAL_HEADWORD_INDEX], true);
					ngram[LEXICAL_MODIFIERWORD_INDEX]
						= wordFeatures->features(ngram[LEXICAL_MODIFIERWORD_INDEX], false);
				} else if (ngram[LEXICAL_NGRAM_SIZE] == head_yes_modifier_yes) {
					ngram[LEXICAL_HEADWORD_INDEX] 
						= wordFeatures->features(ngram[LEXICAL_HEADWORD_INDEX], true);
					ngram[LEXICAL_MODIFIERWORD_INDEX]
						= wordFeatures->features(ngram[LEXICAL_MODIFIERWORD_INDEX], true);
				} else if (ngram[LEXICAL_NGRAM_SIZE] == head_no_modifier_yes) {
					ngram[LEXICAL_HEADWORD_INDEX] 
						= wordFeatures->features(ngram[LEXICAL_HEADWORD_INDEX], false);
					ngram[LEXICAL_MODIFIERWORD_INDEX]
						= wordFeatures->features(ngram[LEXICAL_MODIFIERWORD_INDEX], true);
				} else if (ngram[LEXICAL_NGRAM_SIZE] == head_no_modifier_no) {
					ngram[LEXICAL_HEADWORD_INDEX] 
						= wordFeatures->features(ngram[LEXICAL_HEADWORD_INDEX], false);
					ngram[LEXICAL_MODIFIERWORD_INDEX]
						= wordFeatures->features(ngram[LEXICAL_MODIFIERWORD_INDEX], false);
				}
			}
			
			prunedLexicalCounts->add(ngram, (*iter).second);
			
		}

	}
		
	prunedLexicalCounts->print(outfile);
	
}

