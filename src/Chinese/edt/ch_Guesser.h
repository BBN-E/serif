// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_GUESSER_H
#define ch_GUESSER_H

#include "Generic/edt/Guesser.h"
#include "Generic/common/Symbol.h"
class DebugStream;

class ChineseGuesser {
	// Note: this class is intentionally not a subclass of Guesser.
	// See Guesser.h for an explanation.
public:
	// See Guesser.h for method documentation.

public:
	static void initialize() {}
	static void destroy() {}

	static Symbol guessGender(const SynNode *node, const Mention *mention, std::set<Symbol> suspectedSurnames = std::set<Symbol>()); 
	static Symbol guessType(const SynNode *node, const Mention *mention);
	static Symbol guessNumber(const SynNode *node, const Mention *mention);

	static Symbol guessGender(const EntitySet *entitySet, const Entity* entity) {
		return Guesser::UNKNOWN; }
	static Symbol guessNumber(const EntitySet *entitySet, const Entity* entity) { 
		return Guesser::UNKNOWN; }
private:
	static Symbol guessPersonGender(const SynNode *node, const Mention *mention);

	static DebugStream &_debugOut;
};

#endif
