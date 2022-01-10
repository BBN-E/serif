// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_RIGHT_HEAD_WORD_FT_H
#define AR_RIGHT_HEAD_WORD_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/WordConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Arabic/relations/ar_RelationUtilities.h"
#include "Arabic/parse/ar_STags.h"

class ArabicRightHeadWordFT : public P1RelationFeatureType {
public:
	ArabicRightHeadWordFT() : P1RelationFeatureType(Symbol(L"right-head-word")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));


		Symbol enttype1 = o->getMention1()->getEntityType().getName();
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
		else {
			head2 = temp2;
		}

		*/
		/*Symbol variants[4];

		if (WordConstants::getFirstLetterAlefVariants(head2, variants, 4) != 0) 
			head2 = variants[0];
		if (WordConstants::getLastLetterAlefVariants(head2, variants, 4) != 0)
			head2 = variants[0];
		*/
		resultArray[0] = _new DTQuadgramFeature(this, state.getTag(),
				enttype1, enttype2, head2);
		if(RelationUtilities::get()->distIsLessThanCutoff(o->getMention1(), o->getMention2())){
			wchar_t buffer[300];
			wcscpy(buffer, enttype1.to_string());
			wcscat(buffer, L"-CLOSE");
			Symbol s1 =Symbol(buffer);
			wcscpy(buffer, enttype2.to_string());
			wcscat(buffer, L"-CLOSE");
			Symbol s2 =Symbol(buffer);
			resultArray[1] = _new DTQuadgramFeature(this, state.getTag(), s1, s2, head2);
			return 2;
		}
		else{
			wchar_t buffer[300];
			wcscpy(buffer, enttype1.to_string());
			wcscat(buffer, L"-FAR");
			Symbol s1 =Symbol(buffer);
			wcscpy(buffer, enttype2.to_string());
			wcscat(buffer, L"-FAR");
			Symbol s2 =Symbol(buffer);
			resultArray[1] = _new DTQuadgramFeature(this, state.getTag(), s1, s2, head2);
			return 2;
		}


		return 1;

	}

};

#endif
