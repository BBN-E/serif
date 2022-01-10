// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/SessionLogger.h"
#include "Generic/patterns/ValueMentionPattern.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/patterns/features/ValueMentionPFeature.h"
#include "Generic/patterns/ShortcutPattern.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/RegexPattern.h"
#include "Generic/patterns/ScoringFactory.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/ValueSet.h"
#include "Generic/theories/Value.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/Sexp.h"


// Private symbols
namespace {
	Symbol valueTypeSym(L"type");
	Symbol activityDateSym(L"activity-date");
	Symbol specificDateSym(L"SPECIFIC-DATE");
	Symbol recentDateSym(L"RECENT-DATE");
	Symbol futureDateSym(L"FUTURE-DATE");
	Symbol valueMentionLabelSym(L"valuementionlabel");
	Symbol regexSym(L"regex");
	Symbol docVTypeFreqSym(L"doc_vtype_freq");
	Symbol sentVTypeFreqSym(L"sent_vtype_freq");
	Symbol equalSym(L"==");
	Symbol notEqualSym(L"!=");
	Symbol lessThanSym(L"<");
	Symbol lessThanOrEqualSym(L"<=");
	Symbol greaterThanSym(L">");
	Symbol greaterThanOrEqualSym(L">=");
}

ValueMentionPattern::ValueMentionPattern(Sexp *sexp, const Symbol::HashSet &entityLabels,
							   const PatternWordSetMap& wordSets)
: _activity_date_status(QueryDate::NOT_SPECIFIC), _must_be_specific_date(false), _must_be_recent_date(false), _must_be_future_date(false)
{
	initializeFromSexp(sexp, entityLabels, wordSets);
}

