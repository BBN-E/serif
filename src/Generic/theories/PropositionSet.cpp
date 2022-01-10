// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/limits.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/MentionSet.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"
#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

PropositionSet::PropositionSet(const MentionSet* mentionSet)
	: _mentionSet(mentionSet),
	_defArrayUpdated(false)
{}

PropositionSet::PropositionSet(const PropositionSet &other, int sent_offset, const MentionSet* mentionSet)
: _mentionSet(mentionSet), _defArrayUpdated(false)
{
	// First pass: deep copy the propositions and their arguments, exception PROPOSITION_ARGs
	BOOST_FOREACH(Proposition* prop, other._props) {
		_props.push_back(_new Proposition(*prop, sent_offset, _mentionSet->getParse()));
	}

	// Second pass: update the pointers for PROPOSITION_ARGs
	for (size_t p = 0; p < _props.size(); p++) {
		for (int a = 0; a < _props[p]->getNArgs(); a++) {
			Argument* arg = _props[p]->getArg(a);
			if (arg->getType() == Argument::PROPOSITION_ARG) {
				arg->populateWithProposition(arg->getRoleSym(), findPropositionByUID(arg->getProposition()->getID() + sent_offset*MAX_SENTENCE_PROPS));
			}
		}
	}
}

PropositionSet::~PropositionSet() {
	BOOST_FOREACH(Proposition *prop, _props) {
		delete prop;
	}
}

void PropositionSet::updateDefinitionsArray() const {
	_definitions.clear();
	for (int j = 0; j < getNPropositions(); j++) {
		if (_props[j]->getPredType() == Proposition::NOUN_PRED ||
			_props[j]->getPredType() == Proposition::PRONOUN_PRED ||
			_props[j]->getPredType() == Proposition::SET_PRED) {
			if (_props[j]->getArg(0)->getType() == Argument::MENTION_ARG) {
				SessionLogger::dbg("PropositionSet") << "PropositionSet::updateDefinitionsArray(): mention " << _props[j]->getArg(0)->getMentionIndex() << " -> prop " << j;
				_definitions[_props[j]->getArg(0)->getMentionIndex()] = _props[j];
			}
		}
	}
	for (int k = 0; k < getNPropositions(); k++) {
		if (_props[k]->getPredType() == Proposition::NAME_PRED &&
			_props[k]->getArg(0)->getType() == Argument::MENTION_ARG &&
			_definitions[_props[k]->getArg(0)->getMentionIndex()] == 0) 
		{			
			SessionLogger::dbg("PropositionSet") << "PropositionSet::updateDefinitionsArray(): mention " << _props[k]->getArg(0)->getMentionIndex() << " -> prop " << k;
			_definitions[_props[k]->getArg(0)->getMentionIndex()] = _props[k];
		}
	}
	for (int k = 0; k < getNPropositions(); k++) {
		if (_props[k]->getPredType() == Proposition::MODIFIER_PRED &&
			_props[k]->getNArgs() > 1 &&
			_props[k]->getArg(0)->getType() == Argument::MENTION_ARG &&
			_definitions[_props[k]->getArg(0)->getMentionIndex()] == 0) 
		{		
			SessionLogger::dbg("PropositionSet") << "PropositionSet::updateDefinitionsArray(): mention " << _props[k]->getArg(0)->getMentionIndex() << " -> prop " << k;
			_definitions[_props[k]->getArg(0)->getMentionIndex()] = _props[k];
		}
	}
	for (int k = 0; k < getNPropositions(); k++) {
		if (_props[k]->getPredType() == Proposition::MODIFIER_PRED &&
			_props[k]->getArg(0)->getType() == Argument::MENTION_ARG &&
			_definitions[_props[k]->getArg(0)->getMentionIndex()] == 0) 
		{			
			SessionLogger::dbg("PropositionSet") << "PropositionSet::updateDefinitionsArray(): mention " << _props[k]->getArg(0)->getMentionIndex() << " -> prop " << k;
			_definitions[_props[k]->getArg(0)->getMentionIndex()] = _props[k];
		}
	}
	_defArrayUpdated = true;
}

