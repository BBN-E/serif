#ifndef ES_SEM_TREE_UTILS_H
#define ES_SEM_TREE_UTILS_H

// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.


#include "Generic/common/DebugStream.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SynNode.h"


class SpanishSemTreeUtils {
	// Note: this class is intentionally not a subclass of anything

public:

	static const Symbol::SymbolGroup transitivePastParticiples;

	static const Symbol::SymbolGroup transitiveInfinitives;

	static const Symbol::SymbolGroup temporalWords;

	static const Symbol::SymbolGroup temporalPeriodPortionWords;

	static const Symbol::SymbolGroup negativeAdverbs;

	static const Symbol::SymbolGroup sym_locativeNominals;

	static const Symbol sym_de;
	static const Symbol sym_por;
	static const Symbol sym_haber;
	static const Symbol::SymbolGroup sym_haberForms;


	
	static bool isHaber(Symbol word, bool includeInfinitive);

	static bool looksLikeTemporal(const SynNode *node, const MentionSet *ms, bool flag);

	static bool isParticle(const SynNode &node);
	static bool isAdverb(const SynNode &node);

	static bool isTemporalNP(const SynNode &node, const MentionSet *mentionSet);

	static bool isNegativeAdverb(Symbol);
	static bool isKnownTransitivePastParticiple(Symbol);
	static bool isKnownTemporalNominal(Symbol);
	static bool isTemporalPeriodPortion(Symbol);
	static bool isLocativeNominal(Symbol);
	static bool isConjunctionPOS(Symbol);
	static bool isVerbPOS(Symbol);
	static bool isGerundPOS(Symbol);
	static bool isPastParticiplePOS(Symbol);
	static bool isInfinitivePOS(Symbol); 
	static bool isPunctuationPOS(Symbol);
	static bool isVerbPhrase(Symbol);
	static bool isMainVerbPOS(Symbol);
	static bool isInfinitiveOrGerund(Symbol);
	static bool isSemiAuxVerbPOS(Symbol);
	static bool isAuxVerbPOS(Symbol);

	static int verbMainScore(Symbol);

	static Symbol composeChildSymbols(const SynNode *head, Symbol tag);
	static Symbol primaryVerbSym(const SynNode *synode);

	static const bool USE_2009_TEMPORALS = true;
	// set true to allow temporals inside temporals

};


#endif
