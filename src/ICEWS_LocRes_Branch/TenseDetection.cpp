// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#include <boost/algorithm/string.hpp>

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/theories/Parse.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Value.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/ValueSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/DocTheory.h"

#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/PropPFeature.h"
#include "Generic/patterns/features/MentionPFeature.h"

#include "ICEWS/TenseDetection.h"
#include "ICEWS/Stories.h"

namespace ICEWS {

// copied from QueryDate
// these are expressions for specifically parsing TIMEX normalizations of dates

// YYYY-MM-DD
const boost::wregex TenseDetection::_timex_regex_ymd(L"^([12][0-9][0-9][0-9])-([0123]?[0-9])-([0123]?[0-9]).*");
// YYYY-MM
const boost::wregex TenseDetection::_timex_regex_ym(L"^([12][0-9][0-9][0-9])-([01]?[0-9]).*");
// YYYY-W##
const boost::wregex TenseDetection::_timex_regex_yw(L"^([12][0-9][0-9][0-9])-W([012345][0-9]).*");
// ####T
const boost::wregex TenseDetection::_timex_clock_time(L"^([12][0-9][0-9][0-9])T.*");
// YYYY
const boost::wregex TenseDetection::_timex_regex_y(L"^([12][0-9][0-9][0-9]).*");
// YYY
const boost::wregex TenseDetection::_timex_regex_decade(L"^([12][0-9][0-9])$");
// past XX months or years or decades
const boost::wregex TenseDetection::_timex_regex_past_my(L"^(P[0-9]+[MYD]E?)");


const boost::wregex TenseDetection::_superlative(L"the \\S*est ");

void TenseDetection::setTense(PatternFeatureSet_ptr match, ICEWSEventMentionFinder::MatchData& matchData, const DocTheory *docTheory, SentenceTheory *sentTheory) {
	Symbol tense = getTense(match, matchData, docTheory, sentTheory);
	if (!tense.is_null()) {
		matchData.tense = tense;
		return;
	}
}

Symbol TenseDetection::getTense(PatternFeatureSet_ptr match, ICEWSEventMentionFinder::MatchData& matchData, const DocTheory *docTheory, SentenceTheory *sentTheory) {

	MentionSet *ms = sentTheory->getMentionSet();
	ValueMentionSet *vms = sentTheory->getValueMentionSet();
	PropositionSet *ps = sentTheory->getPropositionSet();

	std::wstring event_code_str = matchData.eventCode.to_string();

	bool is_violence = (event_code_str.find(L"18") == 0 || event_code_str.find(L"19") == 0);

	// Look for dates that directly modify propositions/mentions in the PatternFeatureSet
	bool is_historical = false;
	Symbol tense = Symbol();
	for (size_t f = 0; f < match->getNFeatures(); f++) {
		const Proposition *prop = 0;
		const Mention *ment = 0;
		const SynNode *node = 0;
		if (PropPFeature_ptr sf = boost::dynamic_pointer_cast<PropPFeature>(match->getFeature(f))) {
			prop = sf->getProp();
			node = prop->getPredHead();
			if (prop->getNArgs() > 0 && prop->getArg(0)->getRoleSym() == Argument::REF_ROLE &&
				prop->getArg(0)->getType() == Argument::MENTION_ARG)
				ment = prop->getArg(0)->getMention(ms);
		} else if (MentionPFeature_ptr sf = boost::dynamic_pointer_cast<MentionPFeature>(match->getFeature(f))) {
			ment = sf->getMention();
			node = ment->getNode();
			prop = ps->getDefinition(ment->getIndex());
		} else continue;

		// constructions like "he was sentenced thursday for his bombing of the city last month" are dangerous
		int prop_hist = getHistoricity(docTheory, sentTheory, prop, 0);
		if (prop_hist == OLDER_THAN_ONE_MONTH_ONGOING && (event_code_str == L"173" || event_code_str == L"175")) {
			Symbol predSym = prop->getPredSymbol();
			if (predSym == Symbol(L"detained") || predSym == Symbol(L"imprisoned") || predSym == Symbol(L"jailed"))
				prop_hist = OLDER_THAN_ONE_MONTH;
		}
		if (is_violence && prop != 0 && prop_hist == WITHIN_ONE_MONTH) {
			Symbol predSym = prop->getPredSymbol();
			if (predSym == Symbol(L"wanted") || predSym == Symbol(L"arrested") || predSym == Symbol(L"condemned") || predSym == Symbol(L"denounced") || 
				predSym == Symbol(L"vilified") || predSym == Symbol(L"admonished") || predSym == Symbol(L"blasted") || predSym == Symbol(L"assailed") || 
				predSym == Symbol(L"castigated") || predSym == Symbol(L"censured") || predSym == Symbol(L"castigated") || predSym == Symbol(L"chastised") || 
				predSym == Symbol(L"chided") || predSym == Symbol(L"criticized") || predSym == Symbol(L"criticised") || predSym == Symbol(L"decried") || 
				predSym == Symbol(L"denigrated") || predSym == Symbol(L"deplored") || predSym == Symbol(L"derided") || predSym == Symbol(L"lambasted") || 
				predSym == Symbol(L"rebuked") || predSym == Symbol(L"scolded") || predSym == Symbol(L"slammed") || predSym == Symbol(L"court-martialed") || 
				predSym == Symbol(L"charged") || predSym == Symbol(L"tried") || predSym == Symbol(L"sentenced"))
				continue;
		}
		if (prop_hist == WITHIN_ONE_MONTH)
			return ICEWSEventMention::CURRENT_TENSE;
		if (tense.is_null() || tense == ICEWSEventMention::ONGOING_TENSE) {
			if (prop_hist == OLDER_THAN_ONE_MONTH)
				tense = ICEWSEventMention::HISTORICAL_TENSE;
			else if (prop_hist == OLDER_THAN_ONE_MONTH_ONGOING)
				tense = ICEWSEventMention::ONGOING_TENSE;
		}
		if (tense.is_null() || tense == ICEWSEventMention::ONGOING_TENSE) {
			Symbol temp_tense = getTense(node);
			if (!temp_tense.is_null())
				tense = temp_tense;
		}
		if (tense.is_null() || tense == ICEWSEventMention::ONGOING_TENSE) {
			Symbol temp_tense = getTense(sentTheory, ment);
			if (!temp_tense.is_null())
				tense = temp_tense;
		}
	}
	if (!tense.is_null())
		return tense;

	// Look for full-sentence modifying values (anything at the beginning of a sentence)
	// Also look for "in DATE" constructions which are more likely to apply to the whole sentence context
	for (int v = 0; v < vms->getNValueMentions(); v++) {
		ValueMention *val = vms->getValueMention(v);
		if (val->getStartToken() == 0) {
			if (getHistoricity(docTheory, sentTheory, val) == OLDER_THAN_ONE_MONTH)
				return ICEWSEventMention::HISTORICAL_TENSE;
		} else if (val->getStartToken() < 3 && 3 < sentTheory->getTokenSequence()->getNTokens()) {
			int counter = 0;
			std::wstring tokStr = sentTheory->getTokenSequence()->getToken(0)->getSymbol().to_string();
			boost::to_lower(tokStr);
			if (tokStr == L"earlier" || tokStr == L"later" || tokStr == L"already") {
				tokStr = sentTheory->getTokenSequence()->getToken(1)->getSymbol().to_string();
				boost::to_lower(tokStr);
			}
			if ((tokStr == L"in" || tokStr == L"on" || tokStr == L"but") && getHistoricity(docTheory, sentTheory, val) == OLDER_THAN_ONE_MONTH)
				return ICEWSEventMention::HISTORICAL_TENSE;
			if ((tokStr == L"between" || tokStr == L"from") && getHistoricity(docTheory, sentTheory, val) == OLDER_THAN_ONE_MONTH)
				return ICEWSEventMention::ONGOING_TENSE;
		}
	}

	// A violent event is historical if any value mention in the sentence is historical
	// We do not do this for 14 (Protest) and 17 (Coerce) [tested empirically]
	if (is_violence) {	
		for (int v = 0; v < vms->getNValueMentions(); v++) {
			ValueMention *val = vms->getValueMention(v);
			int historicity = getHistoricity(docTheory, sentTheory, val);
			if (historicity == OLDER_THAN_ONE_MONTH)
				return ICEWSEventMention::HISTORICAL_TENSE;
			else if (historicity == OLDER_THAN_ONE_MONTH_ONGOING)
				return ICEWSEventMention::ONGOING_TENSE;
		}	
	}

	return Symbol();

}

Symbol TenseDetection::getTense(const SynNode* node) {
	if (node == 0)
		return Symbol();
	
	const SynNode *dominatingSNode = node;
	while (dominatingSNode != 0 && dominatingSNode->getTag() != Symbol(L"S")) {
		dominatingSNode = dominatingSNode->getParent();
	}

	// Get constructions like "Three years after ..." ??
	if (dominatingSNode != 0 && dominatingSNode->getParent() != 0) {
		const SynNode *tempNode = dominatingSNode->getParent();
		if (tempNode->getTag() == Symbol(L"SBAR") && tempNode->getHeadIndex() != 0 && tempNode->getHeadWord() == Symbol(L"after")) {
			const SynNode *possibleTime = tempNode->getChild(0);
			if (possibleTime->getHeadWord() == Symbol(L"years") ||
				possibleTime->getHeadWord() == Symbol(L"months") ||
				possibleTime->getHeadWord() == Symbol(L"decades") ||
				possibleTime->getHeadWord() == Symbol(L"centuries") ||
				possibleTime->getHeadWord() == Symbol(L"year") ||
				possibleTime->getHeadWord() == Symbol(L"month") ||
				possibleTime->getHeadWord() == Symbol(L"decade") ||
				possibleTime->getHeadWord() == Symbol(L"century"))
			{
				std::cout << "node historical\n";
				return ICEWSEventMention::HISTORICAL_TENSE;
			}
		}
	}

	return Symbol();
}


Symbol TenseDetection::getTense(SentenceTheory *sentTheory, const Mention* mention) {

	if (mention == 0)
		return Symbol();

	// look for historical modifiers on any of these mentions
	PropositionSet *ps = sentTheory->getPropositionSet();
	for (int p = 0; p < ps->getNPropositions(); p++) {		
		const Proposition *prop = ps->getProposition(p);
		if (prop->getPredSymbol().is_null())
			continue;
		std::wstring predicate = prop->getPredSymbol().to_string();
		if (prop->getPredType() == Proposition::MODIFIER_PRED && 
			prop->getNArgs() > 0 && prop->getArg(0)->getType() == Argument::MENTION_ARG &&
			prop->getArg(0)->getMentionIndex() == mention->getIndex())
		{			
			// then this is a modifier for this mention
			if (predicate == L"long-standing" || predicate == L"longstanding" ||
				predicate == L"standing" || predicate == L"longest-standing" ||
				predicate == L"long-running" || predicate == L"longrunning" ||
				predicate == L"longest-running")
			{
				std::cout << "mention ongoing historical\n";
				return ICEWSEventMention::ONGOING_TENSE;			
			} else if (predicate == L"past") {
				std::cout << "mention past historical\n";
				return ICEWSEventMention::HISTORICAL_TENSE;
			} else if (!mention->getEntityType().matchesPER() && predicate == L"-old") {
				std::cout << "mention -old historical\n";
				return ICEWSEventMention::ONGOING_TENSE;			
			}
		}
		// anniversary of X
		if (prop->getPredType() == Proposition::NOUN_PRED && 
			(predicate == L"decade" || predicate == L"decades" || predicate == L"year" || predicate == L"years" || 
			 predicate == L"months" || predicate == L"anniversary" || predicate == L"commemoration"))
		{
			for (int a = 0; a < prop->getNArgs(); a++) {
				if (prop->getArg(a)->getRoleSym() == Symbol(L"of") &&
					prop->getArg(a)->getType() == Argument::MENTION_ARG &&
					prop->getArg(a)->getMentionIndex() == mention->getIndex()) 
				{
					if (predicate == L"anniversary" || predicate == L"commemoration") {
						std::cout << "mention anniversary historical\n";
						return ICEWSEventMention::HISTORICAL_TENSE;
					} else {
						std::cout << "months/years/decades of... historical\n";
						return ICEWSEventMention::ONGOING_TENSE;
					}
				}
			}
		}
	}
	return Symbol();
}



int TenseDetection::getHistoricity(const DocTheory *docTheory, SentenceTheory *sentTheory, const Proposition *prop, int depth) {
	if (prop == 0)
		return UNKNOWN;
	// Avoid cycles
	if (depth > 10)
		return UNKNOWN;
	MentionSet *ms = sentTheory->getMentionSet();
	ValueMentionSet *vms = sentTheory->getValueMentionSet();
	int referent_index = -1;
	for (int a = 0; a < prop->getNArgs(); a++) {
		Argument *arg = prop->getArg(a);
		if (arg->getRoleSym() == Argument::REF_ROLE)
			referent_index = arg->getMentionIndex();
		ValueMention *val = getValueFromArgument(ms, vms, arg, false);
		if (val != 0) {
			int hist = getHistoricity(docTheory, sentTheory, val);
			if (hist == OLDER_THAN_ONE_MONTH && isSinceValue(sentTheory, val))
				hist = OLDER_THAN_ONE_MONTH_ONGOING;
			if (hist != UNKNOWN)
				return hist;
		}
		if (arg->getRoleSym() == Symbol(L"following") || arg->getRoleSym() == Symbol(L"after")  || arg->getRoleSym() == Symbol(L"since")) {
			int hist = UNKNOWN;
			if (arg->getType() == Argument::PROPOSITION_ARG) {
				if (arg->getProposition() != prop)
					hist = getHistoricity(docTheory, sentTheory, arg->getProposition(), depth + 1);				
			} else if (arg->getType() == Argument::MENTION_ARG) {
				const Proposition *defProp = sentTheory->getPropositionSet()->getDefinition(arg->getMentionIndex());
				if (defProp != 0 && defProp != prop)
					hist = getHistoricity(docTheory, sentTheory, defProp, depth + 1);				
			}
			if (hist == OLDER_THAN_ONE_MONTH && arg->getRoleSym() == Symbol(L"since"))
				hist = OLDER_THAN_ONE_MONTH_ONGOING;
			if (hist != UNKNOWN)
				return hist;
		}
		if (arg->getType() == Argument::MENTION_ARG &&
			prop->getPredSymbol() != Symbol(L"sentenced") &&
			(arg->getRoleSym() == Symbol(L"in") || arg->getRoleSym() == Symbol(L"for") || arg->getRoleSym() == Argument::TEMP_ROLE))
		{
			std::wstring headword = arg->getMention(ms)->getNode()->getHeadWord().to_string();
			if ((headword == L"months" || headword == L"years" || headword == L"year" || headword == L"decades" || headword == L"decade") &&
				!hasSuperlative(sentTheory))
			{
				std::wstring mentionText = arg->getMention(ms)->getNode()->toTextString();
				if (mentionText.find(L"past ") != std::wstring::npos) {
					std::cout << "in/for past months/years/decades\n";
					return OLDER_THAN_ONE_MONTH_ONGOING;
				} else {
					std::cout << "in/for months/years/decades\n";
					return OLDER_THAN_ONE_MONTH;
				}
			}
		}
	}
	
	// in TIME when EVENT
	// did X in TIME as EVENT
	PropositionSet *ps = sentTheory->getPropositionSet();
	for (int p = 0; p < ps->getNPropositions(); p++) {		
		const Proposition *newProp = ps->getProposition(p);	
		ValueMention *temporalVM = 0;
		bool allow_value = false;
		for (int a = 0; a < newProp->getNArgs(); a++) {
			Argument *arg = newProp->getArg(a);
			if (arg->getRoleSym() == Argument::REF_ROLE && arg->getMentionIndex() == referent_index)
				allow_value = true;
			ValueMention *temp = getValueFromArgument(ms, vms, arg, newProp->getPredSymbol() == Symbol(L"when"));
			if (temp != 0 && temp->getType() == Symbol(L"TIMEX2"))
				temporalVM = temp;
			if (arg->getType() == Argument::PROPOSITION_ARG &&
				(arg->getRoleSym() == Symbol(L"when") || arg->getRoleSym() == Symbol(L"as") || arg->getRoleSym() == Symbol(L"after")) &&
				arg->getProposition() == prop)
			{
				allow_value = true;
			}
		}
		if (allow_value && temporalVM != 0) {
			return getHistoricity(docTheory, sentTheory, temporalVM);
		}
	}

	return UNKNOWN;
}

bool TenseDetection::hasSuperlative(SentenceTheory *st) {

	std::wstring sentString = st->getPrimaryParse()->getRoot()->toTextString();
	if (sentString.find(L"the first") != std::wstring::npos)
		return true;
	boost::wcmatch matchResult;
	if (boost::regex_search(sentString.c_str(), matchResult, _superlative))
		return true;
	return false;
}


ValueMention* TenseDetection::getValueFromArgument(MentionSet *ms, ValueMentionSet *vms, Argument *arg, bool is_known_temp) {
	is_known_temp = is_known_temp || arg->getRoleSym() == Argument::TEMP_ROLE;
	if (arg->getType() == Argument::MENTION_ARG) {					
		const Mention *ment = arg->getMention(ms);
		ValueMention *val = getValueFromMention(vms, ment, is_known_temp);
		if (val != 0)
			return val;
		if (ment->getMentionType() == Mention::LIST) {
			const Mention *child = ment->getChild();
			while (child != 0) {
				ValueMention *val = getValueFromMention(vms, child, is_known_temp);
				if (val != 0)
					return val;
				child = child->getNext();
			}
		}
	}
	return 0;
}

ValueMention* TenseDetection::getValueFromMention(ValueMentionSet *vms, const Mention *ment, bool is_known_temp) {
	int stoken = ment->getNode()->getStartToken();
	int etoken = ment->getNode()->getEndToken();

	bool has_date_head = (ment->getNode()->getHeadPreterm()->getTag() == Symbol(L"DATE-NNP"));
	// this is horrible! and copied from ValueMentionPattern!					
	for (int v = 0; v < vms->getNValueMentions(); v++) {
		ValueMention *val = vms->getValueMention(v);
		if ((val->getStartToken() == stoken && val->getEndToken() == etoken) ||
			((is_known_temp || has_date_head) && val->getStartToken() >= stoken && val->getEndToken() <= etoken))
		{
			return val;
		}
		if (has_date_head && val->getStartToken() == stoken)
			return val;
	}
	return 0;
}

bool TenseDetection::isSinceValue(SentenceTheory *st, ValueMention *val) {
	if (val->getStartToken() != 0) {
		std::wstring precedingTok = st->getTokenSequence()->getToken(val->getStartToken() - 1)->getSymbol().to_string();
		boost::to_lower(precedingTok);
		if (precedingTok == L"since")
			return true;
	}
	return false;
}


int TenseDetection::getHistoricity(const DocTheory *docTheory, SentenceTheory *st, ValueMention *val) {

	// this is somewhat copied from QueryDate

	Value *fullValue = docTheory->getValueSet()->getValueByValueMention(val->getUID());
	if (fullValue == 0 || fullValue->getTimexVal() == Symbol())
		return UNKNOWN;

	// doesn't count if it's preceded by "since" and there is a superlative in the sentence like "the first" or "the -est"
	if (isSinceValue(st, val) && hasSuperlative(st)) {
		std::cout << "superlative since, ignoring\n";
		return UNKNOWN;
	}		
	
	std::wstring originalText = L"";
	for (int t = val->getStartToken(); t <= val->getEndToken(); t++) {
		originalText += st->getTokenSequence()->getToken(t)->getSymbol().to_string();
		originalText += L" ";
	}
	std::transform(originalText.begin(), originalText.end(), originalText.begin(), towlower);

	if (originalText.compare(L"the past ") == 0)
		return OLDER_THAN_ONE_MONTH;

	std::wstring valString = fullValue->getTimexVal().to_string();
	//SessionLogger::info("BRANDY") << originalText << ": " << valString << "\n";

	// these are often wrong, so don't rely on them here to return recent dates
	// remember that originalText will have a space on its end
	bool is_untrustworthy = false;
	if (originalText.find(L" day ") != std::wstring::npos)
		is_untrustworthy = true;
	if (originalText.find(L" night") != std::wstring::npos || originalText.compare(L"night ") == 0)
		is_untrustworthy = true;

	std::string publicationDate = Stories::getStoryPublicationDate(docTheory->getDocument());
	if (publicationDate.empty())
		return UNKNOWN;
	if (publicationDate.size() != 10) {
		SessionLogger::info("ICEWS") << "Unexpected publication date format"
									 << '"' << publicationDate << '"';
		return UNKNOWN;
	}
	int doc_year = atoi(publicationDate.substr(0,4).c_str());
	int doc_month = atoi(publicationDate.substr(5,2).c_str());
	int doc_day = atoi(publicationDate.substr(8,2).c_str());

	int target_month = doc_month;
	int target_year = doc_year;
	int target_day = doc_day;
	// subtract month (crudely)
	if (target_month > 1)
		target_month--;
	else {
		target_month = 12;
		target_year--;
	}

	// run a regex over the timex value (sadly these aren't always straight year/month/date)
	boost::wcmatch matchResult;
	if (boost::regex_match(valString.c_str(), matchResult, _timex_clock_time)) {		
		return UNKNOWN;
	} else if (boost::regex_match(valString.c_str(), matchResult, _timex_regex_ymd)) {		
		std::wstring wstr = matchResult[1];
		int year = _wtoi(wstr.c_str());	
		wstr = matchResult[2];
		int month = _wtoi(wstr.c_str());	
		wstr = matchResult[3];
		int day = _wtoi(wstr.c_str());

		// same year/month
		if (year == doc_year && month == doc_month)
			return (is_untrustworthy ? UNKNOWN : WITHIN_ONE_MONTH);

		// previous month, later day
		if (((year == doc_year && month == doc_month - 1) || (year == doc_year - 1 && month == 12 && doc_month == 1)) && day >= doc_day)
			return (is_untrustworthy ? UNKNOWN : WITHIN_ONE_MONTH);

		if (year < target_year || (year == target_year && month < target_month) || (year == target_year && month == target_month && day < target_day))
			return OLDER_THAN_ONE_MONTH;

	} else if (boost::regex_match(valString.c_str(), matchResult, _timex_regex_ym)) {
		std::wstring wstr = matchResult[1];
		int year = _wtoi(wstr.c_str());	
		wstr = matchResult[2];
		int month = _wtoi(wstr.c_str());

		if (year == doc_year && month == doc_month)
			return (is_untrustworthy ? UNKNOWN : WITHIN_ONE_MONTH);
		
		if (year < target_year || (year == target_year && month <= target_month))
			return OLDER_THAN_ONE_MONTH;

	}  else if (boost::regex_match(valString.c_str(), matchResult, _timex_regex_yw)) {

		// we're going to assume this only happens for stuff like "last week", which is never historical
		return WITHIN_ONE_MONTH;

	} else if (boost::regex_match(valString.c_str(), matchResult, _timex_regex_y)) {		
		std::wstring wstr = matchResult[1];
		int year = _wtoi(wstr.c_str());	

		// use document year here, not target year (even in January 2008, 2007 is historical)
		if (year < doc_year)
			return OLDER_THAN_ONE_MONTH;

		// we don't return WITHIN_ONE_MONTH here

	} else if (boost::regex_match(valString.c_str(), matchResult, _timex_regex_decade)) {		
		std::wstring wstr = matchResult[1];
		int year = _wtoi(wstr.c_str());	
		year = year * 10 + 9; // 198 --> 1989

		// use document year here, not target year (even in January 2008, 2007 is historical)
		if (year < doc_year)
			return OLDER_THAN_ONE_MONTH;
	}

	return UNKNOWN;
	

}

}
