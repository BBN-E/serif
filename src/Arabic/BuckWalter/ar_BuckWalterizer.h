// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_BUCKWALTERIZER_H
#define AR_BUCKWALTERIZER_H

#include "Generic/common/Symbol.h"
#include "Generic/theories/Lexicon.h"
#include "Generic/common/limits.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Arabic/BuckWalter/ar_BuckWalterLimits.h"

class LexicalEntry;
class Lexicon;
class BWRuleDictionary;

/** The maximum number of morphs for a LexicalEntry: */
#define MAXIMUM_MORPH_SEGMENTS 10

class BuckWalterizer {

private:
	typedef struct {
        LexicalEntry* prefix;
		LexicalEntry* stem;
		LexicalEntry* suffix;
		Symbol stem_symbol;
		Symbol original;
	} entry;


	static char _message[1000];
	static wchar_t _normalized_input[1000];
	static wchar_t _prefixBuffer[MAXIMUM_PREFIX_LENGTH];
	static wchar_t _stemBuffer[MAXIMUM_STEM_LENGTH];
	static wchar_t _suffixBuffer[MAXIMUM_SUFFIX_LENGTH];
	static LexicalEntry* _entryChart[MAXIMUM_MORPH_SEGMENTS][MAXIMUM_MORPH_ANALYSES];
	//static UTF8OutputStream _debug_stream;

	//These arrays note results found prior to pruning
	static entry _solutionChartStems[MAXIMUM_STEMMED_SOLUTIONS];
	static int _n_stem_solutions;
	static entry _solutionChartNoStems[MAXIMUM_UNSTEMMED_SOLUTIONS];
	static int _n_nostem_solutions;
	static UTF8OutputStream  uos2;
	
	static LexicalEntry* _temp_results[MAXIMUM_MORPH_ANALYSES];	
	static void splitWord(wchar_t* source, wchar_t* prefix, 
								wchar_t* stem, wchar_t* suffix, 
								size_t prefix_length, size_t suffix_length);
	static void zero();
	static void zeroCharts();
	static int processResults(LexicalEntry** results, int max_results, Lexicon* lex, BWRuleDictionary* rule_dict);
	static void  resolveEntryChart(size_t n_prefixes, size_t n_stems, size_t n_suffixes, 
		Symbol stem, Symbol original, BWRuleDictionary* rule_dict);
	static size_t getPossibleStemCategories(LexicalEntry* prefix, LexicalEntry* suffix, Symbol* category_array, BWRuleDictionary* rule_dict);
	static bool isArabicWord(const wchar_t* input);

public:
	static int analyze(const wchar_t* input, LexicalEntry** result, int max_results, Lexicon* lex, BWRuleDictionary* rule_dict);
};
#endif
