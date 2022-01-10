// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Arabic/morphSelection/ar_MorphModel.h"
#include "Arabic/BuckWalter/ar_MorphologicalAnalyzer.h"
#include "Generic/morphAnalysis/SessionLexicon.h"
#include "Generic/theories/LexicalEntry.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/LexicalToken.h"
#include "Arabic/BuckWalter/ar_BuckWalterizer.h"
#include "Arabic/BuckWalter/ar_BWRuleDictionary.h"
#include "Arabic/BuckWalter/ar_Retokenizer.h"
#include "Arabic/BuckWalter/ar_ParseSeeder.h"	

ArabicMorphModel::ArabicMorphModel() : lex(0), rules(0), parseSeeder(0) {
	lex = SessionLexicon::getInstance().getLexicon();
	rules = BWRuleDictionary::getInstance();
	parseSeeder = ParseSeeder::build();
}


ArabicMorphModel::ArabicMorphModel(const char* model_file_prefix) 
					   : MorphModel(model_file_prefix),
						 lex(0), rules(0), parseSeeder(0)
{
	lex = SessionLexicon::getInstance().getLexicon();
	rules = BWRuleDictionary::getInstance();
	parseSeeder = ParseSeeder::build();
}

ArabicMorphModel::~ArabicMorphModel() {
	delete parseSeeder;
	SessionLexicon::destroy();
	Retokenizer::destroy();
	BWRuleDictionary::destroy();
}

//Note: this is a simplification of how word features should be calculated
//it does not use knowledge about the surrounding words this is fine unless 
//the surrounding words are clitics-
//eg. If we see stem wbUNKOWN should be evaluated as having 3 pieces w-b-UNKOWN
//but if we see stem w wbUNKOWN we should evaluate this as having just 1 peice wbUNKNOWN,
//since w is a clitic, the initial token must have read wwbUNKNOWN
Symbol ArabicMorphModel::getTrainingWordFeatures(Symbol word){
	//add calls to buckwalterizer
	const int max_results = 50;
	if (containsDigit(word)) return Symbol(L":NUMBER");
	if (containsASCII(word)) return Symbol(L":NONARABIC");
	LexicalEntry* result[max_results];
	int num_results = BuckWalterizer::analyze(word.to_string(), result, max_results, lex, rules);
	//cheat make a token,
	OffsetGroup start_offset, end_offset;
	start_offset.charOffset = CharOffset(0);
	end_offset.charOffset = CharOffset(static_cast<int>(wcslen(word.to_string()))-1);
	Token* t = _new LexicalToken(start_offset, end_offset, word, 0, num_results, result);
	LocatedString tokenString(t->getSymbol().to_string()); // pass dummy string to ArabicParseSeeder, offsets are never used 
	parseSeeder->processToken(tokenString, t);
	Symbol prefixList[15];
	Symbol suffixList[15];
	int num_prefix = 0;
	int num_suffix = 0;

	for (int i = 0; i < parseSeeder->_n_lex_entries; i++) {
		int numSegs = parseSeeder->_mergedNumSeg[i];
		int seg_num = 0;	
		while (( seg_num<numSegs) && 
			(parseSeeder->_mergedWordChart[seg_num][i].word_part
			!= ArabicParseSeeder::STEM))
		{
			Symbol seg = parseSeeder->_mergedWordChart[seg_num][i].normString;
			bool found = false;
			for (int m = 0; m < num_prefix; m++) {
				if (prefixList[m] == seg) 
					found = true;
			}
			if (!found) {
				prefixList[num_prefix++] = seg;
			}
			seg_num++;
		}
		//skip the stem
		seg_num++;
		while (seg_num < numSegs) {
			Symbol seg = parseSeeder->_mergedWordChart[seg_num][i].normString;
			bool found = false;
			for (int m = 0; m < num_suffix; m++) {
				if (suffixList[m] == seg) 
					found = true;
			}
			if (!found) {
				suffixList[num_suffix++] = seg;
			}
			seg_num++;
		}

	}
	wchar_t unknown_buffer[1000];
	wcscpy(unknown_buffer, L":");
	for (int j = 0; j < num_prefix; j++) {
		wcscat(unknown_buffer,prefixList[j].to_string());
		wcscat(unknown_buffer,L"-");
	}
	wcscat(unknown_buffer, L"STEM-");
	for (int k = 0; k < num_suffix; k++) {
		wcscat(unknown_buffer,suffixList[k].to_string());
		wcscat(unknown_buffer,L"-");
	}
	delete t;
	return Symbol(unknown_buffer);
}

Symbol ArabicMorphModel::getTrainingReducedWordFeatures(Symbol word){
	//add calls to buckwalterizer
	const int max_results = 50;
	if(containsDigit(word)) return Symbol(L":NUMBER");
	if(containsASCII(word)) return Symbol(L":NONARABIC");
	LexicalEntry* result[max_results];
	wchar_t unknown_buffer[100];

	int num_results = BuckWalterizer::analyze(word.to_string(), result, max_results, lex, rules);

	OffsetGroup start_offset, end_offset;
	start_offset.charOffset = CharOffset(0);
	end_offset.charOffset = CharOffset(static_cast<int>(wcslen(word.to_string()))-1);
	Token* t = _new LexicalToken(start_offset, end_offset, word, 0, num_results, result);
	LocatedString tokenString(t->getSymbol().to_string()); // pass dummy string to ArabicParseSeeder, offsets are never used 
	parseSeeder->processToken(tokenString, t);

	int max_num_segs = 0;

	for (int i = 0; i < parseSeeder->_n_lex_entries; i++) {
		int numSegs = parseSeeder->_mergedNumSeg[i];
		if (numSegs > max_num_segs)
			max_num_segs = numSegs;
	}
	swprintf(unknown_buffer, sizeof (unknown_buffer)/sizeof (unknown_buffer[0]), L":%d_segments", max_num_segs);
	delete t;
	return Symbol(unknown_buffer);

}
