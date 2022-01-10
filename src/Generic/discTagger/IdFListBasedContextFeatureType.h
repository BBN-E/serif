// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef IDF_LIST_CONTEXT_FEATURE_TYPE_H
#define IDF_LIST_CONTEXT_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"

#include <boost/foreach.hpp>

//This is mostly just a feature for targeting situations like "Topeka, Kansas."
class IdFListBasedContextFeatureType : public PIdFFeatureType {
public:

	IdFListBasedContextFeatureType(Symbol dict_name, Symbol list_file, bool is_lowercase) : 
		PIdFFeatureType(Symbol(getFeatureName(dict_name, is_lowercase)), InfoSource::OBSERVATION), _is_lower_case(is_lowercase)
	{
		readWordList(list_file.to_string());
	}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state, DTFeature **resultArray) const {

		int num_results=0;

		TokenObservation *o = static_cast<TokenObservation*>(state.getObservation(state.getIndex()));
		Symbol word = o->getSymbol();
		if (_is_lower_case) word = lc_symbol(word);

		//[ITEM] , [ITEM]: if we're before the comma
		bool is_before_comma = state.getIndex()+1 < state.getNObservations() && 
			static_cast<TokenObservation*>(state.getObservation(state.getIndex()+1))->getSymbol() == Symbol(L",");
		bool is_list_end = false;
		bool precedes_list_item = false;

		if (is_before_comma && _byLast.find(word) != _byLast.end()) {

			//Check that the post-comma sequence at least starts some list item
			int post_comma = state.getIndex() + 2;
			Symbol postSym = static_cast<TokenObservation*>(state.getObservation(post_comma))->getSymbol();
			if (_is_lower_case) postSym = lc_symbol(postSym);
			if (_byFirst.find(postSym) != _byFirst.end()) {

				//Check to see if the word before the comma is the end of a list element (checking context)
				for (size_t ind = 0; ind < _byLast.get(word)->size(); ++ind) {
					std::vector<Symbol> term = _wordList.at(_byLast.get(word)->at(ind));

					bool before_matches = false;

					if (state.getIndex() >= static_cast<int>(term.size())) {
						before_matches = true;
						for (size_t i = 0; i < term.size(); ++i) {
							Symbol s = static_cast<TokenObservation*>(state.getObservation(state.getIndex()-static_cast<int>(i)))->getSymbol();
							if (_is_lower_case) s = lc_symbol(s);
							if (s != term.at(term.size()-i-1)) {
								before_matches = false;
								break;
							}
						}
					}
					if (before_matches) {
						is_list_end = true;
						break;
					}
				}

				//Now check to see if the word(s) after the comma also constitute a list item
				for (size_t ind = 0; ind < _byFirst.get(postSym)->size(); ++ind) {
					std::vector<Symbol> term = _wordList.at(_byFirst.get(postSym)->at(ind));

					bool after_matches = false;

					if (post_comma + term.size() < (size_t)(state.getNObservations())) {
						after_matches = true;
						for (size_t i = 0; i < term.size(); ++i) {
							Symbol s = static_cast<TokenObservation*>(state.getObservation(post_comma+static_cast<int>(i)))->getSymbol();
							if (_is_lower_case) s = lc_symbol(s);
							if (s != term.at(i)) {
								after_matches = false;
								break;
							}
						}
					}
					if (after_matches) {
						precedes_list_item = true;
						break;
					}
				}

				if (is_list_end && precedes_list_item) {
					/*std::wcout << L"BeforeComma: " << word.to_string() << std::endl;
					for (int i=1; i<state.getNObservations()-1; ++i)
						std::wcout << static_cast<TokenObservation*>(state.getObservation(i))->getSymbol().to_string() << L" ";
					std::wcout << std::endl;*/
					resultArray[num_results++] = _new DTBigramFeature(this, state.getTag(), Symbol(L"BeforeComma"));
				}
			}
		}

		//[ITEM] , [ITEM]: if we're after the comma
		bool is_after_comma  = state.getIndex() > 2 && 
			static_cast<TokenObservation*>(state.getObservation(state.getIndex()-1))->getSymbol() == Symbol(L",");
		bool is_list_start = false;
		bool follows_list_item = false;

		if (is_after_comma && _byFirst.find(word) != _byFirst.end()) {

			//Check that the post-comma sequence ends some list item
			size_t pre_comma = state.getIndex() - 2;
			Symbol preSym = static_cast<TokenObservation*>(state.getObservation(static_cast<int>(pre_comma)))->getSymbol();
			if (_is_lower_case) preSym = lc_symbol(preSym);
			if (_byLast.find(preSym) != _byLast.end()) {

				for (size_t ind = 0; ind < _byFirst.get(word)->size(); ++ind) {
					std::vector<Symbol> term = _wordList.at(_byFirst.get(word)->at(ind));

					bool after_matches = false;

					//Check to see if the word after the comma is the start of a list element (checking context)
					if (state.getIndex() + term.size() < (size_t)(state.getNObservations())) {
						after_matches = true;
						for (size_t i = 0; i < term.size(); ++i) {
							Symbol s = static_cast<TokenObservation*>(state.getObservation(state.getIndex()+static_cast<int>(i)))->getSymbol();
							if (_is_lower_case) s = lc_symbol(s);
							if (s != term.at(i)) {
								after_matches = false;
								break;
							}
						}
					}
					if (after_matches) {
						is_list_start = true;
						break;
					}
				}

				for (size_t ind = 0; ind < _byLast.get(preSym)->size(); ++ind) {
					std::vector<Symbol> term = _wordList.at(_byLast.get(preSym)->at(ind));

					bool before_matches = false;

					//Now check to see if the word(s) after the comma also constitute a list item
					size_t pre_comma = state.getIndex() - 2;
					if (pre_comma >= term.size()) {
						before_matches = true;
						for (size_t i = 0; i < term.size(); ++i) {
							Symbol s = static_cast<TokenObservation*>(state.getObservation(static_cast<int>(pre_comma-i)))->getSymbol();
							if (_is_lower_case) s = lc_symbol(s);
							if (s != term.at(term.size()-i-1)) {
								before_matches = false;
								break;
							}
						}
					}

					if (before_matches) {
						follows_list_item = true;
						break;
					}
				}

				if (is_list_start && follows_list_item) {
					/*std::wcout << L"AfterComma: " << word.to_string() << std::endl;
					for (int i=1; i<state.getNObservations()-1; ++i)
						std::wcout << static_cast<TokenObservation*>(state.getObservation(i))->getSymbol().to_string() << L" ";
					std::wcout << std::endl;*/
					resultArray[num_results++] = _new DTBigramFeature(this, state.getTag(), Symbol(L"AfterComma"));
				}
			}
		}

		//[ITEM] , [ITEM]: if we're at the beginning of the first [ITEM]
		if (_byFirst.find(word) != _byFirst.end()) {
			//Check to see if we're at the start of a sequence in our list, leading up to a comma
			for (size_t ind = 0; ind < _byFirst.get(word)->size(); ++ind) {
				std::vector<Symbol> term1 = _wordList.at(_byFirst.get(word)->at(ind));

				bool before_matches = false;

				if (state.getIndex() >= static_cast<int>(term1.size())) {
					before_matches = true;
					for (size_t i = 0; i < term1.size(); ++i) {
						Symbol s = static_cast<TokenObservation*>(state.getObservation(state.getIndex()+static_cast<int>(i)))->getSymbol();
						if (_is_lower_case) s = lc_symbol(s);
						if (s != term1.at(i)) {
							before_matches = false;
							break;
						}
					}
				}

				//Now check the bit after the comma if we were successful
				if (before_matches) {
					int after_term = state.getIndex() + static_cast<int>(term1.size());
					bool is_term_before_comma = after_term+1 < state.getNObservations() && 
						static_cast<TokenObservation*>(state.getObservation(after_term))->getSymbol() == Symbol(L",");

					Symbol postSym;
					if (is_term_before_comma) {
						postSym = static_cast<TokenObservation*>(state.getObservation(after_term+1))->getSymbol();
						if (_is_lower_case) postSym = lc_symbol(postSym);
					}
					else
						postSym = SymbolConstants::nullSymbol;

					bool after_comma_is_list_item = false;

					if (is_term_before_comma && _byFirst.find(postSym) != _byFirst.end()) {
						for (size_t ind = 0; ind < _byFirst.get(postSym)->size(); ++ind) {
							std::vector<Symbol> term2 = _wordList.at(_byFirst.get(postSym)->at(ind));

							bool after_matches = false;

							if (after_term + 1 + term2.size() < (size_t)(state.getNObservations())) {
								after_matches = true;
								for (size_t i = 0; i < term2.size(); ++i) {
									Symbol s = static_cast<TokenObservation*>(state.getObservation(after_term+1+static_cast<int>(i)))->getSymbol();
									if (_is_lower_case) s = lc_symbol(s);
									if (s != term2.at(i)) {
										after_matches = false;
										break;
									}
								}
							}
							if (after_matches) {
								after_comma_is_list_item = true;
								break;
							}
						}
					}

					if (after_comma_is_list_item) {
						/*std::wcout << L"StartBeforeComma: " << word.to_string() << std::endl;
						for (int i=1; i<state.getNObservations()-1; ++i)
							std::wcout << static_cast<TokenObservation*>(state.getObservation(i))->getSymbol().to_string() << L" ";
						std::wcout << std::endl;*/
						resultArray[num_results++] = _new DTBigramFeature(this, state.getTag(), Symbol(L"StartBeforeComma"));
						break;
					}
				}
			}
		}
		
		return num_results;
	}

	static wstring getFeatureName(const Symbol wlist_name, bool is_lower_case) {
		wstring buffer = L"list_context-";
		if (is_lower_case)
			buffer.append(L"lc-");

		buffer.append(wlist_name.to_string());
		return buffer;
	}

private:

	// Should we case-normalize words in this word list?
	bool _is_lower_case;

	std::vector<std::vector<Symbol> > _wordList;

	//Shortcut mappings to prevent the model from running glacially
	Symbol::HashMap<std::vector<int> > _byFirst;
	Symbol::HashMap<std::vector<int> > _byLast;

	Symbol lc_symbol(Symbol sym) const {
		std::wstring word(sym.to_string());
		std::transform(word.begin(), word.end(), word.begin(), towlower);
		return Symbol(word);
	}

	void readWordList(const wchar_t *list_file_name) {
		
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
					_byFirst[words.at(0)].push_back(static_cast<int>(_wordList.size()));
					_byLast[words.at(words.size()-1)].push_back(static_cast<int>(_wordList.size()));
					_wordList.push_back(words);
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

};

#endif
