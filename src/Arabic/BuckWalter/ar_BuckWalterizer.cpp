// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Arabic/BuckWalter/ar_BuckWalterizer.h"
#include "Arabic/BuckWalter/ar_BuckWalterFunctions.h"
#include "Arabic/BuckWalter/ar_BWNormalizer.h"
#include "Arabic/BuckWalter/ar_BWRuleDictionary.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/SessionLogger.h"

char BuckWalterizer::_message[1000];
wchar_t BuckWalterizer::_normalized_input[1000];
wchar_t BuckWalterizer::_prefixBuffer[MAXIMUM_PREFIX_LENGTH];
wchar_t BuckWalterizer::_stemBuffer[MAXIMUM_STEM_LENGTH];
wchar_t BuckWalterizer::_suffixBuffer[MAXIMUM_SUFFIX_LENGTH];
LexicalEntry* BuckWalterizer::_entryChart[MAXIMUM_MORPH_SEGMENTS][MAXIMUM_MORPH_ANALYSES];	
LexicalEntry* BuckWalterizer::_temp_results[MAXIMUM_MORPH_ANALYSES];	
BuckWalterizer::entry BuckWalterizer::_solutionChartStems[MAXIMUM_STEMMED_SOLUTIONS];
BuckWalterizer::entry BuckWalterizer::_solutionChartNoStems[MAXIMUM_UNSTEMMED_SOLUTIONS];

int BuckWalterizer::_n_stem_solutions;
int BuckWalterizer::_n_nostem_solutions;
//UTF8OutputStream BuckWalterizer::_debug_stream;

bool BuckWalterizer::isArabicWord(const wchar_t* input) {
	size_t len = wcslen(input);
	for (size_t i = 0; i < len; i++) {
		if ((input[i] < 1536) || (input[i] > 1791)) {
			return false;
		}
	}
	return true;
}

