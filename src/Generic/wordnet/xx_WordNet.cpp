// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#include "Generic/wordnet/xx_WordNet.h"
#include "Generic/common/ParamReader.h"
#include "Generic/parse/xx_STags.h"
#include <boost/foreach.hpp>

#include <assert.h>

// adding time scheme for WordNet
#include "Generic/parse/LanguageSpecificFunctions.h"
#include <boost/scoped_ptr.hpp>

using namespace std;

// specialization needed for stdext::hash_map/has_set on pair<size_t, size_t>
#if defined(_WIN32)
namespace stdext {
	size_t hash_value( const pair< Symbol, int > & p ){
		return hash_value(p.first) ^ hash_value(p.second);
	}
	size_t hash_value( const pair< Symbol, Symbol > & p ){
		return hash_value(p.first) ^ hash_value(p.second);
	}
}
#else
#if defined(__APPLE_CC__)
namespace std {
#else
namespace __gnu_cxx {
#endif
	template<> struct hash<pair< Symbol, int > > {
        size_t operator()( const pair< Symbol, int > & p ) const { 
			return hash<Symbol>()( p.first ) ^ hash<int>()( p.second );
		};
	};
	template<> struct hash<pair< Symbol, Symbol > > {
        size_t operator()( const pair< Symbol, Symbol > & p ) const { 
			return hash<Symbol>()( p.first ) ^ hash<Symbol>()( p.second );
		};
	};
}
#endif

// singleton stuff
WordNet* WordNet::_instance = 0;

WordNet* WordNet::getInstance() {
	if (_instance == 0)
		_instance = _new WordNet();
	return _instance;
}

void WordNet::deleteInstance() {
	delete _instance;
	_instance = 0;
}

WordNet::WordNet() {
	init();
}

namespace {
	const std::set<Symbol> &partitiveWordList() {
		static std::set<Symbol> partitiveWords;
		if (partitiveWords.empty()) {
			partitiveWords.insert(Symbol (L"most"));
			partitiveWords.insert(Symbol (L"many"));
			partitiveWords.insert(Symbol (L"some"));
			partitiveWords.insert(Symbol (L"both"));
			partitiveWords.insert(Symbol (L"several"));
			partitiveWords.insert(Symbol (L"few"));
			partitiveWords.insert(Symbol (L"part"));
			partitiveWords.insert(Symbol (L"parts"));
		}
		return partitiveWords;
	}
}

void WordNet::init() {

	_nlookup = 0;
	_nfirst = 0;
	_nmult = 0;
	
	std::string data_path = ParamReader::getRequiredParam("word_net_dictionary_path");
	if(wninit(data_path.c_str()) != 0){
		/* MRF: General hacks for allowing WordNet to be installed locally on the WinSGE queue machines:
		** WordNet lookups require disk access. As a result, if WordNet is accessed over the network, SERIF slows to a crawl. 
		** This is particularly problematic if there are a large number of simultaneous SERIF jobs. 
		** We have installed WordNet locally on the queue machines but maintain a back-up location, that is a standard SERIF path, 
		** so that the same parameter file can be used for non-bbn deliveries. 
		*/
		//MRF 6-2009: Very Hacky, on the WinSGE queue machines, Wordnet can appear in e:\, c:\, or d:\.  Try all three. 
		//If we implement localization of all of the SERIF models outside of SERIF, this hack could be removed
		bool wordnet_initialized = false;
		#if defined(_WIN32)
		const int n_drives = 3;
		char drive_names[n_drives] = {'c', 'd', 'e'};
		std::string alt_data_path = data_path;
		for(int d_no = 0; d_no < n_drives && !wordnet_initialized; d_no++){			
			alt_data_path[0] = drive_names[d_no];
			if(wninit(alt_data_path.c_str()) == 0){
				SessionLogger::info("init_wn_0")<<"Initializing WordNet from an alternative drive: "<<alt_data_path<<std::endl;
				wordnet_initialized = true;
				data_path = alt_data_path;
			}
		}			
		#endif
		//MRF: an older hack, allow a secondary location for WordNet in case the preferred (local) location is not available.  
		//Could also be removed if all of the models are localized
		if(!wordnet_initialized){
			SessionLogger::warn("backup_wordnet_location")<<"Initializing WordNet from the backup location"<<std::endl;
			data_path = ParamReader::getRequiredParam("word_net_backup_dictionary_path");
			if (wninit(data_path.c_str()) != 0) {
				throw UnexpectedInputException("WordNet::WordNet()",
					"Error in WordNet initialization (parameter 'word_net_backup_dictionary_path'): wninit");
			}
		}
		//MRF: End various WordNet localization hacks
	}
	if (morphinit(data_path.c_str()) != 0) {
		throw UnexpectedInputException("WordNet::WordNet()",
			"Error in WordNet initialization: morphinit");
	}
	personWords_firstSense = _new SymbolBoolHash();
	personWords_allSenses = _new SymbolBoolHash();
	locationWords = _new SymbolBoolHash();
	partitiveWords = _new SymbolBoolHash();

	nounStems = _new SymbolSymbolHash();
	verbStems = _new SymbolSymbolHash();
	nthNounHypernymClassHash = _new SymbolIntToIntHash();
	nthVerbHypernymClassHash = _new SymbolIntToIntHash();
	nthOtherHypernymClassHash = _new SymbolIntToIntHash();
	
	// initialize new hash tables for speeding up WordNet access
	hypernymClassHash_firstSense = _new SymbolToSymbolArrayIntHash();
	hypernymClassHash_allSenses = _new SymbolToSymbolArrayIntHash();
	hyponymClassHash_firstSense = _new SymbolSymbolToBoolHash();
	hyponymClassHash_allSenses = _new SymbolSymbolToBoolHash();
	wordnetWords = _new SymbolBoolHash();
	
	nounHypernymOffsetHash = _new SymbolToIntArrayIntHash();
	verbHypernymOffsetHash = _new SymbolToIntArrayIntHash();

	/*
	nounExceptionListHash = _new SymbolStringHash();
	verbExceptionListHash = _new SymbolStringHash();
	adjExceptionListHash = _new SymbolStringHash();
	advExceptionListHash = _new SymbolStringHash();
	*/

	// get the wordnet word list from an existing file
	// Note: this isn't currently used for anything!
	std::string wordnetWordListFile = ParamReader::getParam("wordnet_word_list");
	if (!wordnetWordListFile.empty()) {
		boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& uis(*uis_scoped_ptr);
		uis.open(wordnetWordListFile.c_str());
		UTF8Token word;
		UTF8Token label;
		while(!uis.eof()){
			uis >> word;
			(* wordnetWords )[word.symValue()] = true;
		}
	}
	// end of new hash tables initialization
	BOOST_FOREACH(const Symbol &word, partitiveWordList()) {
		(*partitiveWords)[word] = true;
	}

	NN = Symbol(L"NN");
	NNS = Symbol(L"NNS");
	NNP = Symbol(L"NNP");
	
	// for debug
	//temp_out1.open("\\\\traid04\\u0\\users\\jchen\\Serif-Regression\\debug\\debugIsHyponym_2.txt");
}

void WordNet::cleanup() {
	// free synsets from the LRU cache, so they don't become false positives during leak detection
	for( synset_lru_list_type::iterator lru_it = synset_lru_list.begin(); lru_it != synset_lru_list.end(); lru_it++ )
		free_syns( lru_it->second );
	synset_lru_list.clear();

	personWords_firstSense->clear();
	personWords_allSenses->clear();
	locationWords->clear();
	nounStems->clear();
	verbStems->clear();
	nthNounHypernymClassHash->clear();
	nthVerbHypernymClassHash->clear();
	nthOtherHypernymClassHash->clear();
	hypernymClassHash_firstSense->clear();
	hypernymClassHash_allSenses->clear();
	hyponymClassHash_firstSense->clear();
	hyponymClassHash_allSenses->clear();
	nounHypernymOffsetHash->clear();
	verbHypernymOffsetHash->clear();

	// Reset the partativeWords list: remove anything we've added *after*
	// the constructor.
	SymbolBoolHash::iterator it=partitiveWords->begin();
	std::set<Symbol> wordList = partitiveWordList();
	while (it != partitiveWords->end()) {
		std::pair<Symbol, bool> entry = *it;
		SymbolBoolHash::iterator next=it; ++next; // erase invalidates the iterator.
		if (wordList.find(entry.first) == wordList.end())
			partitiveWords->erase(it);
		it=next;
	}

	// Currently, wordnetWords is initialized in the constructor with a few words
	// and then never modified (or even used).  So leave it as it is.  (In particular,
	// we don't clear() it.)
}

WordNet::~WordNet() {
	// free synsets from the LRU cache, so they don't become false positives during leak detection
	for( synset_lru_list_type::iterator lru_it = synset_lru_list.begin(); lru_it != synset_lru_list.end(); lru_it++ )
		free_syns( lru_it->second );

	delete personWords_firstSense;
	delete personWords_allSenses;
	delete locationWords;
	delete partitiveWords;
	delete nounStems;
	delete verbStems;
	delete nthNounHypernymClassHash;
	delete nthVerbHypernymClassHash;
	delete nthOtherHypernymClassHash;
	delete wordnetWords;

	// for debug purpose
	/*cerr << "keeping track of hyponym Hashtables !!\n";

	SymbolSymbolToBoolHash::iterator iter;
	UTF8OutputStream temp_out;
	temp_out.open("\\\\traid04\\u0\\users\\jchen\\Serif-Regression\\debug\\debugIsHyponym.txt");
	temp_out << "Hyponym Hash info \n" ;
	iter = hyponymClassHash->begin();
	while (iter != hyponymClassHash->end()) {
		temp_out << (* iter).first.word << " -- " << (* iter).first.proposedHypernym << " -- " << (* iter).second << "\n";
		++iter;
	}


	SymbolToSymbolArrayIntHash::iterator iter2;
	iter2 = hypernymClassHash->begin();
	while (iter2 != hypernymClassHash->end()) {
		temp_out << (* iter2).first << " --  hypernyms \n";
		for (int i=0; i<(* iter2).second.num; i++){
			temp_out << "\t\t" << (* iter2).second.words[i] << "\n";
		}
		temp_out << "\n\n";
		++iter2;
	}
	temp_out.close();
	temp_out1.close(); 
	*/

	// delete new hash tables
	delete hypernymClassHash_firstSense;
	delete hypernymClassHash_allSenses;
	delete hyponymClassHash_firstSense;
	delete hyponymClassHash_allSenses;
	delete nounHypernymOffsetHash;
	delete verbHypernymOffsetHash;
}

Symbol WordNet::stem_noun(Symbol word) const
{
#ifdef DO_WORDNET_PROFILING
	hashCheckinginNounStemTimer.startTimer();
	hashCheckinginNounStemTimer.increaseCount();
#endif
	
	SymbolSymbolHash::iterator iter;
	iter = nounStems->find(word);
	
#ifdef DO_WORDNET_PROFILING
	hashCheckinginNounStemTimer.stopTimer();
#endif

	if (iter != nounStems->end()){
		return (*iter).second;
	} else {
#ifdef DO_WORDNET_PROFILING
		steminNounStemTimer.startTimer();
		steminNounStemTimer.increaseCount();
#endif
	
		Symbol st = stem(word, getpos("n"));
	
#ifdef DO_WORDNET_PROFILING
		steminNounStemTimer.stopTimer();
#endif	

#ifdef DO_WORDNET_PROFILING
		hashInsertioninNounStemTimer.startTimer();
		hashInsertioninNounStemTimer.increaseCount();
#endif

		(*nounStems)[word] = st;

#ifdef DO_WORDNET_PROFILING
		hashInsertioninNounStemTimer.stopTimer();
#endif

		return st;
	}
}

Symbol WordNet::stem_verb(Symbol word) const
{
	SymbolSymbolHash::iterator iter;
	iter = verbStems->find(word);
	if (iter != verbStems->end()){
		return (*iter).second;
	} else {
#ifdef DO_WORDNET_PROFILING
		stemTimer.startTimer();
#endif

		Symbol st = stem(word, getpos("v"));
		
#ifdef DO_WORDNET_PROFILING
		stemTimer.stopTimer();
#endif
		
		(*verbStems)[word] = st;
		return st;
	}
}

Symbol WordNet::stem(Symbol word, Symbol POS) const
{
	int pos_int = 0;
	if (POS.is_null())
		pos_int = getpos("n");
	else if (POS == enAsGen::EnglishSTagsForWordnet::NN || POS == enAsGen::EnglishSTagsForWordnet::NNS ||
			 POS == enAsGen::EnglishSTagsForWordnet::NNP || POS == enAsGen::EnglishSTagsForWordnet::NNPS)
		//This case was taken from en_LanguageSpecificFunctions::isNPtypePOStag()
		pos_int = getpos("n");
	else if (POS == enAsGen::EnglishSTagsForWordnet::VB || POS == enAsGen::EnglishSTagsForWordnet::VBD ||
			 POS == enAsGen::EnglishSTagsForWordnet::VBG || POS == enAsGen::EnglishSTagsForWordnet::VBN ||
			 POS == enAsGen::EnglishSTagsForWordnet::VBP || POS == enAsGen::EnglishSTagsForWordnet::VBZ) 
	{
		//This case was taken from en_WordNet.cpp
		pos_int = getpos("v");
	} else return word;

	return stem(word, pos_int);
}

Symbol WordNet::stem(Symbol word, int POS) const
{
#ifdef DO_WORDNET_PROFILING
	stemTimer.startTimer();
	stemTimer.increaseCount();
#endif
	//word=Symbol(L":others");
	//	POS=1;
	const wchar_t *wstr = word.to_string();
	size_t length = wcslen(wstr);
	char cstr[WORDBUF];
	int char_count = 0;
	for (size_t i = 0; i < length && char_count < WORDBUF-2; i++) {
		if (wstr[i] == 0x0000)
			break;
		if (wstr[i] <= 0x00ff)
			cstr[char_count++] = (char)wstr[i];
		else {
			cstr[char_count++] = (char)(wstr[i] >> 8);
			cstr[char_count++] = (char)wstr[i];
		}
	}
	cstr[char_count] = '\0';

#ifdef DO_WORDNET_PROFILING
	morphTimer.startTimer();
#endif

	char *morph = morphstr(cstr, POS);

#ifdef DO_WORDNET_PROFILING
	morphTimer.stopTimer();
#endif

	if (morph == 0)
		return word;

	wchar_t wbuffer[WORDBUF];
	length = strlen(morph);
	for (size_t j = 0; j < length; j++) {
		wbuffer[j] = wchar_t(morph[j]);
	}
	wbuffer[length] = L'\0';

#ifdef DO_WORDNET_PROFILING
	stemTimer.stopTimer();
#endif	

	return Symbol(wbuffer);
}

bool WordNet::isPerson(Symbol word, bool use_all_senses) {
	
	SymbolBoolHash::iterator iter;
	
	SymbolBoolHash* personWords = personWords_firstSense;
	if (use_all_senses)
		personWords = personWords_allSenses;

	iter = personWords->find(word);
	if (iter != personWords->end()) {
		//if ((*iter).second)
			//cout << word.to_string() << " ";
		return (*iter).second;
	}
	
	if (isHyponymOf(word,"person", use_all_senses)) {
		(*personWords)[word] = true;
		//cout << word.to_string() << " ";
		return true;
	} else {
		(*personWords)[word] = false;
		return false;
	}
		


}

bool WordNet::isLocation(Symbol word) {
	
	SymbolBoolHash::iterator iter;
	
	iter = locationWords->find(word);
	if (iter != locationWords->end()) {
		//if ((*iter).second)
			//cout << word.to_string() << " ";
		return (*iter).second;
	}
	
	if (isHyponymOf(word,"end")) {
		(*locationWords)[word] = false;
		return false;
	} else if (isHyponymOf(word,"region")) {
		(*locationWords)[word] = true;
		//cout << word.to_string() << " ";
		return true;
	} else {
		(*locationWords)[word] = false;
		return false;
	}
	

}


bool WordNet::isPartitive(ChartEntry *entry)
{
	if (entry->isPreterminal)
		return false;

	if (!entry->rightChild->isPPofSignificantConstit)
		return false;

	if ((entry->bridgeType == BRIDGE_TYPE_KERNEL &&
		entry->kernelOp->branchingDirection == BRANCH_DIRECTION_RIGHT) ||
		(entry->bridgeType == BRIDGE_TYPE_EXTENSION &&
		entry->extensionOp->branchingDirection == BRANCH_DIRECTION_RIGHT)) {

		SymbolBoolHash::iterator iter;
		
		iter = partitiveWords->find(entry->headWord);
		if (iter != partitiveWords->end()) {
			return (*iter).second;
		}
		
		if (isHyponymOf(entry->headWord,"digit")) {
			(*partitiveWords)[entry->headWord] = true;
			//cout << entry->headWord.to_string() << " ";
			return true;
		} else if (isHyponymOf(entry->headWord,"large integer")) {
			(*partitiveWords)[entry->headWord] = true;
			//cout << entry->headWord.to_string() << " ";
			return true;
		} else {
			(*partitiveWords)[entry->headWord] = false;
			return false;
		}
		


	}

	return false;

}

bool WordNet::isNumberWord(Symbol word) {
	if (isHyponymOf(word,"digit"))
		return true;
	else if (isHyponymOf(word,"large integer")) 
		return true;
	else if (isHyponymOf(word,"fraction")) 
		return true;
	else return false;
}

// original version, keep for now
bool WordNet::isInWordnet(Symbol word) {
	Symbol lcWord = lowercase_symbol(word);
	SynsetPtr syn = getFirstSynSet(lcWord, getpos("n"));
	if (syn != 0) {
		free_syns(syn);
		return true;
	}
	syn = getFirstSynSet(lcWord, getpos("v"));
	if (syn != 0) {
		free_syns(syn);
		return true;
	}
	syn = getFirstSynSet(lcWord, getpos("a"));
	if (syn != 0) {
		free_syns(syn);
		return true;
	}
	syn = getFirstSynSet(lcWord, getpos("r"));
	if (syn != 0) {
		free_syns(syn);
		return true;
	}
	return false;
}

// speed up of isInWordnet, need to refine further
/*
bool WordNet::isInWordnet(Symbol word) {
	Symbol lcWord = lowercase_symbol(word);
	SymbolBoolHash::iterator iter;
	
	iter = wordnetWords->find(word);
	if (iter != wordnetWords->end()) {
		return true;
	}
	Symbol stem = stem_noun(lcWord);
	if(stem != lcWord && wordnetWords->find(stem) != wordnetWords->end())
		return true;

	stem = stem_verb(lcWord);
	if(stem != lcWord && wordnetWords->find(stem) != wordnetWords->end())
		return true;

	return false;
}
*/

bool WordNet::isNounInWordnet(Symbol word) {
	Symbol lcWord = lowercase_symbol(word);
	SynsetPtr syn = getFirstSynSet(lcWord, getpos("n"));
	if (syn != 0) {
		free_syns(syn);
		return true;
	}
	return false;
}

bool WordNet::isVerbInWordnet(Symbol word) {
	Symbol lcWord = lowercase_symbol(word);
	SynsetPtr syn = getFirstSynSet(lcWord, getpos("v"));
	if (syn != 0) {
		free_syns(syn);
		return true;
	}
	return false;
}

int WordNet::getNthHypernymClass(Symbol word, int n) {
	return getNthHypernymClass(word, Symbol(), n);
}
	
int WordNet::getNthHypernymClass(Symbol word, Symbol POS, int n) {
#ifdef DO_WORDNET_PROFILING
	getNthHypernymTimer.startTimer();
	getNthHypernymTimer.increaseCount();
#endif

	int pos_int;
	SymbolIntToIntHash* hashedHyps = 0;
	Symbol stem = word;
	if (POS.is_null()){
		pos_int = getpos("n");
		hashedHyps = nthNounHypernymClassHash;
		stem = stem_noun(word);
	}
	else if (POS == enAsGen::EnglishSTagsForWordnet::NN || POS == enAsGen::EnglishSTagsForWordnet::NNS ||
			 POS == enAsGen::EnglishSTagsForWordnet::NNP || POS == enAsGen::EnglishSTagsForWordnet::NNPS)
	{
		//This case was taken from en_LanguageSpecificFunctions::isNPtypePOStag()
		pos_int = getpos("n");
		hashedHyps = nthNounHypernymClassHash;
		stem = stem_noun(word);
	}
	else if (POS == enAsGen::EnglishSTagsForWordnet::VB || POS == enAsGen::EnglishSTagsForWordnet::VBD ||
			 POS == enAsGen::EnglishSTagsForWordnet::VBG || POS == enAsGen::EnglishSTagsForWordnet::VBN ||
			 POS == enAsGen::EnglishSTagsForWordnet::VBP || POS == enAsGen::EnglishSTagsForWordnet::VBZ)
	{
		pos_int = getpos("v");
		stem = stem_verb(word);
		hashedHyps = nthVerbHypernymClassHash;
	} else {
#ifdef DO_WORDNET_PROFILING
			getNthHypernymTimer.stopTimer();
#endif	 	
		return 0;
	}

	if (n < 0) {
#ifdef DO_WORDNET_PROFILING
		getNthHypernymTimer.stopTimer();
#endif	 	
		return 0;
	}

	std::pair< Symbol, int > entry( stem, n );
	//if((_nlookup % 100) == 0){
	//	std::cout <<"lookups: "<<_nlookup<<" new: "<<_nfirst<<" repeat: "<<_nmult<<" ";
	//	if(_nlookup > 0) std::cout<<(double)_nmult/_nlookup;
	//	std::cout<<" table size: "<<static_cast<int>(hashedHyps->size())<<std::endl;
	//}
	_nlookup++;
	if (hashedHyps != 0) {
		SymbolIntToIntHash::iterator iter;
		iter = hashedHyps->find(entry);
		if (iter != hashedHyps->end()) {
			_nmult++;
#ifdef DO_WORDNET_PROFILING
			getNthHypernymTimer.stopTimer();
#endif	 	
			return (*iter).second;
		}
	} else {
		//std::cout<<"hashedHyps = 0 for: "<<stem.to_debug_string()<<std::endl;
	}
	_nfirst++;

	SynsetPtr syn = getFirstSynSet(word,pos_int);	
	if (syn == 0) {
		(*hashedHyps)[entry] = 0; 
#ifdef DO_WORDNET_PROFILING
		getNthHypernymTimer.stopTimer();
#endif	 	
		return 0;
	}

	int level = 0;
	SynsetPtr hyper = syn;
	while (level < n && hyper != 0) {
		hyper = hyper->ptrlist;
		level++;
	}

	if (level < n || hyper == 0) {
		free_syns(syn);
		(*hashedHyps)[entry] = 0; 
#ifdef DO_WORDNET_PROFILING
		getNthHypernymTimer.stopTimer();
#endif	 	
		return 0;
	}

	if (hyper->ptroff == 0) {
		// This condition is a weird bug (I think) that only occurs with a very few
		// words (among them "find" and "exceed"). Rather than try to track it down
		// in the archaic WordNet code, we'll just return 0 if this occurs.
		free_syns(syn);
		(*hashedHyps)[entry] = 0; 
#ifdef DO_WORDNET_PROFILING
		getNthHypernymTimer.stopTimer();
#endif	 	
		return 0;
	}
	
	int offset = hyper->ptroff[0];
	free_syns(syn);
	(*hashedHyps)[entry] = offset; 
#ifdef DO_WORDNET_PROFILING
	getNthHypernymTimer.stopTimer();
#endif	 	
	return offset;
}

// original function
/*
int WordNet::getHypernyms(Symbol word, Symbol *results, int MAX_RESULTS) {	
	SynsetPtr syn = getSynSet(word, getpos("n"));
	if (syn == 0)
		return 0;
	
	SynsetPtr hyper = syn;
	int n = 0;
	//loop through hyper/syn, store symbols in hyper->words[i] in results
	while (hyper != 0) {
		for (int i = 0; i < hyper->wcount; i++) {
			if (n < MAX_RESULTS) {
				char *cstr = hyper->words[i];
				wchar_t wcstr[WORDBUF];
				copychar_towchar(cstr, wcstr, WORDBUF);
				results[n++] = Symbol(wcstr);			
			}
		}
		//fill HypernymHash with all results
		hyper = hyper->ptrlist;
	}

	free_syns(syn);
	return n;

}
*/

// revised version for speeding up
int WordNet::getHypernyms(Symbol word, Symbol *results, int MAX_RESULTS) {
	//HypernymHash   Symbol(word) -> #,Symbol[]

	/*
	if (HypernymHash->find(word)) {
		Symbol[] hashedresults = HypernymHash->find(word);
		foreach result (hashedresults)
			results[n] = result
		return #results
	} else {
	}

	Symbol* hashedresults = _new Symbol[hyper->wcount];
	HypernymHash[word]=hashedresults;
	*/

#ifdef DO_WORDNET_PROFILING
	getHypernymTimer.startTimer();
	getHypernymTimer.increaseCount();
#endif

	// this function always uses the "first sense" hash
	SymbolToSymbolArrayIntHash *hypernymClassHash = hypernymClassHash_firstSense;

	if (MAX_RESULTS > Max_Hypernym_Cached) MAX_RESULTS = Max_Hypernym_Cached;
	int n = 0;
	Symbol stem = stem_noun(word);
	SymbolToSymbolArrayIntHash::iterator iter;
	iter = hypernymClassHash->find(stem);
	if (iter != hypernymClassHash->end()) {
		SymbolArrayInt entry = SymbolArrayInt((*iter).second);
		for (int i = 0; i < entry.num; i++) {
			if (n < MAX_RESULTS) {
				results[n++]=entry.words[i];
			}
		}

#ifdef DO_WORDNET_PROFILING
		getHypernymTimer.stopTimer();
#endif

		return n;
	} else {
		SynsetPtr syn = getFirstSynSet(word, getpos("n"));
		if (syn == 0){
			(* hypernymClassHash)[stem].num=0;
	
#ifdef DO_WORDNET_PROFILING
			getHypernymTimer.stopTimer();
#endif

			return 0;
		}
		
		SynsetPtr hyper = syn;
		int n = 0; 
		//loop through hyper/syn, store symbols in hyper->words[i] in results
		while (hyper != 0) {
			for (int i = 0; i < hyper->wcount; i++) {
				// this is a potential bug !!!
				// correct one should read Max_Hypernym_Cached hypernyms and return an array with length of MAX_RESULTS  
				
				if (n < MAX_RESULTS) {
					char *cstr = hyper->words[i];
					wchar_t wcstr[1000];
					copychar_towchar(cstr, wcstr, 1000);
					results[n] = Symbol(wcstr);
					(* hypernymClassHash)[stem].words[n]=Symbol(wcstr); 
					n++;
				}else{
					(* hypernymClassHash)[stem].num=n; 
					free_syns(syn);
				
#ifdef DO_WORDNET_PROFILING
					getHypernymTimer.stopTimer();
#endif

					return n;
				}
			}
		
			//fill HypernymHash with all results
			hyper = hyper->ptrlist;
		}
		(* hypernymClassHash)[stem].num=n; 
		free_syns(syn);
	
#ifdef DO_WORDNET_PROFILING
		getHypernymTimer.stopTimer();
#endif

		return n;
	}
}

// the implementation of this function is for simulating the original function
// static int fillWordNetOffsets(Symbol word, Symbol pos, int *offsets, int MAX_OFFSETS)
int WordNet::getHypernymOffsets(Symbol word, Symbol POS, int *results, int MAX_RESULTS){
	if (MAX_RESULTS > Max_Hypernym_perPOS_Cached) MAX_RESULTS = Max_Hypernym_perPOS_Cached;
	//word=Symbol(L"founding");
	//POS=enAsGen::EnglishSTagsForWordnet::VBG;
	int pos_int;
	SymbolToIntArrayIntHash* hashedHyps = 0;
	Symbol stem = word;
	if (POS.is_null()) {
		pos_int = getpos("n");
		hashedHyps = nounHypernymOffsetHash;
		stem = stem_noun(word);
	}
	else if (POS == enAsGen::EnglishSTagsForWordnet::NN || POS == enAsGen::EnglishSTagsForWordnet::NNS ||
			 POS == enAsGen::EnglishSTagsForWordnet::NNP || POS == enAsGen::EnglishSTagsForWordnet::NNPS) {
		//This case was taken from en_LanguageSpecificFunctions::isNPtypePOStag()
		pos_int = getpos("n");
		hashedHyps = nounHypernymOffsetHash;
		stem = stem_noun(word);
	}
	else if (POS == enAsGen::EnglishSTagsForWordnet::VB || POS == enAsGen::EnglishSTagsForWordnet::VBD ||
			 POS == enAsGen::EnglishSTagsForWordnet::VBG || POS == enAsGen::EnglishSTagsForWordnet::VBN ||
			 POS == enAsGen::EnglishSTagsForWordnet::VBP || POS == enAsGen::EnglishSTagsForWordnet::VBZ)
	{
		//This case was taken from en_WordNet.cpp
		pos_int = getpos("v");
		stem = stem_verb(word);
		hashedHyps = verbHypernymOffsetHash;
	} else {
		return 0;
	}

	int n = 0;
	SymbolToIntArrayIntHash::iterator iter;
	iter = hashedHyps->find(stem);
	if (iter != hashedHyps->end()) {
		
		IntArrayInt entry = IntArrayInt((*iter).second);
		for (int i = 0; i < entry.num; i++) {
			if (n < MAX_RESULTS) {
				results[n++]=entry.offsets[i];
			}
		}
		return n;
	} else {
		SynsetPtr syn = getFirstSynSet(word, pos_int);
		if (syn == 0){
			(* hashedHyps)[stem].num=0;
			return 0;
		}
	
		int numOfCached = 0;
		SynsetPtr hyper = syn;
		while (numOfCached < Max_Hypernym_perPOS_Cached && hyper != 0) {
			if (hyper->ptroff != 0) {
				int offset = hyper->ptroff[0];
				(*hashedHyps)[stem].offsets[numOfCached]=offset; 
				if (n < MAX_RESULTS){
					results[n++]=offset;
				}
			} else {
				(* hashedHyps)[stem].num=numOfCached; 
				free_syns(syn);
				return n;
			}
			hyper = hyper->ptrlist;
			numOfCached++;
		}
		(* hashedHyps)[stem].num=numOfCached; 
		free_syns(syn);
		return n;
	}
}

SynsetPtr WordNet::getSynSet(Symbol word, int POS, int whichsense) {
#ifdef DO_WORDNET_PROFILING
	getSynsetTimer.startTimer();
	getSynsetTimer.increaseCount();
#endif

	Symbol stem = Symbol();
	char *morph1;
	char cstr[WORDBUF];
	if (POS == getpos("n")) {
		stem = stem_noun(word);
		copywchar_tochar(stem.to_string(), cstr, WORDBUF);
		morph1 = cstr;
	} else if (POS == getpos("v")) {
		stem = stem_verb(word);
		copywchar_tochar(stem.to_string(), cstr, WORDBUF);
		morph1 = cstr;
	} else {
		copywchar_tochar(word.to_string(), cstr, WORDBUF);

#ifdef DO_WORDNET_PROFILING
		morphTimer.startTimer();
#endif

		morph1 = morphstr(cstr, POS);

#ifdef DO_WORDNET_PROFILING
		morphTimer.stopTimer();
#endif

		if (morph1 == 0)
			morph1 = cstr;
	}
	int level = 0;
	SynsetPtr syn = findtheinfo_ds(morph1,POS,-1 * getptrtype("@"),whichsense);

#ifdef DO_WORDNET_PROFILING
	getSynsetTimer.stopTimer();
#endif

	return syn;
}

// original function
/*
bool WordNet::isHyponymOf(Symbol word, char* proposed_hypernym) {	
	SynsetPtr syn = getSynSet(word,getpos("n"));
	SynsetPtr hyper;
	if (syn == 0) 
		return false;
	
	if (matches_wordlist_base(syn, proposed_hypernym)) {
		free_syns(syn);
		return true;
	}
	
	SynsetPtr syn_iter = syn;
	while (syn_iter != 0) {
		hyper = syn_iter->ptrlist;
		while (hyper != 0) {
			int retval = matches_wordlist(hyper, proposed_hypernym, 100);
			if (retval >= 0) {
				free_syns(syn);
				return true;
			}
			hyper = hyper->nextss;
		}
		syn_iter = syn_iter->nextss;
	}
	
	free_syns(syn);
	return false;	
}
*/

// revised version, for speeding up
bool WordNet::isHyponymOf(Symbol word, const char* proposed_hypernym, bool use_all_senses) {
	/*
	//current calls check for
		weapon
		vehicle
		transport
		person
		facility
		location
		organization
		office
		stairs
		dwelling
		entryway
		building
		worker
		leader
		fraction
		end
		region
		digit
		large integer
	*/

	SymbolSymbolToBoolHash * hyponymClassHash = hyponymClassHash_firstSense;
	SymbolToSymbolArrayIntHash * hypernymClassHash = hypernymClassHash_firstSense;
	if (use_all_senses) {
		hyponymClassHash = hyponymClassHash_allSenses;
		hypernymClassHash = hypernymClassHash_allSenses;
	}
	
	//add a hash table (hyponymHash) that is  word, proposed -> bool
	//step1 check hyponymHash, if true return
	//step2 check hypernymHash, if true, add to hyponyHash, return true
	//step3 repeat code below, add to hyponymHash true or false, return true or false
	SymbolSymbolToBoolHash::iterator iter;
	std::pair< Symbol, Symbol> entry;
	entry.first = word;
	wchar_t wcstr[1000];
	copychar_towchar(proposed_hypernym, wcstr, 1000);
	entry.second = Symbol(wcstr);

	iter = hyponymClassHash->find(entry);
	if (iter != hyponymClassHash->end()) {
		return (*iter).second;
	}
	
	Symbol stem = stem_noun(word);
	SymbolToSymbolArrayIntHash::iterator iter2;
	iter2 = hypernymClassHash->find(stem);
	if (iter2 != hypernymClassHash->end()) {
		SymbolArrayInt entry2 = SymbolArrayInt((*iter2).second);
		for (int i = 0; i < entry2.num; i++) {
			if (!strcmp((entry2.words[i]).to_debug_string(), proposed_hypernym)){
				(* hyponymClassHash)[entry]=true;
				
				// for debug purpose
				//temp_out1 << entry.word << "--" << entry.proposedHypernym << "\n";
				//temp_out1 << (entry2.words[i]).to_debug_string() << " matched !\n";
				return true;
			}
		}
	}
	
	SynsetPtr syn = 0;
	if (use_all_senses)
		syn = getAllSynSets(word,getpos("n"));
	else syn = getFirstSynSet(word, getpos("n"));
	SynsetPtr hyper;
	if (syn == 0) {
		(* hyponymClassHash)[entry]=false;
		return false;
	}
	
	SynsetPtr syn_iter = syn;
	while (syn_iter != 0) {

		// check this synset's base word list
		if (matches_wordlist_base(syn_iter, proposed_hypernym)) {
			free_syns(syn);
			(* hyponymClassHash)[entry]=true;
			return true;
		}

		// iterate over all hypernyms
		hyper = syn_iter->ptrlist;
		while (hyper != 0) {
			int retval = matches_wordlist(hyper, proposed_hypernym, 100);
			if (retval >= 0) {
				free_syns(syn);
				(* hyponymClassHash)[entry]=true;
				return true;
			}
			hyper = hyper->nextss;
		}

		// move onto next synset
		syn_iter = syn_iter->nextss;
	}
	
	free_syns(syn);
	(* hyponymClassHash)[entry]=false;
	return false;
	
}

Symbol WordNet::lowercase_symbol(Symbol s) const
{
	wstring str = s.to_string();
	wstring::size_type length = str.length();
    for (unsigned i = 0; i < length; ++i) {
        str[i] = towlower(str[i]);
	}
	return Symbol(str.c_str());
}

bool WordNet::matches_wordlist_base (SynsetPtr synptr, const char *word) {
	for (int i = 0; i < synptr->wcount; i++) {
		if (strcmp(synptr->words[i], word) == 0)
			return true;
	}
	return false;
}

int WordNet::matches_wordlist (SynsetPtr synptr, const char *word, int level) {
	if (matches_wordlist_base(synptr, word))
		return level;
	if (level == 0)
		return -1;

	SynsetPtr next_level = synptr->ptrlist;
	while (next_level != 0) {
		int retval = matches_wordlist(next_level, word, level - 1);
		if (retval >= 0)
			return retval;
		next_level = next_level->nextss;
	}
		
	return -1;
}
void WordNet::copywchar_tochar(const wchar_t* wstr, char* cstr, int max_len){
	size_t length = wcslen(wstr);
	//old code ignored buffer overflow, so just truncate w/o complaining
	if(static_cast<int>(length) > (max_len-1)){
		length = max_len-1;
	}
	int char_count = 0;
	for (size_t i = 0; i < length; i++) {
		if (wstr[i] == 0x0000)
			break;
		if (wstr[i] <= 0x00ff)
			cstr[char_count++] = (char)wstr[i];
		else {
			cstr[char_count++] = (char)(wstr[i] >> 8);
			cstr[char_count++] = (char)wstr[i];
		}
	}
	cstr[char_count] = '\0';
}

void WordNet::copychar_towchar(const char* cstr, wchar_t* wcstr, int max_len){				
	size_t length = strlen(cstr);
	//old code ignored buffer overflow, so just truncate w/o complaining
	if(static_cast<int>(length) > (max_len-1)){
		length = max_len-1;
	}
	int char_count = 0;
	for (size_t i = 0; i < length; i++) {
		if (cstr[i] == 0x0000)
			break;
                //		if (cstr[i] <= 0x00ff)  # always true
                wcstr[char_count++] = (wchar_t)cstr[i];
	}
	wcstr[char_count] = L'\0';
}

int WordNet::getSenseCount(Symbol word, int pos) {
	char cstr[WORDBUF];
	copywchar_tochar(word.to_string(), cstr, WORDBUF);
	return get_sense_count(cstr, pos);
}

/*04.04.06 Michael Levit:
	the following two functions provide a more flexible interface to WordNet:
	1) getSynonyms() allows to collect all kinds of relations (synonyms/antonyms/hypernyms)
	from different levels (and different word senses, if needed)
	2) collectNextLevel() is an auxiliary function that enables recursion for getSynonyms()
  03.28.12 Marjorie Freedman
	Stem before getting a synset (wordnet assumes lemmatization). Typically (maybe always?) the stem will be in the results vector.
	This method provides access to word-relations beyond synonyms.  It is used for hypernyms (HYPERPTR), hyponyms(HYPOPTR), and HASPARTPTR.
	The relations *are not* mutually exclusive, e.g. synonyms will also be returned for hypernyms
	

*/
int WordNet::getSynonyms(const Symbol& word, WordNet::CollectedSynonyms& results, int pos, int ptrType, int level, int sense) {

	results.clear();
	SynsetPtr syn = 0;
	Symbol stem = WordNet::stem(word, pos);	
	
	synset_cache_key_type cache_key( stem, pos, ptrType );
	// first check the cache
	synset_cache_type::iterator cit = synset_cache.find( cache_key );
	if( cit != synset_cache.end() ){
		
		pair< synset_cache_key_type, SynsetPtr > entry = (*(*cit).second);
		
		// move the entry to the lru list head
		synset_lru_list.erase( (*cit).second );
		synset_lru_list.push_front( entry );
		// update the cached list iterator
		(*cit).second = synset_lru_list.begin();
		
		syn = entry.second;
		
	} else {
		// not found-- do a wordnet lookup
		
		char cstr[WORDBUF];
		copywchar_tochar(stem.to_string(), cstr, WORDBUF);
		syn = findtheinfo_ds(cstr, pos, ptrType, sense);
		
		pair< synset_cache_key_type, SynsetPtr > entry = make_pair( cache_key, syn );
		
		// add the entry to the lru list, and to the cache. Insertion should never fail.
		synset_lru_list.push_front( entry );
		synset_cache.insert( make_pair( cache_key, synset_lru_list.begin() ) );
		
		if( synset_lru_list.size() > (size_t)WORDNET_CACHE_SIZE ){
			// free the LRU entry
			free_syns( synset_lru_list.back().second );
			synset_cache.erase( synset_lru_list.back().first );
			synset_lru_list.pop_back();
		}
	}

	if (syn == 0) return 0;

	int n=0;
	SynsetPtr synnext=syn;
	do {
		n += collectNextLevel(synnext, level, results);
		synnext = synnext->nextss;
	} while ( synnext != 0 );
	
	return n;
}

//only new words are added; scores for those already in the 'results' are not changed!!!
int WordNet::collectNextLevel(SynsetPtr synptr, int level, WordNet::CollectedSynonyms& results) {
	for (int i = 0; i < synptr->wcount; i++) {
		wchar_t wcstr[WORDBUF];
		copychar_towchar(synptr->words[i], wcstr, WORDBUF);
		size_t j=0;
		while ( wcstr[j] != '\0' ) {
			if ( wcstr[j] == '_' ) 
				wcstr[j] = ' ';
			j++;
		}
		if ( results.find(Symbol(wcstr)) == results.end() )
			results[Symbol(wcstr)] = level;
	}
	int n=synptr->wcount;
	if (level == 0) return n;

	SynsetPtr next_level = synptr->ptrlist;
	while (next_level != 0) {
		n += collectNextLevel(next_level, level-1, results);
		next_level = next_level->nextss;
	}
	return n;
}

/*
int WordNet::isNLevelsOff(char * s1, char * s2, int n) {

	SynsetPtr syn = findtheinfo_ds(s1,getpos(pos),-1 * getptrtype("@"),ALLSENSES);
	SynsetPtr hyper;
		
	if (syn == 0)
		return -1;

	if (matches_wordlist_base(syn, s2))
		return 0;

	SynsetPtr syn_iter = syn;
	while (syn_iter != 0) {
		hyper = syn_iter->ptrlist;
		while (hyper != 0) {
			int retval = matches_wordlist(hyper, s2, n - 1);
			if (retval >= 0) {
				free_syns(syn);
				return n - retval;
			}
			hyper = hyper->nextss;
		}
		syn_iter = syn_iter->nextss;
	}

	free_syns(syn);
	
	syn = findtheinfo_ds(s2,getpos(pos),-1 * getptrtype("@"),ALLSENSES);
	
	syn_iter = syn;
	while (syn_iter != 0) {
		hyper = syn_iter->ptrlist;
		while (hyper != 0) {
			int retval = matches_wordlist(hyper, s1, n - 1);
			if (retval >= 0) {
				free_syns(syn);
				return n - retval;
			}
			hyper = hyper->nextss;
		}
		syn_iter = syn_iter->nextss;
	}

	free_syns(syn);

	return -1;

}

int WordNet::getNumLevelsOff(char * s1, char * s2, char * pos_string, int n) {

	char morph1[WORDBUF];
	char *temp = morphstr(s1,getpos(pos_string));
	if (temp == 0)
		strcpy(morph1, s1);
	else strcpy(morph1, temp);
	char morph2[WORDBUF];
	temp = morphstr(s2,getpos(pos_string));
	if (temp == 0)
		strcpy(morph2, s2);
	else strcpy(morph2, temp);
	return getNumLevelsOffNoMorph(morph1,morph2,pos_string,n);
}

int WordNet::getNumLevelsOffNoMorph(char * s1, char * s2, char * pos_string, int n) {

	pos = pos_string;
	return isNLevelsOff(s1, s2, n);
}
*/

// print timing information
#ifdef DO_WORDNET_PROFILING
void WordNet::printTrace() {
	cout << "WordNet Process Time:" << endl;
	cout << "stem\t" << stemTimer.getTime() << " msec" << endl;
	cout << "stem\t" << stemTimer.getCount() << " times" << endl;
	cout << "stem in stem_noun()\t" << steminNounStemTimer.getTime() << " msec" << endl;
	cout << "stem in stem_noun()\t" << steminNounStemTimer.getCount() << " times" << endl;
	cout << "hash checking in stem_noun()\t" << hashCheckinginNounStemTimer.getTime() << " msec" << endl;
	cout << "hash checking in stem_noun()\t" << hashCheckinginNounStemTimer.getCount() << " times" << endl;
	cout << "hash inserting in stem_noun()\t" << hashInsertioninNounStemTimer.getTime() << " msec" << endl;
	cout << "hash inserting in stem_noun()\t" << hashInsertioninNounStemTimer.getCount() << " times" << endl;
	cout << "getSynset\t" << getSynsetTimer.getTime() << " msec" << endl;
	cout << "getSynset\t" << getSynsetTimer.getCount() << " times" << endl;
	cout << "getHypernym\t" << getHypernymTimer.getTime() << " msec" << endl;
	cout << "getHypernym\t" << getHypernymTimer.getCount() << " times" << endl;
	cout << "getNthHypernym\t" << getNthHypernymTimer.getTime() << " msec" << endl;
	cout << "getNthHypernym\t" << getNthHypernymTimer.getCount() << " times" << endl;
	cout << "morph\t" << morphTimer.getTime() << " msec" << endl;
}
#endif
