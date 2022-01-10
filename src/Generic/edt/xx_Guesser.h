// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_GUESSER_H
#define xx_GUESSER_H

#include "Generic/common/Symbol.h"

class SynNode;
class Mention;
class EntitySet;
class Entity;

class GenericGuesser {
	// Note: this class is intentionally not a subclass of Guesser.
	// See Guesser.h for an explanation.
public:
	// See Guesser.h for method documentation.
	static void initialize() {};
	static void destroy() {};

	static Symbol guessGender(const SynNode *node, const Mention *mention, std::set<Symbol> suspectedSurnames = std::set<Symbol>()) { return Symbol(L""); } 
	static Symbol guessType(const SynNode *node, const Mention *mention) { return Symbol(L""); }
	static Symbol guessNumber(const SynNode *node, const Mention *mention) { return Symbol(L""); }

	static Symbol guessGender(const EntitySet *entitySet, const Entity* entity) { return Symbol(L""); }
	static Symbol guessNumber(const EntitySet *entitySet, const Entity* entity) { return Symbol(L""); }
};


#endif
