// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DT_MAX_MATCHING_LIST_FEATURE_TYPE_H
#define DT_MAX_MATCHING_LIST_FEATURE_TYPE_H

#include "Generic/common/InputUtil.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/ParamReader.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"



#include <stdio.h>
#include <map>
#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>

class DTMaxMatchingListFeatureType : public PIdFFeatureType {
	struct WordContext;
	
public:
	DTMaxMatchingListFeatureType(Symbol model, Symbol dict_name, Symbol dict_filename)
		:  PIdFFeatureType(Symbol(dict_name), InfoSource::OBSERVATION),
		   _wordList()
	{
		readWordList(dict_name.to_string(), dict_filename.to_string());
	}

	Symbol getFeatureName() const {
		return featureName;
	}

	bool isMatch(Symbol wordString) const {
		Symbol::HashMap<Symbol>::const_iterator iter = _wordList.find(wordString);
		if (iter != _wordList.end()) // found match
			return true;
		else
			return false;
	}

	virtual DTFeature *makeEmptyFeature() const{
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
									SymbolConstants::nullSymbol);
	}

	/*
	std::wstring getSentence(const DTState &state) {
		std::wostringstream sentenceString;

		for (size_t i=0; i<state.getNObservations(); ++i) {
			TokenObservation *o = static_cast<TokenObservation*>(state.getObservation(i));
			Symbol word = o->getSymbol();
			sentenceString << word.to_string();
		}

		return sentenceString.str().c_str();
	}

	void updateDictionaryMatchingCache(std::vector<DTObservation *> &observations) {
		int MAX_TOKENS_LOOK_AHEAD = 6;
		dictionaryMatchingCache.clear();

		std::vector<std::wstring> tokens;
		for(int i=0; i<observations.size(); i++) {
			TokenObservation *o = static_cast<TokenObservation*>(observations[i]);
			tokens.push_back(o->getSymbol().to_string());
			dictionaryMatchingCache.push_back(false); // set all to be NOT_IN_LIST
		}

		for(int i=0; i<tokens.size(); i++) {
			// longest first
			for(int j=i+MAX_TOKENS_LOOK_AHEAD; j>=i; j--) {
				std::wostringstream wordsstream;
				for(int idx=i; idx<=j; idx++)
					wordsstream << tokens[idx];
				
				Symbol wordString(wordsstream.str().c_str());
				Symbol::HashMap<Symbol>::const_iterator iter = _wordList.find(wordString);
				if (iter != _wordList.end()) { // found match
					for(int idx=i; idx<=j; idx++) {
						dictionaryMatchingCache[idx]=true;
						break;
					}
				}
			}
		}
	}

	virtual int extractFeatures(const DTState &state, DTFeature **resultArray) const {
		// updateDictionaryMatchingCache(state.getObservations());
		// 

		int MAX_TOKENS_WINDOW = 6;
		int max_start_index = state.getIndex()-MAX_TOKENS_WINDOW>=1?state.getIndex()-MAX_TOKENS_WINDOW:1;
		int max_end_index = state.getIndex()+MAX_TOKENS_WINDOW<state.getNObservations()?state.getIndex()+MAX_TOKENS_WINDOW:state.getNObservations();

		std::wostringstream wordsstream; 
		for(int start_index = max_start_index; start_index <= state.getIndex(); start_index++) {
			for(int end_index = state.getIndex(); end_index <= max_end_index; end_index++) {
				wordsstream.clear();
				for(int idx=start_index; idx<=end_index; idx++)
					wordsstream << static_cast<TokenObservation*>(state.getObservation(idx))->getSymbol().to_string();
				Symbol wordString(wordsstream.str().c_str());
				
				Symbol::HashMap<Symbol>::const_iterator iter = _wordList.find(wordString);
				if (iter != _wordList.end()) { // found match
					resultArray[0] = _new DTBigramFeature(this, state.getTag(), featureName);
					return 1;
				}
			}
		}

		return 0;
	}
	*/

	virtual int extractFeatures(const DTState &state, DTFeature **resultArray) const {
		// updateDictionaryMatchingCache(state.getObservations());
		// 

		Symbol featureName = getFeatureName();

		TokenObservation *o = static_cast<TokenObservation*>(state.getObservation(state.getIndex()));

		if(o->isMatch(featureName)) {
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), featureName);
			return 1;
		}

		return 0;
	}

 private:

	// map from word -> feature, used for old-style word lists.
	// Issues with this approach include: (1) each word can only map
	// to one feature; (2) it does not distinguish between a word that
	// occurs in the context of a name and one that occurs elsewhere,
	// so e.g., if a GPE list contains "(Isle of Man)" then the 
	// wl-gpe=gpe-2 feature will fire whenever it sees the word "of".
	Symbol::HashMap<Symbol> _wordList;
	Symbol featureName;

	// cached full sentence
	// std::wstring fullSentenceString;
	std::vector<bool> dictionaryMatchingCache;

	void addEntry(const wchar_t* word_list_name, const std::vector<Symbol> &words) {
		std::wostringstream wordString;
		for (size_t i=0; i<words.size(); ++i) {
			const Symbol &word = words[i];
			wordString << word.to_string(); // this only does Chinese, so no space
		}
		
		Symbol wordSymbol(wordString.str().c_str());

		if (_wordList.get(wordSymbol) == 0)
			_wordList[wordSymbol] = Symbol(word_list_name);
	}

	void readWordList(	const wchar_t* word_list_name, const wchar_t *list_file_name) {
		featureName = Symbol(word_list_name);

		boost::scoped_ptr<UTF8InputStream> listStream_scoped_ptr(UTF8InputStream::build(list_file_name));
		UTF8InputStream& listStream(*listStream_scoped_ptr);
		UTF8Token token;
		std::vector<Symbol> words;
		cerr.flush();
		bool expect_open_paren = true;
		bool is_in_comments = false;
		while (!listStream.eof()) {
			listStream >> token;
			if (wcscmp(token.chars(), L"") == 0)
				break;
			Symbol tokSym = token.symValue();
			
			// skipping comments
			if (token.chars()[0] == '#') {
				is_in_comments = true;
				continue;
			}
			if (tokSym == SymbolConstants::leftParen)
				is_in_comments = false;
			if(is_in_comments)
				continue;
			// end special handling for comments

			if (expect_open_paren) {
				if (tokSym != SymbolConstants::leftParen)
					throw UnexpectedInputException("DTWordNameListFeatureType::readWordList", 
												   "Expected open parenthesis in ",
												   Symbol(list_file_name).to_debug_string());
				expect_open_paren = false; // we got the open paren.
			} else {
				if (tokSym == SymbolConstants::leftParen)
					throw UnexpectedInputException("DTWordNameListFeatureType::readWordList", 
												   "Unexpected open parenthesis in ",
												   Symbol(list_file_name).to_debug_string());
				if (tokSym != SymbolConstants::rightParen) {
					words.push_back(tokSym);
				} else {
					addEntry(word_list_name, words);
					words.clear();
					expect_open_paren = true;
				}
			}
		}
		if (!expect_open_paren) {
			throw UnexpectedInputException("DTWordNameListFeatureType::readWordList", 
										   "Missing final close parenthesis in ",
										   Symbol(list_file_name).to_debug_string());
		}
		listStream.close();
	}

	/*
	bool contextMatches(boost::shared_ptr<WordContext> context, const DTState &state) const {
		int index = state.getIndex();
		if (index < (int)context->prevWords.size()) {
			//std::cout<< "  no room for leftwords" << std::endl;
			return false; // no room for prev words
		}
		if ((index+static_cast<int>(context->nextWords.size())) >= state.getNObservations()) {
			//std::cout<< "  no room for rightwords" << std::endl;
			return false; // no room for next words
		}	
		// Check previous words.
		int pw_start = index-context->prevWords.size();
		for (size_t i=0; i<context->prevWords.size(); ++i) {
			TokenObservation *o = static_cast<TokenObservation*>(state.getObservation(pw_start+i));
			Symbol word = o->getSymbol();
			if (_is_lower_case) word = lc_symbol(word);
			if (word != context->prevWords[i]) {
				//std::cout << "  prev word mismatch: " << word << " vs "
				//          << context->prevWords[i] << std::endl;;
				return false;
			}
		}

		// Check next words.
		for (size_t i=0; i<context->nextWords.size(); ++i) {
			TokenObservation *o = static_cast<TokenObservation*>(state.getObservation(index+i+1));
			Symbol word = o->getSymbol();
			if (_is_lower_case) word = lc_symbol(word);
			if (word != context->nextWords[i]) {
				//std::cout << "  next word mismatch: " << word << " vs " 
				//		  << context->nextWords[i] << std::endl;
				return false;
			}
	    }
		return true;
	}
	*/
};

#endif
