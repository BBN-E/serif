// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_HEAD_WORD_WC_FT_H
#define AR_HEAD_WORD_WC_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/WordConstants.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigram2IntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Arabic/relations/ar_RelationUtilities.h"

class ArabicHeadWordWCFT : public P1RelationFeatureType {
public:
	ArabicHeadWordWCFT() : P1RelationFeatureType(Symbol(L"head-word-wc")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigram2IntFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  0, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));


		Symbol enttype1 = o->getMention1()->getEntityType().getName();
		Symbol enttype2 = o->getMention2()->getEntityType().getName();

		Symbol head1 = o->getMention1()->getNode()->getHeadWord();
		Symbol head2 = o->getMention2()->getNode()->getHeadWord();

		/*
		// If head1 is a clitic, find the next terminal
		if (WordConstants::isPrefixClitic(head1)) {
			const SynNode *clitic = o->getMention1()->getNode()->getHeadPreterm()->getHead();
			const SynNode *next = clitic->getNextTerminal();
			if (next != 0) {
				const SynNode *parent = next->getParent();
				while (parent != 0 && parent != o->getMention1()->getNode())
					parent = parent->getParent();
				if (parent != 0 && parent == o->getMention1()->getNode())
					head1 = next->getHeadWord();
			}
		}
		// If head2 is a clitic, find the next terminal
		if (WordConstants::isPrefixClitic(head2)) {
			const SynNode *clitic = o->getMention2()->getNode()->getHeadPreterm()->getHead();
			const SynNode *next = clitic->getNextTerminal();
			if (next != 0) {
				const SynNode *parent = next->getParent();
				while (parent != 0 && parent != o->getMention2()->getNode())
					parent = parent->getParent();
				if (parent != 0 && parent == o->getMention2()->getNode())
					head2 = next->getHeadWord();
			}
		}

		Symbol temp1 = WordConstants::removeAl(head1);
		Symbol temp2 = WordConstants::removeAl(head2);

		// if the head word was "Al" try to find next word in the same mention
		// BUT: leave Al attached if it was part of the word!!
		if (wcscmp(temp1.to_string(), L"") == 0) {
			const SynNode *al = o->getMention1()->getNode()->getHeadPreterm()->getHead();
			const SynNode *next = al->getNextTerminal();
			if (next != 0) {
				const SynNode *parent = next->getParent();
				while (parent != 0 && !parent->hasMention())
					parent = parent->getParent();
				if (parent != 0 && parent == o->getMention1()->getNode())
					head1 = next->getHeadWord();
			}
		}
		if (wcscmp(temp2.to_string(), L"") == 0) {
			const SynNode *al = o->getMention2()->getNode()->getHeadPreterm()->getHead();
			const SynNode *next = al->getNextTerminal();
			if (next != 0) {
				const SynNode *parent = next->getParent();
				while (parent != 0 && !parent->hasMention())
					parent = parent->getParent();
				if (parent != 0 && parent == o->getMention2()->getNode())
					head2 = next->getHeadWord();
			}
		}

		*/

		WordClusterClass wc1(head1);
		WordClusterClass wc2(head2);

		int nfeatures = 0;
		if (wc1.c20() != 0 && wc2.c20() != 0)
			resultArray[nfeatures++] = _new DTTrigram2IntFeature(this, state.getTag(),
				enttype1, enttype2, wc1.c20(), wc2.c20());
		if (wc1.c16() != 0 && wc2.c16() != 0)
			resultArray[nfeatures++] = _new DTTrigram2IntFeature(this, state.getTag(),
				enttype1, enttype2, wc1.c16(), wc2.c16());
		if (wc1.c12() != 0 && wc2.c12() != 0)
			resultArray[nfeatures++] = _new DTTrigram2IntFeature(this, state.getTag(),
				enttype1, enttype2, wc1.c12(), wc2.c12());

		return nfeatures;

	}

};

#endif