Proposition* PropositionSet::getDefinition(int index) const {
	if (!_defArrayUpdated) updateDefinitionsArray();
	std::map<int,Proposition*>::const_iterator it = _definitions.find(index);
	if (it == _definitions.end()) {
		return 0;
	} else {
		return it->second;
	}
}

void PropositionSet::setMentionSet(const MentionSet* mentionSet) {
	if (_mentionSet != 0 && mentionSet != _mentionSet)
		throw InternalInconsistencyException("PropositionSet::setMentionSet",
			"MentionSet is already set!");
	_mentionSet = mentionSet;
	_defArrayUpdated = false;
}

void PropositionSet::replaceMentionSet(const MentionSet* mentionSet) {
	if (_mentionSet == 0)
		throw InternalInconsistencyException("PropositionSet::replaceMentionSet",
			"No mention set to replace!");
	if (_mentionSet->getNMentions() != mentionSet->getNMentions())
		throw InternalInconsistencyException("PropositionSet::replaceMentionSet",
			"Replacement mention set is incompatible (different number of mentions)");
	if (_mentionSet->getSentenceNumber() != mentionSet->getSentenceNumber())
		throw InternalInconsistencyException("PropositionSet::replaceMentionSet",
			"Replacement mention set is incompatible (different sentence number)");
	_mentionSet = mentionSet;
	_defArrayUpdated = false;
}

void PropositionSet::takeProposition(Proposition *prop) {
	if (static_cast<int>(_props.size()) < MAX_SENTENCE_PROPS) {
		_props.push_back(prop);
		_defArrayUpdated = false;
	}
	else {
		SessionLogger::warn("max_sentence_propositions") << "Number of propositions in sentence exceeds limit of " << MAX_SENTENCE_PROPS;
	}
}

Proposition *PropositionSet::getProposition(int i) const {
	if (i < getNPropositions())
		return _props[i];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"PropositionSet::getProposition()", getNPropositions(), i);
}


Proposition *PropositionSet::findPropositionByUID(int uid) const {
	BOOST_FOREACH(Proposition *prop, _props) {
		if (prop->getID() == uid)
			return prop;
	}
	return 0;
}

Proposition *PropositionSet::findPropositionByNode(const SynNode *node) const {
	BOOST_FOREACH(Proposition *prop, _props) {
		if (prop->getPredHead() == node)
			return prop;
	}
	BOOST_FOREACH(Proposition *prop, _props) {
		if (prop->getPredHead() != 0 && prop->getPredHead()->getHeadPreterm() == node->getHeadPreterm())
			return prop;
	}
	return 0;
}


void PropositionSet::dump(std::ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "Proposition Set:";

	if (_props.empty()) {
		out << newline << "  (no propositions)";
	}
	else {
		BOOST_FOREACH(Proposition *prop, _props) {
			out << newline << "- ";
			prop->dump(out, indent + 2);
		}
	}

	delete[] newline;
}

void PropositionSet::dump(UTF8OutputStream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << L"Proposition Set:";

	if (_props.empty()) {
		out << newline << L"  (no propositions)";
	}
	else {
		BOOST_FOREACH(Proposition *prop, _props) {
			out << newline << L"- ";
			prop->dump(out, indent + 2);
		}
	}

	delete[] newline;
}


void PropositionSet::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);

	BOOST_FOREACH(Proposition *prop, _props)
		prop->updateObjectIDTable();
}

void PropositionSet::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"PropositionSet", this);
	if (stateSaver->getVersion() >= std::make_pair(1,6))
		stateSaver->savePointer(_mentionSet);

	stateSaver->saveInteger(getNPropositions());
	stateSaver->beginList(L"PropositionSet::_props");
	BOOST_FOREACH(Proposition *prop, _props)
		prop->saveState(stateSaver);
	stateSaver->endList();

	//stateSaver->saveInteger(_first_prop_ID);
	// still need to save an int for backwards compatibility.
	stateSaver->saveInteger(-1);

	stateSaver->endList();
}

