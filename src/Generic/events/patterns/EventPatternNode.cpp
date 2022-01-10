// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/Sexp.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/patterns/EventPatternNode.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include <boost/scoped_ptr.hpp>

Symbol EventPatternNode::PREDICATES = Symbol(L"predicate");
Symbol EventPatternNode::PARTICLE = Symbol(L"particle");
Symbol EventPatternNode::ADVERB = Symbol(L"adverb");
Symbol EventPatternNode::OPTIONAL = Symbol(L"optional");
Symbol EventPatternNode::REQUIRED = Symbol(L"required");
Symbol EventPatternNode::ENTITY_TYPE = Symbol(L"entity_type");
Symbol EventPatternNode::SLOT_TYPE = Symbol(L"slot_type");
Symbol EventPatternNode::EVENT_TYPE = Symbol(L"event_type");
Symbol EventPatternNode::VERB_SYM = Symbol(L"verb");
Symbol EventPatternNode::NOUN_SYM = Symbol(L"noun");
Symbol EventPatternNode::MODIFIER_SYM = Symbol(L"modifier");
Symbol EventPatternNode::OVERRIDING_ENTITY_TYPE = Symbol(L"overriding_entity_type");
EventPatternNode::Map* EventPatternNode::_map;


EventPatternNode::EventPatternNode(Sexp *sexp) :
	_predicateType(NONE), _label(SymbolConstants::nullSymbol),
	_n_etypes(0), _n_predicates(0), _n_required(0), _n_optional(0),
	_particle(SymbolConstants::nullSymbol), _adverb(SymbolConstants::nullSymbol),
	_overriding_entity_type(SymbolConstants::nullSymbol),
	_requiredArguments(0), _requiredRoles(0), _optionalArguments(0),
	_optionalRoles(0)
{
	static bool init = false;
	if (!init) {
		init = true;
		initializeGenericSlots();
	}

	if (sexp->isAtom()) {
		_type = SET;
		_label = sexp->getValue();
		return;
	}

	_type = VOID;

	int nkids = sexp->getNumChildren();

	Symbol event_type = Symbol();
	for (int i = 0; i < nkids; i++) {
		Sexp *slotSexp = sexp->getNthChild(i);
		if (slotSexp->isAtom() || !slotSexp->getFirstChild()->isAtom())
			throwInformativeError("ERROR: ill-formed slot", slotSexp, sexp);

		Symbol trigger = slotSexp->getFirstChild()->getValue();
		if (trigger == OPTIONAL) _n_optional++;
		else if (trigger == REQUIRED) _n_required++;

		if (trigger == EVENT_TYPE) {
			if (slotSexp->getNumChildren() != 2 ||
				!slotSexp->getSecondChild()->isAtom())
				throwInformativeError("ERROR: ill-formed event_type slot", slotSexp, sexp);
			event_type = slotSexp->getSecondChild()->getValue();
		}

		Type new_type = PROP;
		if (trigger == SLOT_TYPE)
			new_type = _type;
		else if (trigger == ENTITY_TYPE ||
			trigger == OVERRIDING_ENTITY_TYPE)
			new_type = ENTITY;

		if (_type == VOID)
			_type = new_type;
		else if (_type != new_type)
			throwInformativeError("ERROR: is this a prop or an entity?", sexp);
	}

	int ngenerics = 0;
	Sexp *generic_slots = 0;
	if (!event_type.is_null()) {
		generic_slots = (*_map)[event_type];
		if (generic_slots != 0) {
			ngenerics = generic_slots->getNumChildren();
			for (int i = 0; i < ngenerics; i++) {
				Sexp *slotSexp = generic_slots->getNthChild(i);
				if (slotSexp->isAtom() || !slotSexp->getFirstChild()->isAtom())
					throwInformativeError("ERROR: ill-formed generic slot", slotSexp, sexp);

				Symbol trigger = slotSexp->getFirstChild()->getValue();
				if (trigger == OPTIONAL) _n_optional++;
				else if (trigger == REQUIRED) _n_required++;
			}
		}
	}

	_requiredArguments = _new EventPatternNode*[_n_required];
	_requiredRoles = _new Symbol*[_n_required];
	_optionalArguments = _new EventPatternNode*[_n_optional];
	_optionalRoles = _new Symbol*[_n_optional];

	_n_optional = 0;
	_n_required = 0;

	for (int j = 0; j < nkids; j++) {
		fillSlot(sexp->getNthChild(j));
	}
	for (int k = 0; k < ngenerics; k++) {
		fillSlot(generic_slots->getNthChild(k));
	}

}

