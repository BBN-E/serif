// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/actors/AWAKEDB.h"
#include "Generic/actors/ActorTokenSubsetTrees.h"
#include "Generic/xdoc/TokenSubsetTrees.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/ParamReader.h"
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp> 
#include <boost/make_shared.hpp>
#include <iostream>


ActorTokenSubsetTrees::ActorTokenSubsetTrees(ActorInfo_ptr actorInfo, ActorEntityScorer_ptr aes)
{	
	_actorEntityScorer = aes;

	if (ParamReader::isParamTrue("limited_actor_match"))
		return;

	BOOST_FOREACH(ActorPattern *ap, actorInfo->getPatterns()) {
		if (ap->acronym || ap->requires_context)
			continue;

		BOOST_FOREACH(Symbol token, ap->lcPattern) {
			if (_actorNameCache.find(token) == _actorNameCache.end())
				_actorNameCache[token] = std::vector<ActorPattern *>();
			_actorNameCache[token].push_back(ap);
		}
	}

	// Remove caches for tokens that are too common; we don't want to match against these unless they share some other
	// common token anyway. These range from 'the' and 'of' to slightly more common things like 'democratic'.
	_too_frequent_token_threshold = ParamReader::getOptionalIntParamWithDefaultValue("tst_too_frequent_token_threshold", 1000);	
}

ActorTokenSubsetTrees::~ActorTokenSubsetTrees() { }

ActorTokenSubsetTrees::ActorScoreMap ActorTokenSubsetTrees::getTSTEqNames(const Mention *mention) {
	std::vector<Symbol> syms = mention->getHead()->getTerminalSymbols();
	std::vector<std::wstring> allNames;
	boost::unordered_map<std::wstring, ActorId> actorName2ActorIdMap;
	ActorScoreMap results;

	std::wstring mentionName = ActorPattern::getNameFromSymbolList(syms);
	std::transform(mentionName.begin(), mentionName.end(), mentionName.begin(), towlower);
	std::wstringstream wss;
	wss << mentionName << L"_" << mention->getEntityType().getName().to_string();
	std::wstring cacheKey = wss.str();

	if (_mentionNameCache.find(cacheKey) != _mentionNameCache.end())
		return _mentionNameCache[cacheKey];

	std::vector<std::wstring> lcNameWords;
	boost::split(lcNameWords, mentionName, boost::is_any_of(L" "));

	// Gather names for organizing into TokenSubsetTrees
	for (size_t i = 0; i < lcNameWords.size(); i++) {
		std::wstring word = lcNameWords[i];
		if (_actorNameCache.find(Symbol(word)) != _actorNameCache.end()) {

			if (_actorNameCache[word].size() > _too_frequent_token_threshold)
				continue;

			BOOST_FOREACH(ActorPattern *ap, _actorNameCache[word]) {
				if (ap->entityTypeSymbol == mention->getEntityType().getName()) {
					allNames.push_back(ap->lcString);
					actorName2ActorIdMap[ap->lcString] = ap->actor_id;
				}
			}
		}
	}
	allNames.push_back(mentionName);

	TokenSubsetTrees tst;
	tst.initializeTrees(allNames);

	std::vector<std::wstring> eqNames1 = tst.getTSTAliases(mentionName);
	BOOST_FOREACH(std::wstring eqName, eqNames1) {
		ActorId aid = actorName2ActorIdMap[eqName];
		double score = _actorEntityScorer->getTSTEditDistanceEquivalent(mention->getEntityType());
		if (mention->getEntityType().matchesPER() && closePersonTSTMatch(mentionName, eqName))
			score = 0.97;
		if (results.find(aid) == results.end() || score > results[aid])
			results[aid] = score;
	}

/*	std::vector<std::wstring> eqNames2 = tst.getTSTOneCharChildren(mentionName);
	BOOST_FOREACH(std::wstring eqName, eqNames2) {
		ActorId aid = actorName2ActorIdMap[eqName];
		std::cout << "For mention: " << UnicodeUtil::toUTF8StdString(mention->getHead()->toFlatString())
			<< " " << mention->getEntityType().getName().to_debug_string() <<  "\n";
		std::cout << "TST One char child/parent match: " << aid.getId() << "\n";

		if (results.find(aid) == results.end())
			results[aid] = 0.7;
	}
	
	std::vector<std::wstring> eqNames3 = tst.getEditDistTSTChildren(mentionName);
	BOOST_FOREACH(std::wstring eqName, eqNames3) {
		ActorId aid = actorName2ActorIdMap[eqName];
		std::cout << "For mention: " << UnicodeUtil::toUTF8StdString(mention->getHead()->toFlatString())
			<< " " << mention->getEntityType().getName().to_debug_string() <<  "\n";
		std::cout << "TST edit distance match match: " << aid.getId() << "\n";

		if (results.find(aid) == results.end())
			results[aid] = 0.6;
	} */

	if (_mentionNameCache.size() > ATST_MAX_ENTRIES)
		_mentionNameCache.clear();

	_mentionNameCache[cacheKey] = results;
	return results;
}


bool ActorTokenSubsetTrees::closePersonTSTMatch(std::wstring name1, std::wstring name2) {
	std::vector<std::wstring> name1Pieces;
	std::vector<std::wstring> name2Pieces;
	boost::split(name1Pieces, name1, boost::is_any_of(L" "));
	boost::split(name2Pieces, name2, boost::is_any_of(L" "));
	
	size_t length1 = name1Pieces.size();
	size_t length2 = name2Pieces.size();

	return length1 > 1 && length2 > 1 && name1Pieces[0] == name2Pieces[0] && name1Pieces[length1 - 1] == name2Pieces[length2 - 1];
}
