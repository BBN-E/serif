// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_INTERESTING_CONTEXTS_FEATURE_TYPE_H
#define D_T_INTERESTING_CONTEXTS_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"

#include <boost/foreach.hpp>

class IdFInterestingContextsFeatureType : public PIdFFeatureType {

private:

	// Should we case-normalize words in this word list?
	bool _is_lower_case;

	//Maps of contexts -> expected entity type
	std::map<std::vector<Symbol>, Symbol> _prefixes;
	std::map<std::vector<Symbol>, Symbol> _suffixes;

	typedef std::pair<std::vector<Symbol>,Symbol> map_item;

	// Words to leave as they are when we're worrying about title case
	std::set<Symbol> _stopwords;

	void initializeMaps() {
		//Stopwords
		const Symbol stopwords[] = {Symbol(L"of"), Symbol(L"the"), Symbol(L"for"), Symbol(L"a"), Symbol(L"an")};
		BOOST_FOREACH (Symbol s, stopwords) {
			_stopwords.insert(s);
		}

		const Symbol ORG_SYMBOL = Symbol(L"ORG");
		const Symbol GPE_SYMBOL = Symbol(L"GPE");

		//ORG prefixes
		Symbol COLLEGE_OF[] = {Symbol(L"College"),Symbol(L"of")};
		Symbol UNIV_OF[] = {Symbol(L"University"),Symbol(L"of")};
		Symbol INST_OF[] = {Symbol(L"Institute"),Symbol(L"of")};
		Symbol INST_FOR[] = {Symbol(L"Institute"),Symbol(L"for")};
		Symbol DEPT_OF[] = {Symbol(L"Department"),Symbol(L"of")};
		Symbol DEPT_FOR[] = {Symbol(L"Department"),Symbol(L"for")};
		Symbol CHURCH_OF[] = {Symbol(L"Church"),Symbol(L"of")};
		Symbol TEMPLE_PRE[] = {Symbol(L"Temple")};
		Symbol MOSQUE_OF[] = {Symbol(L"Mosque"),Symbol(L"of")};
		Symbol BANK_OF[] = {Symbol(L"Bank"),Symbol(L"of")};

		_prefixes[std::vector<Symbol>(COLLEGE_OF, COLLEGE_OF+2)] = ORG_SYMBOL;
		_prefixes[std::vector<Symbol>(UNIV_OF, UNIV_OF+2)] = ORG_SYMBOL;
		_prefixes[std::vector<Symbol>(INST_OF, INST_OF+2)] = ORG_SYMBOL;
		_prefixes[std::vector<Symbol>(INST_FOR, INST_FOR+2)] = ORG_SYMBOL;
		_prefixes[std::vector<Symbol>(DEPT_OF, DEPT_OF+2)] = ORG_SYMBOL;
		_prefixes[std::vector<Symbol>(DEPT_FOR, DEPT_FOR+2)] = ORG_SYMBOL;
		_prefixes[std::vector<Symbol>(CHURCH_OF, CHURCH_OF+2)] = ORG_SYMBOL;
		_prefixes[std::vector<Symbol>(TEMPLE_PRE, TEMPLE_PRE+1)] = ORG_SYMBOL;
		_prefixes[std::vector<Symbol>(MOSQUE_OF, MOSQUE_OF+2)] = ORG_SYMBOL;
		_prefixes[std::vector<Symbol>(BANK_OF, BANK_OF+2)] = ORG_SYMBOL;

		//GPE Prefixes
		Symbol CITY_OF[] = {Symbol(L"city"),Symbol(L"of")};
		Symbol TOWN_OF[] = {Symbol(L"town"),Symbol(L"of")};

		_prefixes[std::vector<Symbol>(CITY_OF, CITY_OF+2)] = GPE_SYMBOL;
		_prefixes[std::vector<Symbol>(TOWN_OF, TOWN_OF+2)] = GPE_SYMBOL;

		//ORG Suffixes
		Symbol HIGH_SCHOOL[] = {Symbol(L"High"),Symbol(L"School")};
		Symbol MIDDLE_SCHOOL[] = {Symbol(L"Middle"),Symbol(L"School")};
		Symbol COLLEGE[] = {Symbol(L"College")};
		Symbol UNIVERSITY[] = {Symbol(L"University")};
		Symbol SCHOOL[] = {Symbol(L"School")};
		Symbol INSTITUTE[] = {Symbol(L"Institute")};
		Symbol CHURCH[] = {Symbol(L"Church")};
		Symbol TEMPLE[] = {Symbol(L"Temple")};
		Symbol SYNAGOGUE[] = {Symbol(L"Synagogue")};
		Symbol MOSQUE[] = {Symbol(L"Mosque")};
		Symbol BANK[] = {Symbol(L"Bank")};

		_suffixes[std::vector<Symbol>(HIGH_SCHOOL, HIGH_SCHOOL+2)] = ORG_SYMBOL;
		_suffixes[std::vector<Symbol>(MIDDLE_SCHOOL, MIDDLE_SCHOOL+2)] = ORG_SYMBOL;
		_suffixes[std::vector<Symbol>(COLLEGE, COLLEGE+1)] = ORG_SYMBOL;
		_suffixes[std::vector<Symbol>(UNIVERSITY, UNIVERSITY+1)] = ORG_SYMBOL;
		_suffixes[std::vector<Symbol>(SCHOOL, SCHOOL+1)] = ORG_SYMBOL;
		_suffixes[std::vector<Symbol>(INSTITUTE, INSTITUTE+1)] = ORG_SYMBOL;
		_suffixes[std::vector<Symbol>(CHURCH, CHURCH+1)] = ORG_SYMBOL;
		_suffixes[std::vector<Symbol>(TEMPLE, TEMPLE+1)] = ORG_SYMBOL;
		_suffixes[std::vector<Symbol>(SYNAGOGUE, SYNAGOGUE+1)] = ORG_SYMBOL;
		_suffixes[std::vector<Symbol>(MOSQUE, MOSQUE+1)] = ORG_SYMBOL;
		_suffixes[std::vector<Symbol>(BANK, BANK+1)] = ORG_SYMBOL;

		//GPE Suffixes
		Symbol DASH_BASED[] = {Symbol(L"-based")};

		_suffixes[std::vector<Symbol>(DASH_BASED, DASH_BASED+1)] = GPE_SYMBOL;
	}


public:
	IdFInterestingContextsFeatureType() : PIdFFeatureType(Symbol(L"special-context"), InfoSource::OBSERVATION) {
		initializeMaps();
		_is_lower_case = false;
	}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	/*
	* Returns a collection of Trigram Features of the form [Observed tag, Feature type, Entity type associated with context.
	* A reasonable modification would be to have multiple prefix/suffix lists per entity type an futher break down
	* the ORG/GPE sysmbol associated with each vector.
	*/
	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		//NOTE: The first and last elements of the state are NULL (sentence begin/end).
		//Keep that in mind when it comes to checking length for prefix/suffix features.
		int num_results=0;
		std::locale loc("C");

