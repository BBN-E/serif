// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "ProfileGenerator/PGFact.h"
#include "ProfileGenerator/PGFactDate.h"
#include "boost/foreach.hpp"


PGFact::PGFact(PGFact_ptr origFact, ProfileSlot::DatabaseFactInfo dfi) : _fact_info(origFact->getFactInfo())
{
	BOOST_FOREACH(PGFactDate_ptr date, origFact->getFactDates()) {
		addDate(date);
	}
	BOOST_FOREACH(PGFactArgument_ptr arg, origFact->getAllArguments()) {
		PGFactArgument_ptr newArg = boost::make_shared<PGFactArgument>(*arg);
		if (dfi.kb_role_map.find(arg->getRole()) != dfi.kb_role_map.end())
			newArg->setRole(dfi.kb_role_map[arg->getRole()]);
		addArgument(newArg, !newArg->isInFocus() && dfi.answer_role == arg->getRole());
	}
}

bool PGFact::matchesDFI(ProfileSlot::DatabaseFactInfo dfi) {
	if (getFactType() != dfi.fact_type || !getAgentArgument() || getAgentArgument()->getRole() != dfi.focus_role)
		return false;

	if (getDBFactType() != dfi.source_info.first || getSourceId() != dfi.source_info.second)
		return false;

	// Also check to make sure the answer role is specified if it's non-null
	if (dfi.answer_role == "")
		return true;

	for (std::vector<PGFactArgument_ptr>::iterator iter = _allArguments.begin(); iter != _allArguments.end(); iter++) {
		if ((*iter)->getRole() == dfi.answer_role)
			return true;
	}

	// We couldn't find an argument with the answer role, so this isn't a match
	return false;
}


void PGFact::addArgument(PGFactArgument_ptr arg, bool is_answer) { 
	_allArguments.push_back(arg);
	if (arg->isInFocus())
		_agentArgument = arg;
	else if (is_answer) 
		_answerArgument = arg;
	else _otherArguments.push_back(arg); 
} 

PGFactArgument_ptr PGFact::getFirstArgumentForRole(std::string role) {
	BOOST_FOREACH(PGFactArgument_ptr fa, _allArguments) {
		if (fa->getRole() == role)
			return fa;
	}
	return PGFactArgument_ptr();
}

std::vector<PGFactArgument_ptr> PGFact::getArgumentsForRole(std::string role) {
	std::vector<PGFactArgument_ptr> results;
	BOOST_FOREACH(PGFactArgument_ptr fa, _allArguments) {
		if (fa->getRole() == role)
			results.push_back(fa);
	}
	return results;
}

bool PGFact::hasReliableAgentMentionConfidence() { 
	if (_agentArgument)
		return _agentArgument->isReliable(); 
	else return false;
}

bool PGFact::hasReliableAnswerMentionConfidence() {
	if (_answerArgument)
		return _answerArgument->isReliable();
	else return false;
}

void PGFact::addDate(PGFactDate_ptr date) {
	_factDates.push_back(date);
}

std::vector<PGFactDate_ptr> PGFact::getFactDates() {
	return _factDates;
}

bool PGFact::hasNoDates() {
	return _factDates.size() == 0;
}

bool PGFact::isEquivalent(PGFact_ptr other) {
	if (other->getFactType() != _fact_info.fact_type_id)
		return false;

	if (_agentArgument) {
		if (!other->getAgentArgument())
			return false;
		if (!_agentArgument->isEquivalent(other->getAgentArgument()))
			return false;
	} else if (other->getAgentArgument()) {
		return false;
	}

	std::vector<PGFactArgument_ptr> otherArgs = other->getOtherArguments();
	if (otherArgs.size() != _otherArguments.size())
		return false;
	bool problem = false;
	for (std::vector<PGFactArgument_ptr>::iterator iter1 = _otherArguments.begin(); iter1 != _otherArguments.end(); iter1++) {
		bool found_match = false;		
		for (std::vector<PGFactArgument_ptr>::iterator iter2 = otherArgs.begin(); iter2 != otherArgs.end(); iter2++) {
			if ((*iter1)->isEquivalent(*iter2)) {
				found_match = true;
				break;
			}
		}
		if (!found_match)
			return false;
	}
	return true;
}