bool ValueMentionPattern::initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	if (LanguageVariantSwitchingPattern::initializeFromAtom(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (childSexp->getValue() == specificDateSym) {
		_must_be_specific_date = true;
		return true;
	} else if (childSexp->getValue() == recentDateSym) {
		_must_be_recent_date = true;
		return true;
	} else if (childSexp->getValue() == futureDateSym) {
		_must_be_future_date = true;
		return true;
	} else {
		logFailureToInitializeFromAtom(childSexp);
		return false;
	}
}

bool ValueMentionPattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	Symbol constraintType = childSexp->getFirstChild()->getValue();
	if (LanguageVariantSwitchingPattern::initializeFromSubexpression(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (constraintType == valueTypeSym) {
		int n_value_types = childSexp->getNumChildren() - 1;
		for (int j = 0; j < n_value_types; j++) {
			// Note: this will raise an exception if the value type name is unknown.
			_valueTypes.push_back(ValueType(childSexp->getNthChild(j+1)->getValue()));
		}
	} else if (constraintType == activityDateSym) {
		if (childSexp->getSecondChild()->getValue() == Symbol(L"IN_RANGE"))
			_activity_date_status = QueryDate::IN_RANGE;
		else if (childSexp->getSecondChild()->getValue() == Symbol(L"OUT_OF_RANGE"))
			_activity_date_status = QueryDate::OUT_OF_RANGE;
		else throwError(childSexp, "activity-date must be IN_RANGE or OUT_OF_RANGE");
	} else if (constraintType == docVTypeFreqSym || constraintType == sentVTypeFreqSym) {
		if (childSexp->getNumChildren() != 3 || !childSexp->getSecondChild()->isAtom() || !childSexp->getThirdChild()->isAtom())
			throwError(childSexp, "comparison constraints must have three atomic children");
		ComparisonConstraint cc;
		cc.constraint_type = constraintType;
		cc.comparison_operator = childSexp->getSecondChild()->getValue();
		if (cc.comparison_operator != equalSym &&
			cc.comparison_operator != notEqualSym &&
			cc.comparison_operator != lessThanSym &&
			cc.comparison_operator != lessThanOrEqualSym &&
			cc.comparison_operator != greaterThanSym &&
			cc.comparison_operator != greaterThanOrEqualSym)
			throwError(childSexp, "comparison operator must be ==, !=, <, >, <=, or >=");
		cc.value = boost::lexical_cast<int>(childSexp->getThirdChild()->getValue().to_string());
		_comparisonConstraints.push_back(cc);
	} else if (constraintType == regexSym) {
		if (_regexPattern != 0)
			throwError(childSexp, "more than one regex in ValueMentionPattern");
		_regexPattern = parseSexp(childSexp->getSecondChild(), entityLabels, wordSets);
	} else {
		logFailureToInitializeFromChildSexp(childSexp);
		return false;
	}
	return true;
} 

PatternFeatureSet_ptr ValueMentionPattern::matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) {
	ValueMentionSet *vmSet = sTheory->getValueMentionSet();
	PatternFeatureSet_ptr allSentenceMentions = boost::make_shared<PatternFeatureSet>();
	bool matched = false;
	std::vector<float> scores;
	for (int i = 0; i < vmSet->getNValueMentions(); i++) {		
		PatternFeatureSet_ptr sfs = matchesValueMention(patternMatcher, vmSet->getValueMention(i));
		if (sfs) {
			allSentenceMentions->addFeatures(sfs);
			scores.push_back(sfs->getScore());
			matched = true;
		}
	}
	if (matched) {
		// just want the best one from this sentence, no fancy combination [currently will always =_score anyway]
		allSentenceMentions->setScore(ScoringFactory::scoreMax(scores, Pattern::UNSPECIFIED_SCORE));
		return allSentenceMentions;
	} else {
		return PatternFeatureSet_ptr();
	}
}

std::vector<PatternFeatureSet_ptr> ValueMentionPattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) {
	std::vector<PatternFeatureSet_ptr> return_vector;

	if (_force_single_match_sentence) {
		PatternFeatureSet_ptr result = matchesSentence(patternMatcher, sTheory, debug);
		if (result.get() != 0) {
			return_vector.push_back(result);
		}
		return return_vector;
	} 

	ValueMentionSet *vmSet = sTheory->getValueMentionSet();
	for (int i = 0; i < vmSet->getNValueMentions(); i++) {		
		PatternFeatureSet_ptr sfs = matchesValueMention(patternMatcher, vmSet->getValueMention(i));
		if (sfs)
			return_vector.push_back(sfs);
	}
	return return_vector;
}

PatternFeatureSet_ptr ValueMentionPattern::matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides) {

	// fall_through_children is not relevant here

	if (arg->getType() == Argument::MENTION_ARG) {
		MentionSet *mSet = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getMentionSet();
		const Mention *ment = arg->getMention(mSet);

		int stoken = ment->getNode()->getStartToken();
		int etoken = ment->getNode()->getEndToken();
		// this is horrible!
		ValueMentionSet *vmSet = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getValueMentionSet();		
		for (int v = 0; v < vmSet->getNValueMentions(); v++) {
			if (vmSet->getValueMention(v)->getStartToken() == stoken &&
				vmSet->getValueMention(v)->getEndToken() == etoken)
			{
				return matchesValueMention(patternMatcher, vmSet->getValueMention(v));
			} else if (arg->getRoleSym() == Argument::TEMP_ROLE &&
				vmSet->getValueMention(v)->getStartToken() >= stoken &&
				vmSet->getValueMention(v)->getEndToken() <= etoken)
			{
				// look for the temporal INSIDE this mention
				// sometimes it gets hidden, as in "his appearance Tuesday"
				return matchesValueMention(patternMatcher, vmSet->getValueMention(v));
			}
		}
	}
	return PatternFeatureSet_ptr();
}