PropositionSet::PropositionSet(StateLoader *stateLoader)
: _mentionSet(0)
{
	_defArrayUpdated = false;

	int id = stateLoader->beginList(L"PropositionSet");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	if (stateLoader->getVersion() >= std::make_pair(1,6)) 
		_mentionSet = static_cast<MentionSet*>(stateLoader->loadPointer());

	int n_props = stateLoader->loadInteger();
	stateLoader->beginList(L"PropositionSet::_props");
	for (int i = 0; i < n_props; i++)
		_props.push_back(_new Proposition(stateLoader));
	stateLoader->endList();

	//_first_prop_ID = stateLoader->loadInteger();
	// still need to read an int for backwards compatibility.
	stateLoader->loadInteger(); 

	stateLoader->endList();
}

void PropositionSet::resolvePointers(StateLoader * stateLoader) {
	if (stateLoader->getVersion() >= std::make_pair(1,6)) 
		_mentionSet = static_cast<MentionSet*>(stateLoader->getObjectPointerTable().getPointer(_mentionSet));
	BOOST_FOREACH(Proposition *prop, _props)
		prop->resolvePointers(stateLoader);
}

const wchar_t* PropositionSet::XMLIdentifierPrefix() const {
	return L"propset";
}

namespace {
	inline bool prop_id_less_than(const Proposition* lhs, const Proposition* rhs) {
		return lhs->getID() < rhs->getID(); 
	}
}

void PropositionSet::saveXML(SerifXML::XMLTheoryElement propositionsetElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("PropositionSet::saveXML()", "Expected context to be NULL");
	propositionsetElem.saveTheoryPointer(X_mention_set_id, _mentionSet);
	// Assign an id to each proposition before we serialize any of them.
	BOOST_FOREACH(Proposition *prop, _props)
		propositionsetElem.generateChildId(prop);

	// Sort the propositions by id (so we don't need to explicitly record the id).
	// JSM 11/19/10 - I'm commenting this out for now because the order of the 
	// propositions affects the behavior of the MetonymyAdder
	//std::vector<Proposition*> sorted_props(_props, _props+_n_props);
	//std::sort(sorted_props.begin(), sorted_props.end(), prop_id_less_than);
	
	BOOST_FOREACH(Proposition *prop, _props) {
		propositionsetElem.saveChildTheory(X_Proposition, prop, _mentionSet); 
		//if ((sorted_props[j]->getID() % MAX_SENTENCE_PROPS) != j)
		//	throw InternalInconsistencyException("PropositionSet::saveXML", 
		//		"Unexpected Proposition::_ID value");
	}
	// No need to serialize _definitions -- just set _defArrayUpdated to false
	// when we deserialize, and we'll reconstruct it if/when necessary.
}

PropositionSet::PropositionSet(SerifXML::XMLTheoryElement propSetElem, int sentence_number)
: _defArrayUpdated(false), _mentionSet(0)
{
	using namespace SerifXML;
	propSetElem.loadId(this);
	_mentionSet = propSetElem.loadTheoryPointer<MentionSet>(X_mention_set_id);
	if (_mentionSet == 0)
		propSetElem.reportLoadError("Expected a mention_set_id");

	XMLTheoryElementList propElems = propSetElem.getChildElementsByTagName(X_Proposition);
	int n_props = static_cast<int>(propElems.size());
	for (int i=0; i<n_props; ++i)
		_props.push_back(_new Proposition(propElems[i], i + sentence_number*MAX_SENTENCE_PROPS));
	for (int i=0; i<n_props; ++i)
		_props[i]->resolvePointers(propElems[i]);
}
