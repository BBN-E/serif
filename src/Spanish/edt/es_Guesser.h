// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_GUESSER_H
#define es_GUESSER_H

#include "Generic/edt/Guesser.h"
#include "Generic/common/Symbol.h"
class UTF8InputStream;
class DebugStream;

class SpanishGuesser {
	// Note: this class is intentionally not a subclass of Guesser.
	// See Guesser.h for an explanation.
public:
	// See Guesser.h for method documentation.
	static void initialize();
	static void destroy();

	static Symbol guessGender(const SynNode *node, const Mention *mention, std::set<Symbol> suspectedSurnames = std::set<Symbol>()); 
	static Symbol guessType(const SynNode *node, const Mention *mention);
	static Symbol guessNumber(const SynNode *node, const Mention *mention);

	static Symbol guessGender(const EntitySet *entitySet, const Entity* entity);
	static Symbol guessNumber(const EntitySet *entitySet, const Entity* entity); 
private:
	static Symbol guessGpeGenderForName(const SynNode *node, const Mention *mention);
	static Symbol guessPersonGender(const SynNode *node, const Mention *mention);
	static Symbol guessPersonGenderForName(const SynNode *node, const Mention *mention);
	static void loadNames(UTF8InputStream &names_file, Symbol::HashSet * names_set);
	static void loadMultiwordNames(UTF8InputStream& uis, SymbolArraySet * names_set);

	static Symbol::HashSet *_femaleNames, *_maleNames;
	static SymbolArraySet *_gpeNames;

	static DebugStream &_debugOut;
	static bool _initialized;

};

#endif
