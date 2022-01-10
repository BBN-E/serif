// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Sexp.h"
#include "Generic/actors/AWAKEDB.h"
#include "Generic/actors/ActorEditDistance.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/DocTheory.h"

#include <iostream>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>

namespace {
    // Symbol constants used by _getVowelessEntries:
    Symbol GPE(L"GPE");
	Symbol PER(L"PER");
	Symbol LOC(L"LOC");
	Symbol FAC(L"FAC");
	Symbol ORG(L"ORG");
   
}

ActorEditDistance::ActorEditDistance(ActorInfo_ptr actorInfo) {	
	
	if (ParamReader::isParamTrue("limited_actor_match"))
		return;

	BOOST_FOREACH(ActorPattern *ap, actorInfo->getPatterns()) {

		// Don't allow edit distance on patterns that require context
		if (ap->acronym || ap->requires_context)
			continue;

		if (ap->entityTypeSymbol == PER) 
			_perActors.push_back(ap);
		if (ap->entityTypeSymbol == ORG)
			_orgActors.push_back(ap);
		if (ap->entityTypeSymbol == GPE)
			_gpeActors.push_back(ap);
		if (ap->entityTypeSymbol == LOC)
			_locActors.push_back(ap);
		if (ap->entityTypeSymbol == FAC)
			_facActors.push_back(ap);
				
	}

	// Nicknames
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& stream(*stream_scoped_ptr);
	std::string nicknamesFile = ParamReader::getRequiredParam("nicknames_file");
	stream.open(nicknamesFile.c_str());
	std::string exceptionString = std::string("Malformed nicknames file: ") + nicknamesFile + " referred to by parameter nicknames_file";

	while (!stream.eof()) {
		Sexp *line = _new Sexp(stream, false, true);
		if (line->isVoid())
			break;

		std::set<std::wstring> nicknameSet;
		if (line->getNumChildren() != 2)
			throw UnexpectedInputException("ActorEditDistance::ActorEditDistance", exceptionString.c_str());
		std::wstring name = line->getFirstChild()->getValue().to_string();
		std::transform(name.begin(), name.end(), name.begin(), towlower);
		Sexp *nicknamesSexp = line->getSecondChild();
		if (!nicknamesSexp->isList())
			throw UnexpectedInputException("ActorEditDistance::ActorEditDistance", exceptionString.c_str());
		for (int i = 0; i < nicknamesSexp->getNumChildren(); i++) {
			Sexp *nicknameSexp = nicknamesSexp->getNthChild(i);
			std::wstring nickname = L"";
			if (!nicknameSexp->isList())
				throw UnexpectedInputException("ActorEditDistance::ActorEditDistance", exceptionString.c_str());
			for (int j = 0; j < nicknameSexp->getNumChildren(); j++) {
				if (nickname.length() > 0)
					nickname += L" ";
				nickname += nicknameSexp->getNthChild(j)->getValue().to_string();
			}
			std::transform(nickname.begin(), nickname.end(), nickname.begin(), towlower);
			nicknameSet.insert(nickname);
			//std::cout << "adding: " << UnicodeUtil::toUTF8StdString(nickname) << "\n";
		}
		//std::cout << "New entry: " << UnicodeUtil::toUTF8StdString(name) << "\n";
		_nicknames[name] = nicknameSet;
		delete line;

		// Add the reverse for each nickname as well
		BOOST_FOREACH(std::wstring nickname, nicknameSet) {			
			_nicknames[nickname].insert(name);
		}
	}
	stream.close();
}	

ActorEditDistance::~ActorEditDistance() { 
	//_debugStream.close();
}