int BuckWalterizer::analyze(const wchar_t* input, LexicalEntry** result, int max_results, 
							Lexicon* lex, BWRuleDictionary* rule_dict) 
{
	try {
		zero();
		//_debug_stream.open("analyzer.debug.file", false);
		
		//normalize the word
		BWNormalizer::normalize(input, _normalized_input);
		Symbol whole_word = Symbol(_normalized_input);

		Symbol prefix;
		Symbol stem;
		Symbol suffix;
		size_t num_prefixes = 0; 
		size_t num_stems = 0; 
		size_t num_suffixes = 0; 
		int n_results = 0;
		size_t n_temp_results = 0;
		Symbol none;
		if ((whole_word == none) || (whole_word == Symbol(L""))) {
			int nresults = lex->getEntriesByKey(Symbol(L"EMPTY_STRING"), result, max_results);
			return nresults;
		}
		if (!isArabicWord(input)) {
			int nresults = lex->getEntriesByKey(Symbol(L"NON_ARABIC"), result, max_results);
			return nresults;
		}
		//if the word is already in the lexicon analyzed, then return it
		if (lex->hasKey(whole_word)) {	
			n_temp_results = lex->getEntriesByKey(whole_word, _temp_results, MAXIMUM_MORPH_ANALYSES);
			for (size_t res = 0; res < n_temp_results; res++) {
				if (_temp_results[res]->getFeatures()->getCategory() == BuckwalterFunctions::ANALYZED) {
					result[n_results] = BuckWalterizer::_temp_results[res];	
					n_results++;
				}
			}
			if (n_results > 0) {
				return n_results;
			}
		}
		
		//Otherwise do the BuckWalter analysis
		for (size_t prefix_length = 0; prefix_length < MAXIMUM_PREFIX_LENGTH; prefix_length++) {
			for (size_t suffix_length = 0; suffix_length < MAXIMUM_SUFFIX_LENGTH; suffix_length++) {
				//there must be at least one character in the stem
				if ((prefix_length + suffix_length) >= wcslen(_normalized_input)) {
					break;
				} 
				else { 
					
					splitWord(_normalized_input, _prefixBuffer, _stemBuffer, _suffixBuffer, prefix_length, suffix_length);

					prefix = (_prefixBuffer[0] != L'\0') ? Symbol(_prefixBuffer) : Symbol(L"NULL");
					stem = Symbol(_stemBuffer); //the stem must always exist
					suffix = (_suffixBuffer[0] != L'\0') ? Symbol(_suffixBuffer) : Symbol(L"NULL");

					//if the prefix and suffix both exist in Lexicon, then go ahead construct the chart
					if (lex->hasKey(prefix) /*&& lex->hasKey(stem)*/ && lex->hasKey(suffix)) {
						num_prefixes = lex->getEntriesByKey(prefix, _entryChart[0], MAXIMUM_MORPH_ANALYSES);
						num_stems = lex->getEntriesByKey(stem, _entryChart[1], MAXIMUM_MORPH_ANALYSES);
						num_suffixes = lex->getEntriesByKey(suffix, _entryChart[2], MAXIMUM_MORPH_ANALYSES);
					
						//find the paths through the chart which are legal
						resolveEntryChart(num_prefixes, num_stems, num_suffixes, stem, whole_word, rule_dict);
					}
				}
			}
		}
		
		//construct lexical entries for the resulting paths and return
		n_results = processResults(result,max_results, lex, rule_dict);
		if (n_results == 0) {
			FeatureValueStructure* fvs = FeatureValueStructure::build(
					BuckwalterFunctions::ANALYZED, BuckwalterFunctions::NULL_SYMBOL,
					BuckwalterFunctions::NULL_SYMBOL, BuckwalterFunctions::NULL_SYMBOL, true);
				LexicalEntry* le = _new LexicalEntry(lex->getNextID(), Symbol(input) , fvs, NULL, 0);
				lex->addDynamicEntry(le);	
				result[0] = le;
				return 1;
		}
		return n_results;
	} 
	catch(UnrecoverableException e) {
		sprintf(_message, "%s%s%s", e.getSource(), ": ", e.getMessage());  
		throw UnrecoverableException("BuckWalterizer::_analyze", _message);
	}
}

void BuckWalterizer::splitWord(wchar_t* source, wchar_t* prefix, 
								wchar_t* stem, wchar_t* suffix, 
								size_t prefix_length, size_t suffix_length)
{

	size_t source_len = wcslen(source);
	size_t stem_length = source_len - suffix_length - prefix_length;

	//get prefix
	for(size_t i = 0; i < prefix_length; i++){
		prefix[i] = source[i];
	}
	prefix[prefix_length] = L'\0';

	//get stem
	for (size_t j = 0; j < stem_length; j++) {
		stem[j] = source[prefix_length+j];
	}
	stem[stem_length] = L'\0';

	//get suffix
	for (size_t k = 0; k < suffix_length; k++) {
		suffix[k] = source[prefix_length+stem_length+k];
	}
	suffix[suffix_length] = L'\0';
	return;
}

void BuckWalterizer::zero() {
	_normalized_input[0] = L'\0';
	_prefixBuffer[0] = L'\0';
	_stemBuffer[0] = L'\0';
	_suffixBuffer[0]= L'\0';

	_n_stem_solutions = 0;
	_n_nostem_solutions = 0;
	
	zeroCharts();	
	
	size_t n_analyses;
	for (n_analyses = 0; n_analyses < MAXIMUM_MORPH_ANALYSES; n_analyses++) {
			_temp_results[n_analyses] = NULL;
	}
	for (n_analyses = 0; n_analyses < MAXIMUM_STEMMED_SOLUTIONS; n_analyses++) {
		_solutionChartStems[n_analyses].prefix = NULL;	
		_solutionChartStems[n_analyses].stem = NULL;
		_solutionChartStems[n_analyses].suffix = NULL;
	}
	for (n_analyses = 0; n_analyses < MAXIMUM_UNSTEMMED_SOLUTIONS; n_analyses++) {
		_solutionChartNoStems[n_analyses].prefix = NULL;	
		_solutionChartNoStems[n_analyses].stem = NULL;
		_solutionChartNoStems[n_analyses].suffix = NULL;
	}
}

