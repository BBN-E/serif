// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/icews/ActorMentionPattern.h"
#include "Generic/icews/ICEWSActorInfo.h"
#include "Generic/theories/ActorMention.h"
#include "Generic/common/Sexp.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/patterns/features/PatternFeatureSet.h"

namespace {
	Symbol mustBeCountrySym(L"is-country");
	Symbol mayNotBeCountrySym(L"is-not-country");
	Symbol properNounSym(L"proper-noun");
	Symbol compositeSym(L"composite");
	Symbol actorCodeSym(L"actor-code");
	Symbol agentCodeSym(L"agent-code");
	Symbol sectorCodeSym(L"sector-code");
	Symbol blockActorCodeSym(L"block-actor-code");
	Symbol blockAgentCodeSym(L"block-agent-code");
	Symbol blockSectorCodeSym(L"block-sector-code");
	Symbol mentionSym(L"mention");
	Symbol blockMentionSym(L"block-mention");
	Symbol isNotDictionaryActorSym(L"is-not-dictionary-actor");
	Symbol isDictionaryActorSym(L"is-dictionary-actor");
}

ICEWSActorMentionPattern::ICEWSActorMentionPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets)
: _actorType(ANY_ACTOR), _mustBeCountry(false), _mayNotBeCountry(false), _isDictionaryActor(false), _isNotDictionaryActor(false)
{
	initializeFromSexp(sexp, entityLabels, wordSets);
	_use_awake_db_for_icews = ParamReader::isParamTrue("use_awake_db_for_icews");
}