ActorEditDistance::ActorEditDistanceMap ActorEditDistance::findCloseActors(const Mention *mention, double threshold, const SentenceTheory *st, const DocTheory *dt) {
	ActorEditDistanceMap closeActors;
	const SynNode *head = mention->getHead();
	CharOffset start_offset = st->getTokenSequence()->getToken(head->getStartToken())->getStartCharOffset();
	CharOffset end_offset = st->getTokenSequence()->getToken(head->getEndToken())->getEndCharOffset();
	LocatedString *originalName = dt->getDocument()->getOriginalText()->substring(start_offset.value(), end_offset.value() + 1);
	while (originalName->indexOf(L"\r\n") != -1)
		originalName->replace(L"\r\n", L" ");
	while (originalName->indexOf(L"\n") != -1)
		originalName->replace(L"\n", L" ");
	while (originalName->indexOf(L"  ") != -1)
		originalName->replace(L"  ", L" ");
	std::wstring name = std::wstring(originalName->toWString());
	delete originalName;
	std::transform(name.begin(), name.end(), name.begin(), towlower);
	size_t name_len = name.length();

//	std::cout << "##############################################\n";
//	std::cout << "Checking " << UnicodeUtil::toUTF8StdString(name) << "\n";
//	std::cout << "###############################################\n";

	//_debugStream << L"-------" << name << L"-------\n";
	
	std::vector<ActorPattern *> *actorPatterns = 0;
	if (mention->getEntityType().matchesGPE())
		actorPatterns = &_gpeActors;
	else if (mention->getEntityType().matchesPER())
		actorPatterns = &_perActors;
	else if (mention->getEntityType().matchesORG())
		actorPatterns = &_orgActors;
	else if (mention->getEntityType().matchesLOC())
		actorPatterns = &_locActors;
	else if (mention->getEntityType().matchesFAC())
		actorPatterns = &_facActors;
	else
		return closeActors;

	std::wstringstream wss;
	wss << name << L"_" << mention->getEntityType().getName().to_string() << L"_" << threshold;
	std::wstring cacheKey = wss.str();

	if (_cache.find(cacheKey) != _cache.end())
		return _cache[cacheKey];

	EntityType et = mention->getEntityType();
	std::set<std::wstring> equivalentNameSet;
	equivalentNameSet.insert(name);
	expandNameToEquivalentSet(name, et, equivalentNameSet);	

	for (size_t i = 0; i < actorPatterns->size(); i++) {
		
		std::wstring actorPattern = (*actorPatterns)[i]->lcString;
		ActorId actor_id = (*actorPatterns)[i]->actor_id;

		// Check for exact match between equivalent name set (including original name) and pattern
		std::set<std::wstring>::iterator it1;
		for (it1 = equivalentNameSet.begin(); it1 != equivalentNameSet.end(); ++it1) {
			std::wstring n = *it1;
			if (n == actorPattern) {
				float similarity = (float)0.98;
				if (closeActors.find(actor_id) == closeActors.end() || similarity > closeActors[actor_id])
					closeActors[actor_id] = similarity;
				break;
			}
		}

		// Edit distance between name and pattern
		size_t pat_len = actorPattern.length();
		if ((float)name_len / pat_len < threshold || (float)pat_len / name_len < threshold)
			continue;

		float similarity = _editDistance.similarity(name, actorPattern);

		if (similarity < threshold)
			continue;

		if (closeActors.find(actor_id) == closeActors.end() || similarity > closeActors[actor_id])
			closeActors[actor_id] = similarity;
	}

	if (_cache.size() > AED_MAX_ENTRIES)
		_cache.clear();

	_cache[cacheKey] = closeActors;

	return closeActors;
}

void ActorEditDistance::expandNameToEquivalentSet(std::wstring name, EntityType entityType, std::set<std::wstring> & eqNames) {
	if (!entityType.matchesPER())
		return;

	size_t pos = name.find_first_of(L" ");
	if (pos != std::wstring::npos) {
		std::wstring firstName = name.substr(0, pos);
		if (_nicknames.find(firstName) != _nicknames.end()) {
			std::wstring restOfName = name.substr(pos + 1);
			std::set<std::wstring> nicknameSet = _nicknames[firstName];
			BOOST_FOREACH(std::wstring nickname, nicknameSet) {
				std::wstring newName = nickname + L" " + restOfName;
				//std::cout << "Expanded: " << UnicodeUtil::toUTF8StdString(name) << " to " << UnicodeUtil::toUTF8StdString(newName) << "\n";
				eqNames.insert(newName);
			}
		}
	}
}