EventPatternNode::~EventPatternNode() {
	delete[] _requiredArguments;
	delete[] _requiredRoles;
	delete[] _optionalArguments;
	delete[] _optionalRoles;
}


void EventPatternNode::fillSlot(Sexp *slotSexp) {
	Symbol trigger = slotSexp->getFirstChild()->getValue();
	int nkids = slotSexp->getNumChildren();

	if (trigger == ENTITY_TYPE) {

		if (nkids != 2)
			throwInformativeError("ERROR: ill-formed entity type slot", slotSexp);

		Sexp *s = slotSexp->getSecondChild();
		if (s->isAtom())
			_n_etypes = 1;
		else _n_etypes = s->getNumChildren();

		_etypes = _new Symbol[_n_etypes];

		if (s->isAtom())
			_etypes[0] = s->getValue();
		else {
			for (int i = 0; i < _n_etypes; i++) {
				Sexp *child = s->getNthChild(i);
				if (!child->isAtom())
					throwInformativeError("ERROR: ill-formed child in slot", child, slotSexp);
				_etypes[i] = child->getValue();
			}
		}

	} else if (trigger == PREDICATES) {

		if (nkids != 3)
			throwInformativeError("ERROR: ill-formed predicate slot", slotSexp);

		Sexp *type = slotSexp->getSecondChild();
		if (!type->isAtom())
			throwInformativeError("ERROR: ill-formed predicate slot", slotSexp);
		if (type->getValue() == VERB_SYM)
			_predicateType = VERB;
		else if (type->getValue() == NOUN_SYM)
			_predicateType = NOUN;
		else if (type->getValue() == MODIFIER_SYM)
			_predicateType = MODIFIER;
		else throwInformativeError("ERROR: ill-formed predicate slot", slotSexp);

		Sexp *s = slotSexp->getThirdChild();
		if (s->isAtom())
			_n_predicates = 1;
		else _n_predicates = s->getNumChildren();

		_predicates = _new Symbol[_n_predicates];

		if (s->isAtom())
			_predicates[0] = s->getValue();
		else {
			for (int i = 0; i < _n_predicates; i++) {
				Sexp *child = s->getNthChild(i);
				if (!child->isAtom())
					throwInformativeError("ERROR: ill-formed child in slot", child, slotSexp);
				_predicates[i] = child->getValue();
			}
		}

	} else if (trigger == SLOT_TYPE || trigger == EVENT_TYPE) {

		if (nkids != 2 || !slotSexp->getSecondChild()->isAtom())
			throwInformativeError("ERROR: ill-formed slot_type or event_type slot", slotSexp);
		_label = slotSexp->getSecondChild()->getValue();

	} else if (trigger == OVERRIDING_ENTITY_TYPE) {

		if (nkids != 2 || !slotSexp->getSecondChild()->isAtom())
			throwInformativeError("ERROR: ill-formed overriding_entity_type slot", slotSexp);
		_overriding_entity_type = slotSexp->getSecondChild()->getValue();

	} else if (trigger == REQUIRED) {
		if (nkids != 3)
			throwInformativeError("ERROR: ill-formed required slot", slotSexp);
		Sexp *role = slotSexp->getSecondChild();
		if (role->isAtom()) {
			_requiredRoles[_n_required] = _new Symbol[2];
			_requiredRoles[_n_required][0] = role->getValue();
			_requiredRoles[_n_required][1] = SymbolConstants::nullSymbol;
		} else {
			int nroles = role->getNumChildren();
			_requiredRoles[_n_required] = _new Symbol[nroles + 1];
			for (int i = 0; i < nroles; i++) {
				Sexp *child = role->getNthChild(i);
				if (!child->isAtom())
					throwInformativeError("ERROR: ill-formed child in role slot", child, slotSexp);
				_requiredRoles[_n_required][i] = child->getValue();
			}
			_requiredRoles[_n_required][nroles] = SymbolConstants::nullSymbol;
		}
		_requiredArguments[_n_required] = _new EventPatternNode(slotSexp->getThirdChild());
		_n_required++;
	} else if (trigger == OPTIONAL) {
		if (nkids != 3)
			throwInformativeError("ERROR: ill-formed optional slot", slotSexp);
		Sexp *role = slotSexp->getSecondChild();
		if (role->isAtom()) {
			_optionalRoles[_n_optional] = _new Symbol[2];
			_optionalRoles[_n_optional][0] = role->getValue();
			_optionalRoles[_n_optional][1] = SymbolConstants::nullSymbol;
		} else {
			int nroles = role->getNumChildren();
			_optionalRoles[_n_optional] = _new Symbol[nroles + 1];
			for (int i = 0; i < nroles; i++) {
				Sexp *child = role->getNthChild(i);
				if (!child->isAtom())
					throwInformativeError("ERROR: ill-formed child in role slot", child, slotSexp);
				_optionalRoles[_n_optional][i] = child->getValue();
			}
			_optionalRoles[_n_optional][nroles] = SymbolConstants::nullSymbol;
		}
		_optionalArguments[_n_optional] = _new EventPatternNode(slotSexp->getThirdChild());
		_n_optional++;
	} else if (trigger == PARTICLE || trigger == ADVERB) {
		if (nkids != 2)
			throwInformativeError("ERROR: ill-formed particle/adverb slot", slotSexp);
		Sexp *part = slotSexp->getSecondChild();
		if (part->isAtom()) {
			if (trigger == PARTICLE)
				_particle = part->getValue();
			else _adverb =  part->getValue();
		} else throwInformativeError("ERROR: ill-formed particle/adverb slot", slotSexp);
	} else {
		throwInformativeError("ERROR: unknown slot", slotSexp);
	}
}