		TokenObservation *o = static_cast<TokenObservation*>(state.getObservation(state.getIndex()));
		Symbol word = o->getSymbol();
		if (_is_lower_case) word = SymbolUtilities::lowercaseSymbol(word);

		std::set<Symbol> word_on_list_match;

		//Collect Title Case Prefix
		std::vector<Symbol> titleLeft;
		for (int i = state.getIndex(); i > 0; --i) {
			Symbol s = (static_cast<TokenObservation*>(state.getObservation(i)))->getSymbol();
			if (s != SymbolConstants::nullSymbol) {
				if (_stopwords.find(s) != _stopwords.end()) {
					//Only take stopwords if the preceding word is title case
					Symbol s2 = (static_cast<TokenObservation*>(state.getObservation(i-1)))->getSymbol();
					if (s2 != SymbolConstants::nullSymbol && std::isupper(s2.to_string()[0],loc)) {
						titleLeft.insert(titleLeft.begin(), s);
					}
				} else if (std::isupper(s.to_string()[0],loc)) {
					if (_is_lower_case) s = SymbolUtilities::lowercaseSymbol(s);
					titleLeft.insert(titleLeft.begin(), s);
				}
			} else {
				break;
			}
		}

		//Prefix features
		std::set<Symbol> all_caps_begin_match_state;
		std::set<Symbol> after_begin_match_state;
		std::set<Symbol> all_caps_after_begin_match_state;

