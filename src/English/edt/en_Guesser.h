// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_GUESSER_H
#define en_GUESSER_H

#include "Generic/edt/Guesser.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolArray.h"
class UTF8InputStream;
class DebugStream;

class EnglishGuesser {
	// Note: this class is intentionally not a subclass of Guesser.
	// See Guesser.h for an explanation.
public:
	// See Guesser.h for method documentation.
	static bool guessUntyped;
	static bool guessXDocNames;

	static void initialize();
	static void destroy();

	static Symbol guessGender(const SynNode *node, const Mention *mention, std::set<Symbol> suspectedSurnames = std::set<Symbol>());
	static Symbol guessType(const SynNode *node, const Mention *mention);
	static Symbol guessNumber(const SynNode *node, const Mention *mention);

	static Symbol guessGender(const EntitySet *entitySet, const Entity* entity);
	static Symbol guessNumber(const EntitySet *entitySet, const Entity* entity);

private:

	static Symbol guessPersonGender(const SynNode *node, const Mention *mention, std::set<Symbol> suspectedSurnames);
	static Symbol guessPersonGenderForName(const SynNode *node, const Mention *mention, std::set<Symbol> suspectedSurnames);
	static Symbol guessPersonGenderForXDocName(const SynNode *node, const Mention *mention);
	static void loadNames(UTF8InputStream &femaleNamesFile, UTF8InputStream &maleNamesFile,
						  UTF8InputStream &femaleDescsFile, UTF8InputStream &maleDescsFile,
						  UTF8InputStream &pluralDescsFile, UTF8InputStream &pluralNounsFile,
						  UTF8InputStream &singularNounsFile, UTF8InputStream &xdocMaleNamesFile,
						  UTF8InputStream &xdocFemaleNamesFile);

	static void loadMultiwordNames(UTF8InputStream &uis, SymbolArraySet *names_set);

	static Symbol::HashSet *_femaleNames, *_maleNames, *_femaleDescriptors, *_maleDescriptors,
	                       *_pluralDescriptors, *_pluralNouns, *_singularNouns;

	static SymbolArraySet *_xdocMaleNames, *_xdocFemaleNames;

	static DebugStream &_debugOut;
	static bool _initialized;
};

#endif