bool ICEWSActorMentionPattern::initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	Symbol atom = childSexp->getValue();
	if (Pattern::initializeFromAtom(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (atom == mustBeCountrySym) {
		_mustBeCountry = true;
	} else if (atom == mayNotBeCountrySym) {
		_mayNotBeCountry = true;
	} else if (atom == isDictionaryActorSym) {
		_isDictionaryActor = true;
	} else if (atom == isNotDictionaryActorSym) {
		_isNotDictionaryActor = true;
	} else if (atom == properNounSym) {
		_actorType = PROPER_NOUN_ACTOR;
	} else if (atom == compositeSym) {
		_actorType = COMPOSITE_ACTOR;
	} else {
		logFailureToInitializeFromAtom(childSexp);
		return false;
	}
	return true;
}

bool ICEWSActorMentionPattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	Symbol constraintType = childSexp->getFirstChild()->getValue();
	if (Pattern::initializeFromSubexpression(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (constraintType == agentCodeSym) {
		fillListOfWords(childSexp, wordSets, _agentCodes, true, false);
	} else if (constraintType == actorCodeSym) {
		fillListOfWords(childSexp, wordSets, _actorCodes, true, false);
	} else if (constraintType == sectorCodeSym) {
		fillListOfWords(childSexp, wordSets, _sectorCodes, true, false);
	} else if (constraintType == blockAgentCodeSym) {
		fillListOfWords(childSexp, wordSets, _blockAgentCodes, true, false);
	} else if (constraintType == blockActorCodeSym) {
		fillListOfWords(childSexp, wordSets, _blockActorCodes, true, false);
	} else if (constraintType == blockSectorCodeSym) {
		fillListOfWords(childSexp, wordSets, _blockSectorCodes, true, false);
	} else if (constraintType == mentionSym) {
		if (childSexp->getNumChildren() != 2)
			throwError(childSexp, "expected exactly one mention in (icews-actor (mention ...))");
		_mention = parseSexp(childSexp->getNthChild(1), entityLabels, wordSets);
	} else if (constraintType == blockMentionSym) {
		if (childSexp->getNumChildren() != 2)
			throwError(childSexp, "expected exactly one mention in (icews-actor (block-mention ...))");
		_blockMention = parseSexp(childSexp->getNthChild(1), entityLabels, wordSets);
	} else {
		logFailureToInitializeFromChildSexp(childSexp);		
		return false;
	}
	return true;
}

Pattern_ptr ICEWSActorMentionPattern::replaceShortcuts(const SymbolToPatternMap &refPatterns) {
	replaceShortcut<MentionMatchingPattern>(_mention, refPatterns);
	replaceShortcut<MentionMatchingPattern>(_blockMention, refPatterns);
	return shared_from_this();
}

void ICEWSActorMentionPattern::getReturns(PatternReturnVecSeq & output) const {
	Pattern::getReturns(output);
	if (_mention) _mention->getReturns(output);
}

void ICEWSActorMentionPattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";	
	out << "ActorMentionPattern:";
	if (!getID().is_null()) out << getID();
	out << std::endl;
	for (int i = 0; i < indent; i++) out << " ";	
	out << "  (dump method not implemented yet)";
}

PatternFeatureSet_ptr ICEWSActorMentionPattern::matchesICEWSActorMention(PatternMatcher_ptr patternMatcher, ActorMention_ptr m) {

	// We'd like to use the EventMentionFinder's _actorInfo here, but
	// we can't pass it down through matchesDocument(...), since
	// that's an inherited function. Is there a better way to do this? 
	ActorInfo_ptr actorInfo = ActorInfo::getAppropriateActorInfoForICEWS(); 

	// Constraints: actor type
	if ((_actorType == PROPER_NOUN_ACTOR) && !boost::dynamic_pointer_cast<ProperNounActorMention>(m))
		return PatternFeatureSet_ptr();
	if ((_actorType == COMPOSITE_ACTOR) && !boost::dynamic_pointer_cast<CompositeActorMention>(m))
		return PatternFeatureSet_ptr();

	// Get the actor & agent info.
	Symbol actorCode, agentCode;
	ActorId actorId;
	AgentId agentId;
	if (ProperNounActorMention_ptr pm = boost::dynamic_pointer_cast<ProperNounActorMention>(m)) {
		if (_use_awake_db_for_icews) {
			// We don't have externally meaningful codes, so we use names
			// They are not unique, but that's OK
			actorCode = pm->getActorName();
		} else {
			actorCode = pm->getActorCode();
		}
		actorId = pm->getActorId();
		if (_isDictionaryActor && actorId.isNull())
			return PatternFeatureSet_ptr();
		if (_isNotDictionaryActor && !actorId.isNull()) 
			return PatternFeatureSet_ptr();
	} else if (CompositeActorMention_ptr cm = boost::dynamic_pointer_cast<CompositeActorMention>(m)) {
		if (_use_awake_db_for_icews) {
			// See comment above
			actorCode = cm->getPairedActorName();
			agentCode = cm->getPairedAgentName();	
		} else {
			actorCode = cm->getPairedActorCode();
			agentCode = cm->getPairedAgentCode();				
		}
		actorId = cm->getPairedActorId();
		agentId = cm->getPairedAgentId();
		
		if (_isDictionaryActor && (actorId.isNull() || agentId.isNull()))
			return PatternFeatureSet_ptr();
		if (_isNotDictionaryActor && !actorId.isNull() && !agentId.isNull())
			return PatternFeatureSet_ptr();
	}

	// Constraints: actor codes & agent codes
	if ((!_actorCodes.empty()) && (_actorCodes.find(actorCode) == _actorCodes.end()))
		return PatternFeatureSet_ptr();
	if ((!_blockActorCodes.empty()) && (_blockActorCodes.find(actorCode) != _blockActorCodes.end()))
		return PatternFeatureSet_ptr();
	if ((!_agentCodes.empty()) && (_agentCodes.find(agentCode) == _agentCodes.end()))
		return PatternFeatureSet_ptr();
	if ((!_blockAgentCodes.empty()) && (_blockAgentCodes.find(agentCode) != _blockAgentCodes.end()))
		return PatternFeatureSet_ptr();

	// Constraints: sector codes
	if ((!_sectorCodes.empty()) || (!_blockSectorCodes.empty())) {
		std::vector<Symbol> sectors;
		if (boost::dynamic_pointer_cast<ProperNounActorMention>(m))
			sectors = actorInfo->getAssociatedSectorCodes(actorId);
		else if (boost::dynamic_pointer_cast<CompositeActorMention>(m))
			sectors = actorInfo->getAssociatedSectorCodes(agentId);
		if (!_sectorCodes.empty()) {
			bool ok = false; // did we find the sector code we wanted?
			for (std::vector<Symbol>::iterator it=sectors.begin(); it!=sectors.end(); ++it) {
				if (_sectorCodes.find(*it) != _sectorCodes.end()) {
					ok = true;
					break;
				}
			}
			if (!ok) return PatternFeatureSet_ptr();
		}
		if (!_blockSectorCodes.empty()) {
			for (std::vector<Symbol>::iterator it=sectors.begin(); it!=sectors.end(); ++it) {
				if (_blockSectorCodes.find(*it) != _blockSectorCodes.end())
					return PatternFeatureSet_ptr();
			}
		}
	}

	// Constraints: country.  (Note: only proper noun actors can be countries)
	bool isACountry = (!actorId.isNull()) && (agentId.isNull()) && actorInfo->isACountry(actorId);
	if (_mustBeCountry && !isACountry)
		return PatternFeatureSet_ptr();
	if (_mayNotBeCountry && isACountry)
		return PatternFeatureSet_ptr();

	// Constraint: block mention
	if (_blockMention) {
		if (_blockMention->castTo<MentionMatchingPattern>()->matchesMention(patternMatcher, m->getEntityMention(), true)) // allow fall-through
			return PatternFeatureSet_ptr();
	}

	// Constraint: mention
	PatternFeatureSet_ptr mentionSFS;
	if (_mention) {
		mentionSFS = _mention->castTo<MentionMatchingPattern>()->matchesMention(patternMatcher, m->getEntityMention(), true); // allow fall-through
		if (!mentionSFS)
			return PatternFeatureSet_ptr();
	}

	// Build our final result.
	PatternFeatureSet_ptr result = boost::make_shared<PatternFeatureSet>();
	// Add a feature for the return value (if we have one)
	if (getReturn())
		result->addFeature(boost::make_shared<ActorMentionReturnPFeature>(shared_from_this(), m));
	// Add a feature for the pattern itself
	//allFeatures->addFeature(boost::make_shared<PropPFeature>(shared_from_this(), prop, sent_no));
	// Add a feature for the ID (if we have one)
	addID(result);
	// Initialize our score.
	result->setScore(this->getScore());
	// Add in the mention features
	if (mentionSFS) result->addFeatures(mentionSFS);
	return result;
}
