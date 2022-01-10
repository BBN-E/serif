// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/events/patterns/EventPatternMatcher.h"
#include "Generic/events/patterns/EventPatternNode.h"
#include "Generic/events/EventUtilities.h"
#include "Generic/events/patterns/EventPatternSet.h"
#include "Generic/events/patterns/EventSearchNode.h"
#include "Generic/events/patterns/DeprecatedPatternEventFinder.h"
#include "Generic/relations/xx_RelationUtilities.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/ParamReader.h"

#include "Generic/CASerif/correctanswers/CorrectAnswers.h"
#include <set>

#define MAX_STACK_DEPTH 500
static Symbol crimeNounSym = Symbol(L"CRIME_NOUN");
static Symbol anyEventSym = Symbol(L"ANY_EVENT");
static Symbol whereEventSym = Symbol(L"EVENT_THAT_TAKES_WHERE");

EventPatternMatcher::EventPatternMatcher(const EventPatternSet* set) : _patternSet(set)
{
	ALLOW_TYPE_OVERRIDING = ParamReader::isParamTrue("coerce_types_for_events_and_relations");
}

EventPatternMatcher::~EventPatternMatcher() {
	delete _patternSet;
}


void EventPatternMatcher::resetForNewSentence(const PropositionSet *props,
											  MentionSet *mentions, ValueMentionSet *values)
{
	_mentionSet = mentions;
	_valueSet = values;
	collectDefinitions(props);
	setMentionsAsValues();
}


void EventPatternMatcher::setMentionsAsValues() {
	for (int mentid = 0; mentid < _mentionSet->getNMentions(); mentid++) {
		_mentionsAsValues[mentid] = 0;
		const Mention *mention = _mentionSet->getMention(mentid);
		for (int valid = 0; valid < _valueSet->getNValueMentions(); valid++) {
			if (mention->getNode()->getStartToken() == _valueSet->getValueMention(valid)->getStartToken() &&
				mention->getNode()->getEndToken() == _valueSet->getValueMention(valid)->getEndToken())
			{
				_mentionsAsValues[mentid] = _valueSet->getValueMention(valid);
				break;
			}
		}
	}
}


Symbol EventPatternMatcher::getEntityOrValueType(const Mention *mention) {
	if (mention->getEntityType().isRecognized())
		return mention->getEntityType().getName();
	if (_mentionsAsValues[mention->getIndex()] != 0)
		return _mentionsAsValues[mention->getIndex()]->getFullType().getNicknameSymbol();
	return mention->getEntityType().getName();	
}

EventSearchNode *EventPatternMatcher::matchAllPossibleNodes(const Proposition *prop, Symbol sym) {

	int n_nodes;
	EventPatternNode **nodeSet = _patternSet->getNodes(sym, n_nodes);

	EventSearchNode *result = 0;
	EventSearchNode *iter = 0;
	for (int i = 0; i < n_nodes; i++) {
		_stack_depth = 0;
		EventSearchNode *temp = matchNode(prop, nodeSet[i]);
		if (temp != 0) {
			if (result == 0) {
				result = temp;
				iter = result;
			} else {
				iter->setNext(temp);
				iter = iter->getNext();
			}
		}
	}

	return result;

}

EventSearchNode *EventPatternMatcher::matchNode(const Proposition *prop, Symbol sym) {

	_stack_depth++;
	if (_stack_depth > MAX_STACK_DEPTH)
		return 0;

	int n_nodes;
	EventPatternNode **nodeSet = _patternSet->getNodes(sym, n_nodes);

	for (int i = 0; i < n_nodes; i++) {
		EventSearchNode *result = matchNode(prop, nodeSet[i]);
		if (result != 0) {
			_stack_depth--;
			return result;
		}
	}

/*if (ParamReader::isParamTrue("use_correct_answers")) {
	if (prop->getPredHead() != 0) {
		if (sym == crimeNounSym ||
			sym == anyEventSym ||
			sym == whereEventSym)
		{
			Symbol etype = PatternEventFinder::_correctAnswers->getEventType(
				PatternEventFinder::_sentence_num,
				prop->getPredHead()->getHeadPreterm()->getStartToken(),
				PatternEventFinder::_docName);
			if (!etype.is_null()) {
				EventSearchNode *result = new EventSearchNode(prop);
				result->setLabel(PatternEventFinder::FORCED_SYM);
				if (PatternEventFinder::DEBUG) {
					PatternEventFinder::_debugStream << L"crime noun argument added:";
					PatternEventFinder::_debugStream << prop->getPredHead()->toTextString();
					PatternEventFinder::_debugStream << L"\n";
				}
				_stack_depth--;
				return result;
			}
		}
	}
  }*/

	_stack_depth--;
	return 0;
}