void BuckWalterizer::zeroCharts(){
	for (size_t n_segs = 0; n_segs < MAXIMUM_MORPH_SEGMENTS ; n_segs++) {
		for (size_t n_analyses = 0; n_analyses < MAXIMUM_MORPH_ANALYSES; n_analyses++) {
			_entryChart[n_segs][n_analyses] = NULL;
		}
	}
}

int BuckWalterizer::processResults(LexicalEntry** results,int max_results,
								   Lexicon* lex, BWRuleDictionary* rule_dict) 
{
	int num_total_solutions = 0;
	if (_n_stem_solutions > 0) {
		int max = _n_stem_solutions;
		if (max > max_results) {
			max =  max_results;
			/*
			SessionLogger::logger->beginWarning();
			(*SessionLogger::logger)<<"Skipping Stem Lexical Entries: \n";
			for(int i = max; i < _n_stem_solutions; i++){
				(*SessionLogger::logger)<<_solutionChartStems[i].prefix->toString();
				(*SessionLogger::logger)<<_solutionChartStems[i].stem->toString();
				(*SessionLogger::logger)<<_solutionChartStems[i].suffix->toString();
			}
			*/
		}
		for (int i = 0; i < max; i++) {
			//make the array of sub parts
			_temp_results[0] = _solutionChartStems[i].prefix;
			_temp_results[1] = _solutionChartStems[i].stem;
			_temp_results[2] = _solutionChartStems[i].suffix;

			FeatureValueStructure* fvs = FeatureValueStructure::build(
				BuckwalterFunctions::ANALYZED, BuckwalterFunctions::NULL_SYMBOL,
				BuckwalterFunctions::NULL_SYMBOL, BuckwalterFunctions::NULL_SYMBOL, true);
			LexicalEntry* le = _new LexicalEntry(lex->getNextID(), _solutionChartStems[i].original, fvs, _temp_results, 3);

			lex->addDynamicEntry(le);	
			results[num_total_solutions++] = le;
//`			le->dump(_debug_stream);
		}
	} 
	else {
		/*Note: This loop adds a lexical entry for each stem, category combination
		 *		that is a possibility given A(prefix) and C(suffix)- always at least 
		 *		145 with NULL prefix/suffix.  Consequently the lexicon grows too quickly.
		 *	New Solution: mark unknown stems with category 'UNKNOWN' and generate
		 *		categories from prefix/suffix when they are needed
		 */
		//New Solution
		int max = _n_nostem_solutions;
		if (max > max_results) {
			max =  max_results;
			SessionLogger::LogMessageMaker warning = SessionLogger::logger->reportWarning().with_id("skip_nostem_lex");
			warning << "Skipping NoStem Lexical Entries: \n";
			for(int i = max; i < _n_nostem_solutions; i++){
				warning << _solutionChartNoStems[i].prefix->toString();
				warning << _solutionChartNoStems[i].stem_symbol.to_string()<<"\n";
				warning << _solutionChartNoStems[i].suffix->toString();
			}
		}
		for (int i = 0; i < max; i++) {
			// UKNOWN stem entry could have been added to Lexicon in a previous iteration
			if (lex->hasKey(_solutionChartNoStems[i].stem_symbol)) {
				int n_results = lex->getEntriesByKey(_solutionChartNoStems[i].stem_symbol, _temp_results, MAXIMUM_MORPH_ANALYSES);
				// find the resulting entry whose category == UNKNOWN
				int j = 0;
				while (j < n_results && (_temp_results[j]->getFeatures()->getCategory() != BuckwalterFunctions::UNKNOWN)) {
					j++;
				}
				if (j == n_results) {
					sprintf(_message, "%s", "No existing lexical entry for this key with cateogry UNKNOWN");  
					throw UnrecoverableException("BuckWalterizer::processResults", _message);
				}
				_solutionChartNoStems[i].stem = _temp_results[j];
			}
			else {
				//create the stem lexical entry
				FeatureValueStructure* fvs_stem = FeatureValueStructure::build(BuckwalterFunctions::UNKNOWN, 
					BuckwalterFunctions::NULL_SYMBOL, BuckwalterFunctions::NULL_SYMBOL, 
					BuckwalterFunctions::NULL_SYMBOL, false);	
				LexicalEntry* le_stem = _new LexicalEntry(lex->getNextID(), _solutionChartNoStems[i].stem_symbol, fvs_stem, NULL, 0);
				_solutionChartNoStems[i].stem = le_stem;
				lex->addDynamicEntry(le_stem);	
			}
			_temp_results[0] = _solutionChartNoStems[i].prefix;
			_temp_results[1] = _solutionChartNoStems[i].stem;
			_temp_results[2] = _solutionChartNoStems[i].suffix;

			//make the final lexical entry
			FeatureValueStructure* fvs = FeatureValueStructure::build(BuckwalterFunctions::ANALYZED, 
				BuckwalterFunctions::NULL_SYMBOL, BuckwalterFunctions::NULL_SYMBOL, 
				BuckwalterFunctions::NULL_SYMBOL, true);	
			LexicalEntry* le = _new LexicalEntry(lex->getNextID(),  _solutionChartNoStems[i].original, fvs, _temp_results, 3);
			lex->addDynamicEntry(le);	
			results[num_total_solutions++] = le;
//			le->dump(_debug_stream);
		}
	}
	return num_total_solutions;
}

