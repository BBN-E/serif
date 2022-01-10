// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DT_NAMELIST_FEATURE_TYPE_H
#define DT_NAMELIST_FEATURE_TYPE_H

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

class DTWordNameListFeatureType : public PIdFFeatureType {
	struct WordContext;
	
public:
	DTWordNameListFeatureType(Symbol model, Symbol dict_name, Symbol dict_filename,
							  bool is_lower_case=false, bool use_context=false, bool do_hybrid=false, int offset=0)
		:  PIdFFeatureType(Symbol(getNameWorldListName(dict_name, is_lower_case, use_context, do_hybrid, offset)), InfoSource::OBSERVATION),
		   _wordList() , _is_lower_case(is_lower_case), _use_context(use_context), _do_hybrid(do_hybrid), _offset(offset)
	{
		if (do_hybrid && ParamReader::hasParam("stopwords_for_name_lists")) {
			readStopwords(ParamReader::getParam("stopwords_for_name_lists"));
		}
		readWordList(dict_name.to_string(), dict_filename.to_string());
	}

	virtual DTFeature *makeEmptyFeature() const{
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
									SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state, DTFeature **resultArray) const {
		//Check that we're within an operable range
		if (state.getIndex() + _offset < 0 || state.getIndex() + _offset >= state.getNObservations())
			return 0;
		TokenObservation *o = static_cast<TokenObservation*>(state.getObservation(state.getIndex()+_offset));
		Symbol word = o->getSymbol();
		if (_is_lower_case) word = lc_symbol(word);
		if (_use_context) {
			Symbol::HashMap<std::vector<boost::shared_ptr<WordContext> > >::const_iterator iter = 
				_wordListWithContext.find(word);
			if (iter == _wordListWithContext.end())
				return 0;
			// Collect feature values into a set (to avoid duplicates)
			Symbol::SymbolGroup featureValues;
			for (size_t i = 0; i < (*iter).second.size(); i++) {
				boost::shared_ptr<WordContext> context = (*iter).second[i];
				if (contextMatches(context, state))
					featureValues.insert(context->featureValue);
				if (static_cast<int>(featureValues.size()) >= 
					DTFeatureType::MAX_FEATURES_PER_EXTRACTION)
					break;
			}
			// Use the feature values to populate resultArray.
			int num_results = 0;
			BOOST_FOREACH(Symbol fv, featureValues) {
				resultArray[num_results++] = _new DTBigramFeature(this, state.getTag(), fv);
			}
			return num_results;
		} else if (_do_hybrid) {
			Symbol::HashMap<std::set<Symbol> >::const_iterator iter = _wordListWithVectors.find(word);
			Symbol::SymbolGroup featureValues;
			//Check if the word is in the regular list
			if (iter != _wordListWithVectors.end()) {
				std::set<Symbol>::iterator it;
				for (it = (*iter).second.begin(); it != (*iter).second.end(); ++it) {
					Symbol featureValue = *it;
					featureValues.insert(featureValue);
					if (static_cast<int>(featureValues.size()) >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION)
						break;
				}
			} else { //Next check the stopword (context) list
				bool in_context_list = false;
				Symbol::HashMap<std::vector<boost::shared_ptr<WordContext> > >::const_iterator iter2;
				for (iter2 = _wordListWithContext.begin(); iter2 != _wordListWithContext.end(); ++iter2)
				{
					if ((*iter2).first == word) {
						in_context_list = true;
						break;
					}
				}
				if (in_context_list) { //Check for matching contexts
					for (size_t i = 0; i < (*iter2).second.size(); ++i) {
						boost::shared_ptr<WordContext> context = (*iter2).second[i];
						if (contextMatches(context, state)) {
							featureValues.insert(context->featureValue);
						}
						if (static_cast<int>(featureValues.size()) >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION)
							break;
					}
				} else { //
					return 0;
				}
			}							
			// Use the feature values to populate resultArray.
			int num_results = 0;
			BOOST_FOREACH(Symbol fv, featureValues) {
				resultArray[num_results++] = _new DTBigramFeature(this, state.getTag(), fv);
			}
			return num_results;
		} else {
			Symbol::HashMap<Symbol>::const_iterator iter = _wordList.find(word);
			if (iter == _wordList.end())
				return 0;
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), (*iter).second);
			return 1;
		}
	}

	static wstring getNameWorldListName(const Symbol wlist_name, bool is_lower_case,
										bool use_context, bool do_hybrid, int offset) {
		wstring buffer = L"wl-";
		if (is_lower_case)
			buffer.append(L"lc-");
		if (use_context)
			buffer.append(L"context-");
		else if (do_hybrid)
			buffer.append(L"hybrid-");
		if (offset > 0)
			buffer.append(L"next" + boost::lexical_cast<wstring>(offset) + L"-");
		else if (offset < 0)
			buffer.append(L"prev" + boost::lexical_cast<wstring>(-1 * offset) + L"-");

		buffer.append(wlist_name.to_string());
		return buffer;
	}

 private:
	// Should we case-normalize words in this word list?
	bool _is_lower_case;

	// Should we use context?
	bool _use_context;

	//Should we instead use a hybrid approach? (Context for stopwords)
	bool _do_hybrid;

	//If 0, consider this feature for the current word. Otherwise,
	//look this many tokens forward (+ number) or back (- number)
	int _offset;

	// map from word -> feature, used for old-style word lists.
	// Issues with this approach include: (1) each word can only map
	// to one feature; (2) it does not distinguish between a word that
	// occurs in the context of a name and one that occurs elsewhere,
	// so e.g., if a GPE list contains "(Isle of Man)" then the 
	// wl-gpe=gpe-2 feature will fire whenever it sees the word "of".
	Symbol::HashMap<Symbol> _wordList;

	// map from word -> context, used for new-style word lists.  These
	// features are more restrictive, and only fire if the word
	// appears in the appropriate context for the name.
	struct WordContext {
		std::vector<Symbol> prevWords;
		std::vector<Symbol> nextWords;
		Symbol featureValue;
	};
	Symbol::HashMap<std::vector<boost::shared_ptr<WordContext> > > _wordListWithContext;

	// map from word -> a vector of features, used for "hybrid" word lists.
	// Avoids issue (1) with _wordList above. Does not record context,
	// but the "hybrid" approach should use _wordListWithContext for stopwords.
	// Does not record exact position ("gpe-2"), but rather start, middle, end,
	// and singleton word.
	Symbol::HashMap<std::set<Symbol> > _wordListWithVectors;

	std::set<Symbol> _stopwords;

	Symbol lc_symbol(Symbol sym) const {
		std::wstring word(sym.to_string());
		std::transform(word.begin(), word.end(), word.begin(), towlower);
		return Symbol(word);
	}

	void addEntry(const wchar_t* word_list_name, const std::vector<Symbol> &words) {
		// note: words are already case-normalized if _is_lower_case.
		if (_use_context) {
			for (size_t i=0; i<words.size(); ++i) {
				const Symbol &word = words[i];
				boost::shared_ptr<WordContext> context= boost::make_shared<WordContext>();
				_wordListWithContext[word].push_back(context);
				for (size_t j=0; j<words.size(); ++j) {
					if (j<i) context->prevWords.push_back(words[j]);
					if (j>i) context->nextWords.push_back(words[j]);
				}
				std::wostringstream featureName;
				featureName << word_list_name << L"-" << (i==0?"ST":"CO");
				context->featureValue = Symbol(featureName.str().c_str());
				//std::cout << featureName.str() << ": ";
				//BOOST_FOREACH(Symbol &w, context->prevWords)
				//	std::cout << w << " ";
				//std::cout << "{{" << word << "}}";
				//BOOST_FOREACH(Symbol &w, context->nextWords)
				//	std::cout << " " << w;
				//std::cout << std::endl;
			}
		} else if (_do_hybrid) {
			for (size_t i=0; i<words.size(); ++i) {
				const Symbol &word = words[i];
				std::wostringstream featureName;
				featureName << word_list_name << L"-" << (i==0?(words.size()>1?"ST":"OW"):(i==words.size()-1?"EN":"CO"));
				//If it's a stopword, record context
				if (_stopwords.find(word) != _stopwords.end()) {
					boost::shared_ptr<WordContext> context= boost::make_shared<WordContext>();
					_wordListWithContext[word].push_back(context);
					for (size_t j=0; j<words.size(); ++j) {
						if (j<i) context->prevWords.push_back(words[j]);
						if (j>i) context->nextWords.push_back(words[j]);
					}
					context->featureValue = Symbol(featureName.str().c_str());
					//std::cout << featureName.str() << ": ";
					//BOOST_FOREACH(Symbol &w, context->prevWords)
					//	std::cout << w << " ";
					//std::cout << "{{" << word << "}}";
					//BOOST_FOREACH(Symbol &w, context->nextWords)
					//	std::cout << " " << w;
					//std::cout << std::endl;
				} else {
					_wordListWithVectors[word].insert(Symbol(featureName.str().c_str()));
					//std::cout << featureName.str() << ": ";
					//std::cout << "{{" << word << "}}";
					//std::cout << std::endl;
				}
			}
		} else {
			for (size_t i=0; i<words.size(); ++i) {
				const Symbol &word = words[i];
				std::wostringstream featureName;
				featureName << word_list_name << L"-" << (i+1);
				if (_wordList.get(word) == 0)
					_wordList[word] = Symbol(featureName.str().c_str());
			}
		}
	}

	void readWordList(	const wchar_t* word_list_name, const wchar_t *list_file_name) {
		
		boost::scoped_ptr<UTF8InputStream> listStream_scoped_ptr(UTF8InputStream::build(list_file_name));
		UTF8InputStream& listStream(*listStream_scoped_ptr);
		UTF8Token token;
		std::vector<Symbol> words;
		cerr.flush();
		bool expect_open_paren = true;
		while (!listStream.eof()) {
			listStream >> token;
			if (wcscmp(token.chars(), L"") == 0)
				break;
			Symbol tokSym = token.symValue();
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
					if (_is_lower_case) tokSym = lc_symbol(tokSym);
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

	void readStopwords(const std::string filename) {
		//std::cout << "Loading stopwords from " << filename << "." << std::endl;
		std::set<std::wstring> stopwords = InputUtil::readFileIntoSet(filename, false, true);
		BOOST_FOREACH(std::wstring w, stopwords) {
			_stopwords.insert(Symbol(w));
		}
		//std::cout << "Loaded." << std::endl;
	}

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
		size_t pw_start = index-context->prevWords.size();
		for (size_t i=0; i<context->prevWords.size(); ++i) {
			TokenObservation *o = static_cast<TokenObservation*>(state.getObservation(static_cast<int>(pw_start+i)));
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
			TokenObservation *o = static_cast<TokenObservation*>(state.getObservation(index+static_cast<int>(i)+1));
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

};

#endif