EventSearchNode *EventPatternMatcher::matchNode(const Mention *ment, Symbol sym) {

	_stack_depth++;
	if (_stack_depth > MAX_STACK_DEPTH)
		return 0;

	int n_nodes;
	EventPatternNode **nodeSet = _patternSet->getNodes(sym, n_nodes);

	for (int i = 0; i < n_nodes; i++) {
		EventSearchNode *result = matchNode(ment, nodeSet[i]);
		if (result != 0) {
			_stack_depth--;
			return result;
		}
	}

/*if (ParamReader::isParamTrue("use_correct_answers")) {
	Proposition *prop = _definingProps[ment->getIndex()];
	if (prop != 0 && prop->getPredHead() != 0 &&
		(sym == crimeNounSym ||
			sym == anyEventSym ||
			sym == whereEventSym))
	{
		Symbol etype = PatternEventFinder::_correctAnswers->getEventType(
			PatternEventFinder::_sentence_num,
			prop->getPredHead()->getHeadPreterm()->getStartToken(),
			PatternEventFinder::_docName);
		if (!etype.is_null()) {
			EventSearchNode *result = new EventSearchNode(prop);
			result->setLabel(PatternEventFinder::FORCED_SYM);
			if (PatternEventFinder::DEBUG) {
				PatternEventFinder::_debugStream << L"crime noun argument added:";
				PatternEventFinder::_debugStream << prop->getPredHead()->toTextString();
				PatternEventFinder::_debugStream << L"\n";
			}
			_stack_depth--;
			return result;
		}
	}
  }*/
	_stack_depth--;
	return 0;

}

EventSearchNode *EventPatternMatcher::matchNode(const Mention *ment, const EventPatternNode *node) {
	_stack_depth++;
	if (_stack_depth > MAX_STACK_DEPTH)
		return 0;

	if (node->isProp()) {
		EventSearchNode *result = 0;
		if (_definingProps[ment->getIndex()] != 0)
			result = matchNode(_definingProps[ment->getIndex()], node);
		else if (_definingSetProps[ment->getIndex()] != 0)
			result = matchNode(_definingSetProps[ment->getIndex()], node);
		_stack_depth--;
		return result;
	}

	if (node->isSet()) {
		EventSearchNode *result = matchNode(ment, node->getLabel());
		_stack_depth--;
		return result;
	}

	Proposition *set = _definingSetProps[ment->getIndex()];
	if (set != 0) {
		// don't set label on top node, since it's just a set
		EventSearchNode *result = new EventSearchNode(ment);
		bool found = false;
		for (int j = 1; j < set->getNArgs(); j++) {
			Argument *arg = set->getArg(j);
			if (arg->getType() == Argument::MENTION_ARG) {
				EventSearchNode *child = matchNode(arg->getMention(_mentionSet), node);
				// child's label is already set in matchNode
				if (child != 0) {
					result->setChild(child);
					found = true;
				}
			}
		}
		if (found) {
			_stack_depth--;
			return result;
		} else {
			delete result;
			_stack_depth--;
			return 0;
		}
	} else {
		int n_entity_types = node->getNEntityTypes();
		Symbol myType = getEntityOrValueType(ment);
		Symbol *etypes = node->getEntityTypes();
		for (int i = 0; i < n_entity_types; i++) {
			if (myType == etypes[i]) {
				EventSearchNode *result = new EventSearchNode(ment);
				result->setLabel(node->getLabel());
				_stack_depth--;
				return result;
			}
		}
		if (!ment->getEntityType().isRecognized() && ALLOW_TYPE_OVERRIDING) {
			if (node->hasOverridingEntityType() &&
				RelationUtilities::get()->coercibleToType(ment, 
				node->getOverridingEntityType().getName()))
			{
				EventSearchNode *result = new EventSearchNode(ment);
				result->setLabel(node->getLabel());
				result->setOverridingEntityType(node->getOverridingEntityType());
				_stack_depth--;
				return result;
			} else if (ment->getMentionType() == Mention::DESC ||
					   ment->getMentionType() == Mention::PRON)
			{
				/*for (int i = 0; i < n_entity_types; i++) {
					if (RelationUtilities::get()->coercibleToType(ment, etypes[i])) {
						EventSearchNode *result = new EventSearchNode(ment);
						result->setLabel(node->getLabel());
						result->setOverridingEntityType(EntityType(etypes[i]));
						_stack_depth--;
						return result;
					}
				}*/
			}
		}
		if (n_entity_types == 0) {
			EventSearchNode *result = new EventSearchNode(ment);
			result->setLabel(node->getLabel());
			_stack_depth--;
			return result;
		}

	}

	_stack_depth--;
	return 0;

}

