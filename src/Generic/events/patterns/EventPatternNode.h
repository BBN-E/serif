// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_PATTERN_NODE_H
#define EVENT_PATTERN_NODE_H

#include "Generic/common/Symbol.h"
#include <string>
#include "Generic/common/hash_map.h"
#include "Generic/theories/EntityType.h"
class Sexp;

class EventPatternNode {
public:
	EventPatternNode(Sexp *sexp);
	~EventPatternNode();

	bool isSet() const { return _type == SET; }
	bool isProp() const { return _type == PROP; }
	bool isEntity() const { return _type == ENTITY; }
	bool isVoid() const { return _type == VOID; }

	bool hasNounPredicate() const { return _predicateType == NOUN; }
	bool hasVerbPredicate() const { return _predicateType == VERB; }
	bool hasModifierPredicate() const { return _predicateType == MODIFIER; }

	EntityType getOverridingEntityType() const; 
	bool hasOverridingEntityType() const;

	Symbol getLabel() const { return _label; }
	Symbol getParticle() const { return _particle; }
	Symbol getAdverb() const { return _adverb; }
	int getNPredicates() const { return _n_predicates; }
	Symbol *getPredicates() const {	return _predicates; }
	int getNEntityTypes() const { return _n_etypes; }
	Symbol *getEntityTypes() const {	return _etypes; }
	
	int getNRequiredArguments() const {	return _n_required;	}
	Symbol *getNthRequiredRoles(int n) const { return _requiredRoles[n]; }
	EventPatternNode *getNthRequiredArgument(int n) const { return _requiredArguments[n]; }
	
	int getNOptionalArguments() const {	return _n_optional;	}
	Symbol *getNthOptionalRoles(int n) const { return _optionalRoles[n]; }
	EventPatternNode *getNthOptionalArgument(int n) const { return _optionalArguments[n]; }

private:
	typedef enum { SET, PROP, ENTITY, VOID } Type;
	typedef enum { NOUN, VERB, MODIFIER, NONE } PredicateType;
	Type _type;

	// either relation type, event type, or set type
	Symbol _label;
	
	// ENTITY
	Symbol *_etypes;
	int _n_etypes;
	Symbol _overriding_entity_type;
	
	// PROP
	Symbol *_predicates;
	int _n_predicates;
	PredicateType _predicateType;
	Symbol _particle;
	Symbol _adverb;

	EventPatternNode **_requiredArguments;
	Symbol **_requiredRoles;
	int _n_required;

	EventPatternNode **_optionalArguments;
	Symbol **_optionalRoles;
	int _n_optional;

	void fillSlot(Sexp *slotSexp);
	void throwInformativeError(const char* error, Sexp *sexp, Sexp *sexp2 = 0);

	static Symbol PREDICATES;
	static Symbol PARTICLE;
	static Symbol ADVERB;
	static Symbol OPTIONAL;
	static Symbol REQUIRED;
	static Symbol ENTITY_TYPE;
	static Symbol SLOT_TYPE;
	static Symbol EVENT_TYPE;
	static Symbol VERB_SYM;
	static Symbol NOUN_SYM;
	static Symbol MODIFIER_SYM;
	static Symbol OVERRIDING_ENTITY_TYPE;


	struct HashKey {
		size_t operator()(const Symbol& s) const { return s.hash_code(); }
	};
    struct EqualKey {
        bool operator()(const Symbol& s1, const Symbol& s2) const { return s1 == s2; }
    };

	typedef serif::hash_map<Symbol, Sexp *, HashKey, EqualKey> Map;
	static Map* _map;
	void initializeGenericSlots();

};

#endif