size_t BuckWalterizer::getPossibleStemCategories(LexicalEntry* prefix, LexicalEntry* suffix, 
												 Symbol* category_array, BWRuleDictionary* rule_dict)
{
	Symbol temp_categories[1000];
	size_t num_candidates = rule_dict->getFollowingPossibilities(Symbol(L"A"), prefix->getFeatures()->getCategory(), temp_categories);
	size_t num_results = 0;
	
	for (size_t i = 0; i < num_candidates; i++) {
		if (rule_dict->isBCPermitted(temp_categories[i], suffix->getFeatures()->getCategory())) {
			category_array[num_results] = temp_categories[i];
			num_results++;	
		}
	}
	return num_results;
}

void BuckWalterizer::resolveEntryChart(size_t n_prefixes, size_t n_stems, size_t n_suffixes, 
										Symbol stem, Symbol original, BWRuleDictionary* rule_dict)
{

	for (size_t prefix_number = 0; prefix_number < n_prefixes; prefix_number++) {

		for (size_t suffix_number = 0; suffix_number < n_suffixes; suffix_number++) {
			
			//check if we have any stems aleady in Lexicon
			if (n_stems > 0) {
				for (size_t stem_number = 0; stem_number < n_stems; stem_number++) {
					if (BuckWalterizer::_entryChart[0][prefix_number] == NULL || 
					   BuckWalterizer::_entryChart[1][stem_number]== NULL || 
					   BuckWalterizer::_entryChart[2][suffix_number]== NULL)
					{
						sprintf(_message, "%s", "Analysis Showing NULL lexical entry");  
						throw UnrecoverableException("BuckWalterizer::resolveEntryChart", _message);
					}
				   
					if (rule_dict->isACPermitted(
						BuckWalterizer::_entryChart[0][prefix_number]->getFeatures()->getCategory(), 
						BuckWalterizer::_entryChart[2][suffix_number]->getFeatures()->getCategory())) 
					{
						// Allow A -Cat=UNKNOWN- C combinations (but add them to nostem chart)
						if (BuckWalterizer::_entryChart[1][stem_number]->getFeatures()->getCategory() == 
							BuckwalterFunctions::UNKNOWN)
						{
							//check to make sure we haven't run out of space for solutions
							if (_n_nostem_solutions > MAXIMUM_UNSTEMMED_SOLUTIONS) {
								sprintf(_message, "%s%d", "Number of possible analyses for stemless words has exceeded maximum allowed, ", MAXIMUM_MORPH_ANALYSES);  
								throw UnrecoverableException("BuckWalterizer::resolveEntryChart", _message);
							}

							_solutionChartNoStems[_n_nostem_solutions].prefix = _entryChart[0][prefix_number];
							_solutionChartNoStems[_n_nostem_solutions].stem = _entryChart[1][stem_number];
							_solutionChartNoStems[_n_nostem_solutions].suffix = _entryChart[2][suffix_number];
							_solutionChartNoStems[_n_nostem_solutions].stem_symbol = _entryChart[1][stem_number]->getKey();
							_solutionChartNoStems[_n_nostem_solutions].original = original;
							_n_nostem_solutions++;
							
						}
						//Alow explicit A-B-C combinations, 
						else if (rule_dict->isABPermitted(
								BuckWalterizer::_entryChart[0][prefix_number]->getFeatures()->getCategory(), 
								BuckWalterizer::_entryChart[1][stem_number]->getFeatures()->getCategory())
							&& 
							rule_dict->isBCPermitted(
								BuckWalterizer::_entryChart[1][stem_number]->getFeatures()->getCategory(),
								BuckWalterizer::_entryChart[2][suffix_number]->getFeatures()->getCategory()))
						{
						
							//check to make sure we haven't run out of space for solutions
							if (_n_stem_solutions > MAXIMUM_STEMMED_SOLUTIONS)
							{
								sprintf(_message, "%s%d", "Number of possible analyses for stemmed words has exceeded maximum allowed, ", MAXIMUM_MORPH_ANALYSES);  
								throw UnrecoverableException("BuckWalterizer::resolveEntryChart", _message);
							}

							_solutionChartStems[_n_stem_solutions].prefix = _entryChart[0][prefix_number];
							_solutionChartStems[_n_stem_solutions].stem = _entryChart[1][stem_number];
							_solutionChartStems[_n_stem_solutions].suffix = _entryChart[2][suffix_number];
							_solutionChartStems[_n_stem_solutions].stem_symbol = _entryChart[1][stem_number]->getKey();
							_solutionChartStems[_n_stem_solutions].original = original;
							_n_stem_solutions++;
						}
					}
				}

			}
			else {
				if (BuckWalterizer::_entryChart[0][prefix_number] == NULL ||
				   BuckWalterizer::_entryChart[2][suffix_number] == NULL)
				{
					sprintf(_message, "%s", "Analysis Showing NULL here lexical entry");  
					throw UnrecoverableException("BuckWalterizer::resolveEntryChart", _message);
				}
				if (rule_dict->isACPermitted(
					BuckWalterizer::_entryChart[0][prefix_number]->getFeatures()->getCategory(), 
					BuckWalterizer::_entryChart[2][suffix_number]->getFeatures()->getCategory())
					)
				{
					//check to make sure we haven't run out of space for solutions
					if (_n_nostem_solutions > MAXIMUM_UNSTEMMED_SOLUTIONS) {
						sprintf(_message, "%s%d", "Number of possible analyses for stemless words has exceeded maximum allowed, ", MAXIMUM_MORPH_ANALYSES);  
						throw UnrecoverableException("BuckWalterizer::resolveEntryChart", _message);
					}

					_solutionChartNoStems[_n_nostem_solutions].prefix = _entryChart[0][prefix_number];
					_solutionChartNoStems[_n_nostem_solutions].stem = NULL;
					_solutionChartNoStems[_n_nostem_solutions].suffix = _entryChart[2][suffix_number];
					_solutionChartNoStems[_n_nostem_solutions].stem_symbol = stem;
					_solutionChartNoStems[_n_nostem_solutions].original = original;
					_n_nostem_solutions++;
				
				}

			}
		}
	}
}								