EventSearchNode *EventPatternMatcher::matchNode(const Proposition *prop,
												const EventPatternNode *node)
{
	_stack_depth++;
	if (_stack_depth > MAX_STACK_DEPTH)
		return 0;

	if (node->isSet()) {
		EventSearchNode *result = matchNode(prop, node->getLabel());
		_stack_depth--;
		return result;
	}

	if (prop->getPredType() == Proposition::SET_PRED) {
		EventSearchNode *result = new EventSearchNode(prop);
		bool found = false;
		const Mention *set = 0;
		if (prop->getArg(0)->getType() == Argument::MENTION_ARG) {
			 set = prop->getArg(0)->getMention(_mentionSet);
		}
		for (int j = 1; j < prop->getNArgs(); j++) {
			Argument *arg = prop->getArg(j);
			EventSearchNode *child = 0;
			if (arg->getType() == Argument::MENTION_ARG) {
				if (arg->getMention(_mentionSet) != set)
					child = matchNode(arg->getMention(_mentionSet), node);
			} else if (arg->getType() == Argument::PROPOSITION_ARG)
				if (arg->getProposition() != prop)
					child = matchNode(arg->getProposition(), node);
			if (child != 0) {
				found = true;
				result->setChild(child);
			}
		}
		_stack_depth--;
		if (!found) {
			delete result;
			return 0;
		} else return result;
	}

	if ((node->hasNounPredicate() && prop->getPredType() != Proposition::NOUN_PRED) ||
		(node->hasVerbPredicate() && prop->getPredType() != Proposition::VERB_PRED &&
		 prop->getPredType() != Proposition::COPULA_PRED) ||
		 (node->hasModifierPredicate() && prop->getPredType() != Proposition::MODIFIER_PRED))
	{
		_stack_depth--;
		return 0;
	}

	if (node->isEntity()) {
		// no modifier junk
		if (prop->getArg(0)->getRoleSym() == Argument::REF_ROLE &&
			(prop->getPredType() == Proposition::NOUN_PRED ||
			prop->getPredType() == Proposition::NAME_PRED ||
			prop->getPredType() == Proposition::PRONOUN_PRED)) 
		{
			EventSearchNode *result = matchNode(prop->getArg(0)->getMention(_mentionSet), node);
			_stack_depth--;
			return result;
		} else {
			_stack_depth--;
			return 0;
		}
	}

	int n_predicates = node->getNPredicates();
	if (n_predicates != 0) {
		Symbol my_predicate;
		if (node->hasNounPredicate())
			my_predicate = EventUtilities::getStemmedNounPredicate(prop);
		else if (node->hasVerbPredicate())
			my_predicate = EventUtilities::getStemmedVerbPredicate(prop);
		else my_predicate = prop->getPredSymbol();
		Symbol *predicates = node->getPredicates();
		bool matched = false;
		for (int i = 0; i < n_predicates; i++) {
			if (my_predicate == predicates[i])
				matched = true;
		}
		if (!matched) {
			_stack_depth--;
			return 0;
		}
	}

	Symbol particle = node->getParticle();
	if (particle != SymbolConstants::nullSymbol) {
		if ((prop->getParticle() == 0 ||
			 prop->getParticle()->getHeadWord() != particle) &&
			(prop->getAdverb() == 0 ||
			 prop->getAdverb()->getHeadWord() != particle))
		{
			_stack_depth--;
			return 0;
		}
	}

	Symbol adverb = node->getAdverb();
	if (adverb != SymbolConstants::nullSymbol) {
		if (prop->getAdverb() == 0 ||
			prop->getAdverb()->getHeadWord() != adverb)
		{
			_stack_depth--;
			return 0;
		}
	}

	EventSearchNode *result = new EventSearchNode(prop);

	// When does this happen?
	if (prop->getNArgs() > (int)MentionUID_tag::max_items_per_sentence()) {
		_stack_depth--;
		return 0;
	}

	std::set<int> usedArguments;
	for (int i = 0; i < node->getNRequiredArguments(); i++) {
		Symbol *roles = node->getNthRequiredRoles(i);
		bool matched = false;
		for (int j = 0; j < prop->getNArgs(); j++) {
			if (usedArguments.find(j) != usedArguments.end()) continue;
			Argument *arg = prop->getArg(j);
			Symbol my_role = arg->getRoleSym();
			for (int k = 0; roles[k] != SymbolConstants::nullSymbol; k++) {
				if (my_role == roles[k])
					matched = true;
			}
			if (!matched) continue;

			matched = false;
			EventSearchNode *child;
			if (arg->getType() == Argument::MENTION_ARG)
				child = matchNode(arg->getMention(_mentionSet), node->getNthRequiredArgument(i));
			else if (arg->getType() == Argument::PROPOSITION_ARG)
				child = matchNode(arg->getProposition(), node->getNthRequiredArgument(i));
			else child = 0;

			if (child != 0) {
				result->setChild(child);
				matched = true;
				usedArguments.insert(j);
				break;
			}
		}
		// failed to find a match for i-th required argument
		if (!matched) {
			_stack_depth--;
			delete result;
			return 0;
		}
	}

	for (int l = 0; l < node->getNOptionalArguments(); l++) {
		Symbol *roles = node->getNthOptionalRoles(l);
		for (int j = 0; j < prop->getNArgs(); j++) {
			if (usedArguments.find(j) != usedArguments.end()) continue;
			Argument *arg = prop->getArg(j);
			Symbol my_role = arg->getRoleSym();
			bool matched = false;
			for (int k = 0; roles[k] != SymbolConstants::nullSymbol; k++) {
				if (my_role == roles[k]) {
					matched = true;
					break;
				}
			}
			if (!matched) continue;

			EventSearchNode *child;
			if (arg->getType() == Argument::MENTION_ARG)
				child = matchNode(arg->getMention(_mentionSet), node->getNthOptionalArgument(l));
			else if (arg->getType() == Argument::PROPOSITION_ARG)
				child = matchNode(arg->getProposition(), node->getNthOptionalArgument(l));
			else child = 0;

			if (child != 0) {
				result->setChild(child);
				usedArguments.insert(j);
				break;
			}
		}
	}

	result->setLabel(node->getLabel());
	_stack_depth--;
	return result;
}


void EventPatternMatcher::collectDefinitions(const PropositionSet *propSet) {
	_definingProps.clear();
	_definingSetProps.clear();

	for (int j = 0; j < propSet->getNPropositions(); j++) {
		Proposition *prop = propSet->getProposition(j);
		if (prop->getPredType() == Proposition::NOUN_PRED ||
			prop->getPredType() == Proposition::PRONOUN_PRED)
		{
			int index = prop->getArg(0)->getMentionIndex();
			_definingProps[index] = prop;

		} else if (prop->getPredType() == Proposition::SET_PRED) {
			int index = prop->getArg(0)->getMentionIndex();
			_definingSetProps[index] = prop;

		}
	}

	// if we can't find a noun prop, use a modifier prop
	for (int k = 0; k < propSet->getNPropositions(); k++) {
		Proposition *prop = propSet->getProposition(k);
		if (prop->getPredType() == Proposition::MODIFIER_PRED ||
			prop->getPredType() == Proposition::NAME_PRED)
		{
			int index = prop->getArg(0)->getMentionIndex();
			if (_definingProps[index] == 0)
				_definingProps[index] = prop;

		}
	}
}