PatternFeatureSet_ptr ValueMentionPattern::matchesParseNode(PatternMatcher_ptr patternMatcher, int sent_no, const SynNode *node) {
	// Walk over all ValueMentions in sentence, and try to match any whose 
	// head is the node

	int start = node->getStartToken();
	int end = node->getEndToken();

	SentenceTheory *st = patternMatcher->getDocTheory()->getSentenceTheory(sent_no);
	const ValueMentionSet *vms = st->getValueMentionSet();
	for (int i = 0; i < vms->getNValueMentions(); i++) {
		const ValueMention *vm = vms->getValueMention(i);

		if (vm->getStartToken() == start && vm->getEndToken() == end) 
		{
			PatternFeatureSet_ptr returnSet = matchesValueMention(patternMatcher, vm);
			if (returnSet != 0) {
				return returnSet;	
			}
		} /*else if (vm->getStartToken() == start)
		{
			PatternFeatureSet_ptr returnSet = matchesValueMention(patternMatcher, vm);
			if (returnSet != 0) {
				return returnSet;	
			}
		}*/
	}
	return PatternFeatureSet_ptr();
}

PatternFeatureSet_ptr ValueMentionPattern::matchesValueMention(PatternMatcher_ptr patternMatcher, const ValueMention *valueMent) {

	const Value *value = valueMent->getDocValue();

	if (_activity_date_status != QueryDate::NOT_SPECIFIC) {
		const QueryDate* activityDate = patternMatcher->getActivityDate();
		if (!activityDate)
			return PatternFeatureSet_ptr();

		if (value && activityDate->getDateStatus(value) != _activity_date_status)
			return PatternFeatureSet_ptr();
	}

	if (_must_be_specific_date) {
		if (!value) 
			SessionLogger::warn("value_pattern") << "Value has not been created, ValueMention pattern constraint will be ignored.";
		else if (!QueryDate::isSpecificDate(value))
			return PatternFeatureSet_ptr();
	}

	if (_must_be_recent_date) {
		if (!value) 
			SessionLogger::warn("value_pattern") << "Value has not been created, ValueMention pattern constraint will be ignored.";
		else {
			Symbol timexVal = value->getTimexVal();

			boost::optional<boost::gregorian::date> documentDate = patternMatcher->getDocumentDate();
			if (!documentDate) 
				return PatternFeatureSet_ptr();

			std::wstring possibleDate = QueryDate::getYMDDate(value);

			if (possibleDate == L"")
				return PatternFeatureSet_ptr();

			boost::gregorian::date valueDate;
			try {
				valueDate = boost::gregorian::from_string(UnicodeUtil::toUTF8String(possibleDate));
			} catch (...) {
				return PatternFeatureSet_ptr();
			}
	
			if (*documentDate < valueDate ||
				*documentDate - valueDate > boost::gregorian::date_duration(RECENT_DAYS_CONSTRAINT))
			{
				return PatternFeatureSet_ptr();
			}
		}
	}

	if (_must_be_future_date) {
		if (!value) 
			SessionLogger::warn("value_pattern") << "Value has not been created, ValueMention pattern constraint will be ignored.";
		else {
			Symbol timexVal = value->getTimexVal();
			
			boost::optional<boost::gregorian::date> documentDate = patternMatcher->getDocumentDate();
			if (!documentDate) 
				return PatternFeatureSet_ptr();

			std::wstring possibleDate = QueryDate::getYMDDate(value);
			boost::gregorian::date valueDate;

			if (possibleDate != L"") {
				try {
					valueDate = boost::gregorian::from_string(UnicodeUtil::toUTF8String(possibleDate));
				}  catch (...) {
					return PatternFeatureSet_ptr();
				}			
				
				if (*documentDate >= valueDate) {
					return PatternFeatureSet_ptr();
				}
			} else {
				std::wstring yearDate = QueryDate::getYDate(value);
				if (yearDate != L"") {
					yearDate += L"-01-01";
					try {
						valueDate = boost::gregorian::from_string(UnicodeUtil::toUTF8String(yearDate));								
					}  catch (...) {
						return PatternFeatureSet_ptr();
					}
					if (documentDate->year() >= valueDate.year())
						return PatternFeatureSet_ptr();
				} else return PatternFeatureSet_ptr();
			}		
		}
	}

	BOOST_FOREACH(ComparisonConstraint cc, _comparisonConstraints) {
		int actual_value = 0;
		if (cc.constraint_type == docVTypeFreqSym) {
			ValueSet *valSet = patternMatcher->getDocTheory()->getValueSet();
			for (int i = 0; i < valSet->getNValues(); i++) {
				if (valSet->getValue(i)->getFullType() == valueMent->getFullType())
					actual_value++;
			}
		} else if (cc.constraint_type == sentVTypeFreqSym) {
			const ValueMentionSet *vmSet = patternMatcher->getDocTheory()->getSentenceTheory(valueMent->getSentenceNumber())->getValueMentionSet();
			for (int vm = 0; vm < vmSet->getNValueMentions(); vm++) {
				if (vmSet->getValueMention(vm)->getFullType() == valueMent->getFullType())
					actual_value++;
			}
		}
		if ((cc.comparison_operator == equalSym && actual_value != cc.value) ||
			(cc.comparison_operator == notEqualSym && actual_value == cc.value) ||
			(cc.comparison_operator == lessThanSym && actual_value >= cc.value) ||
			(cc.comparison_operator == lessThanOrEqualSym && actual_value > cc.value) ||
			(cc.comparison_operator == greaterThanSym && actual_value <= cc.value) ||
			(cc.comparison_operator == greaterThanOrEqualSym && actual_value < cc.value))
		{
			return PatternFeatureSet_ptr();
		}
	}

	if (_valueTypes.empty()) {
		return makePatternFeatureSet(valueMent, patternMatcher);
	}

	for (size_t i = 0; i < _valueTypes.size(); i++) {
		if (valueMent->getFullType() == _valueTypes[i]) {
			return makePatternFeatureSet(valueMent, patternMatcher);
		}
	}

	return PatternFeatureSet_ptr();
}

