// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_RIGHT_HEAD_WORD_WC_FT_H
#define AR_RIGHT_HEAD_WORD_WC_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/WordConstants.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTBigramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Arabic/relations/ar_RelationUtilities.h"

class ArabicRightHeadWordWCFT : public P1RelationFeatureType {
public:
	ArabicRightHeadWordWCFT() : P1RelationFeatureType(Symbol(L"right-head-word-wc")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramIntFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		Symbol enttype2 = o->getMention2()->getEntityType().getName();
		Symbol head2 = o->getMention2()->getNode()->getHeadWord();

		/*
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

		Symbol temp2 = WordConstants::removeAl(head2);

		// if the head word was "Al" try to find next word in the same mention
		// BUT: leave Al attached if it was part of the word!!
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

		WordClusterClass wc2(head2);

		int nfeatures = 0;
		if (wc2.c20() != 0)
			resultArray[nfeatures++] = _new DTBigramIntFeature(this, state.getTag(), enttype2, wc2.c20());
		if (wc2.c16() != 0)
			resultArray[nfeatures++] = _new DTBigramIntFeature(this, state.getTag(), enttype2, wc2.c16());
		if (wc2.c12() != 0)
			resultArray[nfeatures++] = _new DTBigramIntFeature(this, state.getTag(), enttype2, wc2.c12());

		return nfeatures;

	}

};

#endif