		//Find prefix-based features
		BOOST_FOREACH (map_item prefix_pair, _prefixes){

			std::vector<Symbol> prefix = prefix_pair.first;

			bool all_caps_begin_match = true;
			bool after_begin_match = true;
			bool all_caps_after_begin_match = true;

			//AllCapsBegin
			if (titleLeft.size() < prefix.size() + 1) {
				all_caps_begin_match = false;
			}
			//AfterBegin
			if ((unsigned int)(state.getIndex()) < prefix.size() + 1) {
				after_begin_match = false;
			}
			//AllCapsAfterBegin
			if ((unsigned int)(state.getIndex()) < prefix.size() + titleLeft.size()) {
				all_caps_after_begin_match = false;
			}

			for (size_t i = 0; i < prefix.size(); ++i) {

				//WordOnList
				if (_stopwords.find(word) == _stopwords.end() && word == prefix.at(i)) {
					word_on_list_match.insert(prefix_pair.second);
				}

				//AllCapsBegin
				if (all_caps_begin_match && titleLeft.at(i) != prefix.at(i)) {
					all_caps_begin_match = false;
				}

				//AfterBegin
				if (after_begin_match) {
					size_t offset = state.getIndex() - prefix.size() + i;
					Symbol s = static_cast<TokenObservation*>(state.getObservation(static_cast<int>(offset)))->getSymbol();
					if (_is_lower_case) s = SymbolUtilities::lowercaseSymbol(s);
					if (s != prefix.at(i)) {
						after_begin_match = false;
					}
				}

				//AllCapsAfterBegin
				if (all_caps_after_begin_match) {
					size_t offset = state.getIndex() - titleLeft.size() - prefix.size() + i + 1;
					Symbol s = static_cast<TokenObservation*>(state.getObservation(static_cast<int>(offset)))->getSymbol();
					if (_is_lower_case) s = SymbolUtilities::lowercaseSymbol(s);
					if (s != prefix.at(i)) {
						all_caps_after_begin_match = false;
					}
				}
			}

			if (all_caps_begin_match) all_caps_begin_match_state.insert(prefix_pair.second);
			if (after_begin_match) after_begin_match_state.insert(prefix_pair.second);
			if (all_caps_after_begin_match) all_caps_after_begin_match_state.insert(prefix_pair.second);
		}
		//Make a set of features that get built up and then spat out
		BOOST_FOREACH (Symbol label, all_caps_begin_match_state) {
			resultArray[num_results++] = _new DTTrigramFeature(this, state.getTag(), Symbol(L"AllCapsBegin"), label);
		}
		BOOST_FOREACH (Symbol label, after_begin_match_state) {
			resultArray[num_results++] = _new DTTrigramFeature(this, state.getTag(), Symbol(L"AfterBegin"), label);
		}
		/*BOOST_FOREACH (Symbol label, all_caps_after_begin_match_state) {
			resultArray[num_results++] = _new DTTrigramFeature(this, state.getTag(), Symbol(L"AllCapsAfterBegin"), label);
		}*/

		//Collect Title Case suffix
		std::vector<Symbol> titleRight;
		for (int i = state.getIndex(); i < state.getNObservations() - 1; ++i) {
			Symbol s = (static_cast<TokenObservation*>(state.getObservation(i)))->getSymbol();
			if (s != SymbolConstants::nullSymbol)
			{
				if (_stopwords.find(s) != _stopwords.end()) {
					//Only take stopwords if the next word is title case
					Symbol s2 = (static_cast<TokenObservation*>(state.getObservation(i+1)))->getSymbol();
					if (s2 != SymbolConstants::nullSymbol && std::isupper(s2.to_string()[0],loc)) {
						titleRight.push_back(s);
					}
				} else if (std::isupper(s.to_string()[0],loc)) {
					if (_is_lower_case) s = SymbolUtilities::lowercaseSymbol(s);
					titleRight.push_back(s);
				}
			} else {
				break;
			}
		}

		//TheContext
		int the_index = -1;
		for (int i = 1; i < 4 && state.getIndex()-i > 0; ++i) {
			Symbol s = static_cast<TokenObservation*>(state.getObservation(state.getIndex()-i))->getSymbol();
			if (SymbolUtilities::lowercaseSymbol(s) == Symbol(L"the")) {
				the_index = i;
				break;
			}
		}

		//Suffix features
		std::set<Symbol> all_caps_end_match_state;
		std::set<Symbol> before_end_match_state;
		std::set<Symbol> all_caps_before_end_match_state;
		std::set<Symbol> the_context_match_state;