PatternFeatureSet_ptr ValueMentionPattern::makePatternFeatureSet(const ValueMention *valueMent, PatternMatcher_ptr patternMatcher, 
											const Symbol matchSym, float confidence) {
	PatternFeatureSet_ptr sfs = boost::make_shared<PatternFeatureSet>();
	// Add a feature for the return value (if we have one)
	if (getReturn())
		sfs->addFeature(boost::make_shared<ValueMentionReturnPFeature>(shared_from_this(), valueMent, patternMatcher->getActiveLanguageVariant()));
	// Add a feature for the pattern itself
	sfs->addFeature(boost::make_shared<ValueMentionPFeature>(shared_from_this(), valueMent, matchSym, patternMatcher->getActiveLanguageVariant(), confidence));
	// Add a feature for the ID (if we have one)
	addID(sfs);
	// Add in any features from the regex sub-pattern.
	if (_regexPattern) {
		RegexPattern_ptr regexPattern = _regexPattern->castTo<RegexPattern>();
		PatternFeatureSet_ptr regexSfs = regexPattern->matchesValueMention(patternMatcher, valueMent);
		if (regexSfs) {
			sfs->addFeatures(regexSfs);
		} else {
			return PatternFeatureSet_ptr();
		}
	}
	// Initialize our score.
	sfs->setScore(this->getScore());

	return sfs;
}

void ValueMentionPattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";
	out << "ValueMentionPattern: ";
	if (!getID().is_null()) out << getID();
	out << std::endl;
	/*
	if (_n_value_types != 0) {		
		for (int i = 0; i < indent; i++) out << " ";
		out << "  value-types = {";
		for (int j = 0; j < _n_value_types; j++)
			out << " " << _valueTypes[j]->getName() << " ";
		out << "}\n";	
	}
	*/	
}

Pattern_ptr ValueMentionPattern::replaceShortcuts(const SymbolToPatternMap &refPatterns) {
	replaceShortcut<RegexPattern>(_regexPattern, refPatterns);
	return shared_from_this();
}

void ValueMentionPattern::getReturns(PatternReturnVecSeq & output) const {
	Pattern::getReturns(output);
	if (_regexPattern)
		_regexPattern->getReturns(output);
}
