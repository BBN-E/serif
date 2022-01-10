// Copyright (c) 2006 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/PropStatusManager.h"
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include "Generic/common/Sexp.h"
#include "Generic/theories/Proposition.h"


// Private symbols
namespace {
	Symbol psmSym(L"psm");
	Symbol blockStatusSym(L"block_status");
	Symbol requireStatusSym(L"require_status");
}


PropStatusManager::PropStatusManager(bool require_negative) {
	if (require_negative)
		addRequiredStatus(PropositionStatus::NEGATIVE);
}

PropStatusManager::PropStatusManager(Sexp* sexp) {
	int n_statuses = sexp->getNumChildren() - 1;
	for(int j = 0; j < n_statuses; j++) {
		Sexp *child = sexp->getNthChild(j+1);
		if (!child->isAtom())
			throwError(sexp);
		std::vector<std::wstring> parts;
		std::wstring stringToSplit = child->getValue().to_string();
		boost::split(parts, stringToSplit, boost::is_any_of(":"));
		if (parts.size() != 2)
			throwError(sexp);
		PropositionStatusAttribute status = PropositionStatusAttribute::getFromString(parts[1].c_str());
		bool result = false;
		if (parts[0] == L"allow")
			result = addAllowedStatus(status);
		else if (parts[0] == L"block")
			result = addBlockedStatus(status);
		else if (parts[0] == L"require")
			result = addRequiredStatus(status);
		if (!result)
			throwError(sexp);
	}
}

void PropStatusManager::throwError(Sexp *sexp) {
	std::stringstream error;
	error << "Invalid PropStatusManager spec: " << sexp->to_debug_string();
	throw UnexpectedInputException("PropStatusManager::PropStatusManager", error.str().c_str());
}

bool PropStatusManager::isPropStatusManagerSym(Symbol constraintType) {
	return (constraintType == psmSym);
}

bool PropStatusManager::propositionIsValid(PatternMatcher_ptr patternMatcher, const Proposition* prop) {
	BOOST_FOREACH(PropositionStatusAttribute status, prop->getStatuses()) {
		if (blocks(status))
			return false;

		if (patternMatcher->isBlockedPropositionStatus(status) && !explicitlyAllows(status))
			return false;
	}

	BOOST_FOREACH(PropositionStatusAttribute requiredStatus, _required) {
		if (!prop->hasStatus(requiredStatus))
			return false;
	}

	return true;
}

bool PropStatusManager::addAllowedStatus(PropositionStatusAttribute att) {
	if (_blocked.find(att) != _blocked.end())
		return false;

	_allowed.insert(att);
	return true;
}

bool PropStatusManager::addRequiredStatus(PropositionStatusAttribute att) {
	if (_blocked.find(att) != _blocked.end())
		return false;

	_required.insert(att);
	_allowed.insert(att);
	return true;
}

bool PropStatusManager::addBlockedStatus(PropositionStatusAttribute att) {	
	if (_allowed.find(att) != _allowed.end())
		return false;

	_blocked.insert(att);
	return true;
}

bool PropStatusManager::explicitlyAllows(PropositionStatusAttribute att) {
	return (_allowed.find(att) != _allowed.end() || _required.find(att) != _required.end());
}
bool PropStatusManager::blocks(PropositionStatusAttribute att) {
	return (_blocked.find(att) != _blocked.end());
}
