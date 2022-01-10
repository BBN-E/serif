// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/AcronymMaker.h"
#include "Generic/edt/xx_AcronymMaker.h"
#include "Generic/common/WordConstants.h"
#include "Generic/common/ParamReader.h"

#include <boost/foreach.hpp>

GenericAcronymMaker::GenericAcronymMaker() {
	// to do: read reservedAcronyms from a data file.
	if (ParamReader::isParamTrue("simple_rule_name_link_distillation")) {
		std::vector<Symbol> united_states;
		std::vector<Symbol> united_nations;
		united_states.push_back(Symbol(L"united"));
		united_states.push_back(Symbol(L"states"));
		united_nations.push_back(Symbol(L"united"));
		united_nations.push_back(Symbol(L"nations"));
		reservedAcronyms[Symbol(L"us")].push_back(united_states);
		reservedAcronyms[Symbol(L"un")].push_back(united_nations);
	}
}

std::vector<Symbol> GenericAcronymMaker::generateAcronyms(const std::vector<Symbol>& words) {
	std::vector<Symbol> results;
	std::vector<Symbol> filteredWords;

	BOOST_FOREACH(Symbol word, words) {
		if (WordConstants::isAcronymStopWord(word))
			continue;
		filteredWords.push_back(word);
	}

	// No single-letter acronyms!
	if (filteredWords.size() < 2)
		return results;
	
	//concatenate the initials
	std::wstring acronym1; // without periods
	std::wstring acronym2; // with periods
	BOOST_FOREACH(Symbol word, filteredWords) {
		const wchar_t w = *word.to_string();	
		acronym1 += w;
		acronym2 += w + std::wstring(L".");
	}
	//finally, the last period may be left off e.g. at end of sentence
	std::wstring acronym3(acronym2.begin(), acronym2.end()-1);

	// Check if this is a "reserved" acronym.  If so, then we only
	// keep it if it matches one of the names it's reserved for.
	ReservedAcronymTable::iterator it = reservedAcronyms.find(Symbol(acronym1.c_str()));
	if (it != reservedAcronyms.end()) {
		bool matches_reserved_acronym = false;
		BOOST_FOREACH(SymbolList reservedAcronym, (*it).second) {
			if (reservedAcronym == words)
				matches_reserved_acronym = true;
		}
		if (!matches_reserved_acronym)
			return results; // return empty result.
	}

	results.push_back(Symbol(acronym1.c_str()));
	results.push_back(Symbol(acronym2.c_str()));
	results.push_back(Symbol(acronym3.c_str()));

	return results;
}