void EventPatternNode::throwInformativeError(const char* error, Sexp *sexp, Sexp *sexp2)
{
	char c[1000];
	if (sexp2 == 0)
		sprintf(c, "%s: %s", error, sexp->to_debug_string().c_str());
	else sprintf(c, "%s: %s\n...in %s", error, sexp->to_debug_string().c_str(),
			sexp2->to_debug_string().c_str());
	throw UnexpectedInputException("EventPatternNode::??", c);

}


void EventPatternNode::initializeGenericSlots()
{
	_map = _new Map(100);
	std::string slots = ParamReader::getParam("generic_event_slots");
	if (!slots.empty()) {
		boost::scoped_ptr<UTF8InputStream> gs_stream_scoped_ptr(UTF8InputStream::build(slots.c_str()));
		UTF8InputStream& gs_stream(*gs_stream_scoped_ptr);
		Sexp *slot = _new Sexp(gs_stream);
		while (!slot->isVoid()) {
			if (slot->getNumChildren() != 2 ||
				!slot->getFirstChild()->isList() ||
				!slot->getSecondChild()->isList())
				throwInformativeError("ERROR: ill-formed generic slot", slot);
			Sexp *types = slot->getFirstChild();
			Sexp *value = slot->getSecondChild();
			int ntypes = types->getNumChildren();
			for (int i = 0; i < ntypes; i++) {
				Sexp *type = types->getNthChild(i);
				if (!type->isAtom())
					throwInformativeError("ERROR: ill-formed type in generic slot", slot);
				(*_map)[type->getValue()] = _new Sexp(*value);
			}

			delete slot;
			slot = _new Sexp(gs_stream);
		}
		delete slot;
	}
}

EntityType EventPatternNode::getOverridingEntityType() const {
	if (hasOverridingEntityType())
		return EntityType(_overriding_entity_type);
	else return EntityType::getOtherType();
}
bool EventPatternNode::hasOverridingEntityType() const {
	return _overriding_entity_type != SymbolConstants::nullSymbol;
}
