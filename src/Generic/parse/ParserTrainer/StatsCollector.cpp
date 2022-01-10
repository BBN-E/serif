// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include <string>
#include <iostream>
#include "Generic/parse/ParserTrainer/StatsCollector.h"
#include "Generic/parse/ParseNode.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/Symbol.h"
#include "Generic/parse/ParserTrainer/ChainFinder.h"
#include "Generic/parse/ParserTrainer/TrainerPOS.h"
#include "Generic/common/UTF8OutputStream.h"

StatsCollector::StatsCollector()
{

	priorCounts = _new NgramScoreTable(2, INITIAL_PRIOR_TABLE_SIZE);
	headCounts = _new NgramScoreTable(5, INITIAL_HEAD_TABLE_SIZE);
	premodCounts = _new NgramScoreTable(8, INITIAL_MODIFIER_PRE_TABLE_SIZE);
	postmodCounts = _new NgramScoreTable(8, INITIAL_MODIFIER_POST_TABLE_SIZE);
	leftLexicalCounts = _new NgramScoreTable(8, INITIAL_LEXICAL_LEFT_TABLE_SIZE);
	rightLexicalCounts = _new NgramScoreTable(8, INITIAL_LEXICAL_RIGHT_TABLE_SIZE);
	POS = _new TrainerPOS(INITIAL_POS_TABLE_SIZE);
	wordWordFeatureCounts = _new NgramScoreTable(2, INITIAL_POS_TABLE_SIZE);
	wordFeat = WordFeatures::build();

	nodeCount = 0;

	head_yes = Symbol (L"H1");
	head_no = Symbol (L"H0");
	head_yes_modifier_no = Symbol (L"H1M0");
	head_yes_modifier_yes = Symbol (L"H1M1");
	head_no_modifier_yes = Symbol (L"H0M1");
	head_no_modifier_no = Symbol (L"H0M0");

	// no sequential bigrams will be used
	sequentialBigrams = _new SequentialBigrams();
}

StatsCollector::StatsCollector(const char* sequentialBigramFile)
{
	priorCounts = _new NgramScoreTable(2, INITIAL_PRIOR_TABLE_SIZE);
	headCounts = _new NgramScoreTable(5, INITIAL_HEAD_TABLE_SIZE);
	premodCounts = _new NgramScoreTable(8, INITIAL_MODIFIER_PRE_TABLE_SIZE);
	postmodCounts = _new NgramScoreTable(8, INITIAL_MODIFIER_POST_TABLE_SIZE);
	leftLexicalCounts = _new NgramScoreTable(8, INITIAL_LEXICAL_LEFT_TABLE_SIZE);
	rightLexicalCounts = _new NgramScoreTable(8, INITIAL_LEXICAL_RIGHT_TABLE_SIZE);
	POS = _new TrainerPOS(INITIAL_POS_TABLE_SIZE);

	nodeCount = 0;

	head_yes = Symbol (L"H1");
	head_no = Symbol (L"H0");
	head_yes_modifier_no = Symbol (L"H1M0");
	head_yes_modifier_yes = Symbol (L"H1M1");
	head_no_modifier_yes = Symbol (L"H0M1");
	head_no_modifier_no = Symbol (L"H0M0");
	wordWordFeatureCounts = _new NgramScoreTable(2, INITIAL_POS_TABLE_SIZE);
	wordFeat = WordFeatures::build();



	sequentialBigrams = _new SequentialBigrams(sequentialBigramFile);
}