		//Find suffix-based features
		BOOST_FOREACH (map_item suffix_pair, _suffixes){

			std::vector<Symbol> suffix = suffix_pair.first;

			bool all_caps_end_match = true;
			bool before_end_match = true;
			bool all_caps_before_end_match = true;
			bool the_context_match = the_index > 0 ? true : false;
			int the_context_offset = 0;

			//AllCapsEnd
			if (titleRight.size() < suffix.size() + 1) {
				all_caps_end_match = false;
			}
			//BeforeEnd
			if ((size_t)(state.getIndex() + suffix.size() + 2) > (size_t)(state.getNObservations())) {
				before_end_match = false;
			}
			//AllCapsBeforeEnd
			if ((size_t)(state.getIndex() + titleRight.size() + suffix.size() + 2) > (size_t)(state.getNObservations())) {
				all_caps_before_end_match = false;
			}

			for (size_t i = 0; i < suffix.size(); ++i) {

				//WordOnList
				if (_stopwords.find(word) == _stopwords.end() && word == suffix.at(i)) {
					word_on_list_match.insert(suffix_pair.second);
				}

				//AllCapsEnd
				if (all_caps_end_match && titleRight.at(titleRight.size()-suffix.size()+i) != suffix.at(i)) {
					all_caps_end_match = false;
				}

				//BeforeEnd
				if (before_end_match) {
					Symbol s = static_cast<TokenObservation*>(state.getObservation(static_cast<int>(state.getIndex()+i+1)))->getSymbol();
					if (_is_lower_case) s = SymbolUtilities::lowercaseSymbol(s);
					if (s != suffix.at(i)) {
						before_end_match = false;
					}
				}

				//AllCapsBeforeEnd
				if (all_caps_before_end_match) {
					size_t offset = state.getIndex() + titleRight.size() + i;
					Symbol s = static_cast<TokenObservation*>(state.getObservation(static_cast<int>(offset)))->getSymbol();
					if (_is_lower_case) s = SymbolUtilities::lowercaseSymbol(s);
					if (s != suffix.at(i)) {
						all_caps_before_end_match = false;
					}
				}

				//TheContext
				if (the_context_match && the_index > 0) {
					if (the_context_offset == 0) { //first pass
						//Find the proposed starting location of this suffix ahead of the current index
						bool found_index = false; //SVN disallows breaking anywhere inside BOOST_FOREACH; workaround
						for (int j = 1; j <= 4-the_index &&
										(unsigned int)(state.getNObservations()) >
													   state.getIndex() + j + suffix.size(); ++j)
						{
							Symbol m = static_cast<TokenObservation*>(state.getObservation(state.getIndex()+j))->getSymbol();
							if (_is_lower_case) m = SymbolUtilities::lowercaseSymbol(m);
							if (!found_index && m == suffix.at(i)) {
								the_context_offset = j;
								found_index = true;
							}
						}
						if (the_context_offset == 0) { //Coundn't find this suffix in range
							the_context_offset = -1;
							the_context_match = false;
						}
					  //If we found a possible start for this suffix, pursue that as much as possible
					} else if (the_context_offset > 0) {
						Symbol m = static_cast<TokenObservation*>(state.getObservation(static_cast<int>(state.getIndex()+the_context_offset+i)))->getSymbol();
						if (_is_lower_case) m = SymbolUtilities::lowercaseSymbol(m);
						if (m != suffix.at(i)) {
							the_context_match = false;
							the_context_offset = -1;
						}
					}
				}
			}

			if (all_caps_end_match) all_caps_end_match_state.insert(suffix_pair.second);
			if (before_end_match) before_end_match_state.insert(suffix_pair.second);
			if (all_caps_before_end_match) all_caps_before_end_match_state.insert(suffix_pair.second);
			if (the_context_match) the_context_match_state.insert(suffix_pair.second);
		}

		BOOST_FOREACH (Symbol label, all_caps_end_match_state) {
			resultArray[num_results++] = _new DTTrigramFeature(this, state.getTag(), Symbol(L"AllCapsEnd"), label);
		}
		BOOST_FOREACH (Symbol label, before_end_match_state) {
			resultArray[num_results++] = _new DTTrigramFeature(this, state.getTag(), Symbol(L"BeforeEnd"), label);
		}
		BOOST_FOREACH (Symbol label, the_context_match_state) {
			resultArray[num_results++] = _new DTTrigramFeature(this, state.getTag(), Symbol(L"TheContext"), label);
		}
		/*BOOST_FOREACH (Symbol label, all_caps_before_end_match_state) {
			resultArray[num_results++] = _new DTTrigramFeature(this, state.getTag(), Symbol(L"AllCapsBeforeEnd"), label);
		}*/

		BOOST_FOREACH (Symbol label, word_on_list_match) {
			resultArray[num_results++] = _new DTTrigramFeature(this, state.getTag(), Symbol(L"WordIsOnList"), label);
		}
		
		return num_results;
	}

};

#endif