void StatsCollector::collect (ParseNode* parse)
{
	nodeCount++;

	ParseNode* headNode = parse->headNode;
	Symbol headword = parse->headWordNode->headNode->label;
	Symbol headtag = parse->headWordNode->label;
	bool headisfirst = parse->headWordNode->isFirstWord;
	wstring chain_string = L"";
	Symbol chain_symbol;
	
	Symbol prior_ngram[2];
	
	prior_ngram[0] = parse->label; prior_ngram[1] = headtag;
	priorCounts->add(prior_ngram);
	
	// if preterminal
	if (parse->headNode->headNode == 0) {
		Symbol tag_ngram[2];
		Symbol word_ngram[2];
		
		tag_ngram[0] = parse->headNode->label;
		if (headisfirst)
			tag_ngram[1] = head_yes;
		else tag_ngram[1] = head_no;
		POS->add(tag_ngram, parse->label);
		word_ngram[0] = tag_ngram[0];
		word_ngram[1] = wordFeat->features(word_ngram[0], headisfirst);

		wordWordFeatureCounts->add(word_ngram);
		
		return;
	}
	
	ParseNode* nextHead = ChainFinder::find(headNode, chain_string);
	chain_symbol = Symbol(chain_string.c_str());
	
	Symbol head_ngram[5];
	head_ngram[0] = chain_symbol; head_ngram[1] = parse->label;
	head_ngram[2] = headword; head_ngram[3] = headtag;
	if (headisfirst)
		head_ngram[4] = head_yes;
	else head_ngram[4] = head_no;

	headCounts->add(head_ngram);
	collect(nextHead);

	Symbol previousModifier = Symbol(L":ADJ"); 
	Symbol previousWord = head_ngram[2];
	Symbol previousTag = head_ngram[3];
	bool mod_is_first = headisfirst;
	ParseNode* nextModifier = parse->premods;
	bool use_left_sequential = 
		sequentialBigrams->use_left_sequential_bigrams(parse->label);
	while (nextModifier != 0) {
		if (use_left_sequential) {
			collectModifierStats(leftLexicalCounts, premodCounts, parse->label,
					headNode->label, previousModifier, previousWord, 
					previousTag, mod_is_first, nextModifier);
			
			previousWord = nextModifier->headWordNode->headNode->label;
			previousTag = nextModifier->headWordNode->label;
			mod_is_first = nextModifier->headWordNode->isFirstWord;
		} else {
			collectModifierStats(leftLexicalCounts, premodCounts, parse->label,
					headNode->label, previousModifier, headword, 
					headtag, headisfirst, nextModifier);
		}
		previousModifier = nextModifier->label;
		nextModifier = nextModifier->next;
	}
	Symbol modifier_ngram[8];
	modifier_ngram[0] = Symbol(L":EXIT");   modifier_ngram[1]= Symbol(L":EXIT");
	modifier_ngram[2] = parse->label;     modifier_ngram[3] = headNode->label;
	modifier_ngram[4] = previousModifier; 
	if (headisfirst)
		modifier_ngram[7] = head_yes;
	else modifier_ngram[7] = head_no;

	if (use_left_sequential) {
		modifier_ngram[5] = previousWord; 
		modifier_ngram[6] = previousTag; 
	} else {
		modifier_ngram[5] = headword; 
		modifier_ngram[6] = headtag; 
	}

	premodCounts->add(modifier_ngram);

	previousModifier = Symbol(L":ADJ");
	previousWord = head_ngram[2];
	previousTag = head_ngram[3];
	mod_is_first = headisfirst;
	nextModifier = parse->postmods;
	bool use_right_sequential = 
		sequentialBigrams->use_right_sequential_bigrams(parse->label);
	while (nextModifier != 0) {
		if (use_right_sequential) {
			collectModifierStats(rightLexicalCounts, postmodCounts, parse->label,
				headNode->label, previousModifier, previousWord, previousTag, 
				mod_is_first, nextModifier);
			previousWord = nextModifier->headWordNode->headNode->label;
			previousTag = nextModifier->headWordNode->label;
			mod_is_first = nextModifier->headWordNode->isFirstWord;
		} else {
			collectModifierStats(rightLexicalCounts, postmodCounts, parse->label,
				headNode->label, previousModifier, headword, 
				headtag, headisfirst, nextModifier);
		}

		previousModifier = nextModifier->label;
		nextModifier = nextModifier->next;
	}
	modifier_ngram[0] = Symbol(L":EXIT");  modifier_ngram[1] = Symbol(L":EXIT");
	modifier_ngram[2] = parse->label;     modifier_ngram[3] = headNode->label;
	modifier_ngram[4] = previousModifier; 
	if (headisfirst)
		modifier_ngram[7] = head_yes;
	else modifier_ngram[7] = head_no;
	if (use_right_sequential) {
		modifier_ngram[5] = previousWord; 
		modifier_ngram[6] = previousTag; 
	} else {
		modifier_ngram[5] = headword; 
		modifier_ngram[6] = headtag; 
	}
	postmodCounts->add(modifier_ngram);

}

void StatsCollector::collectModifierStats(NgramScoreTable* LexicalCounts, 
			NgramScoreTable* ModCounts, Symbol P, Symbol H, Symbol previous, 
			Symbol hw, Symbol ht, bool headisfirst, ParseNode* modifier)
{
	Symbol modifierword = modifier->headWordNode->headNode->label;
	Symbol modifiertag = modifier->headWordNode->label;
	wstring chain_string = L"";
	Symbol chain_symbol;
	ParseNode* nextMod;

	nextMod = ChainFinder::find(modifier, chain_string);
	collect(nextMod);
	chain_symbol = Symbol(chain_string.c_str());

	Symbol modifier_ngram[8];
	modifier_ngram[0] = chain_symbol; modifier_ngram[1] = modifiertag;
	modifier_ngram[2] = P;  modifier_ngram[3] = H; modifier_ngram[4] = previous; 
	modifier_ngram[5] = hw; modifier_ngram[6] = ht;
	if (headisfirst)
			modifier_ngram[7] = head_yes;
		else modifier_ngram[7] = head_no;
	ModCounts->add(modifier_ngram);

	Symbol lexical_ngram[8];
	lexical_ngram[0] = modifierword; lexical_ngram[1] = modifier->label;
	lexical_ngram[2] = modifiertag;	lexical_ngram[3] = P;  lexical_ngram[4] = H; 
	lexical_ngram[5] = hw; lexical_ngram[6] = ht;
	if (modifier->headWordNode->isFirstWord) {
		if (headisfirst)
			lexical_ngram[7] = head_yes_modifier_yes;
		else lexical_ngram[7] = head_no_modifier_yes;
	} else {
		if (headisfirst)
			lexical_ngram[7] = head_yes_modifier_no;
		else lexical_ngram[7] = head_no_modifier_no;
	}
	LexicalCounts->add(lexical_ngram);
}



void StatsCollector::print_all(char *p, char *h, char *pre, char *post, 
						   char *ll, char *rl, char *pos)
{
	UTF8OutputStream out;
	out.open(p);
	out << nodeCount << '\n';
	priorCounts->print_to_open_stream(out);
	out.close();

	headCounts->print(h);
	premodCounts->print(pre);
	postmodCounts->print(post);
	leftLexicalCounts->print(ll);
	rightLexicalCounts->print(rl);
	POS->print(pos);
}
void StatsCollector::print_vocab(const char* voc){
	wordWordFeatureCounts->print(voc);
}
	
