// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_HEAD_WORD_FT_H
#define AR_HEAD_WORD_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/WordConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Arabic/relations/ar_RelationUtilities.h"


class ArabicHeadWordFT : public P1RelationFeatureType {
public:
	ArabicHeadWordFT() : P1RelationFeatureType(Symbol(L"head-word")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
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

		/*// If head1 is a clitic, find the next terminal
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
		else {
			head1 = temp1;
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
		else {
			head2 = temp2;
		}
		*/
		/*Symbol variants[4];

		if (WordConstants::getFirstLetterAlefVariants(head1, variants, 4) != 0) 
			head1 = variants[0];
		if (WordConstants::getFirstLetterAlefVariants(head2, variants, 4) != 0) 
			head2 = variants[0];
		if (WordConstants::getLastLetterAlefVariants(head1, variants, 4) != 0)
			head1 = variants[0];
		if (WordConstants::getLastLetterAlefVariants(head2, variants, 4) != 0)
			head2 = variants[0];
		*/

		resultArray[0] = _new DTQuintgramFeature(this, state.getTag(),
				enttype1, enttype2, head1, head2);
		if(RelationUtilities::get()->distIsLessThanCutoff(o->getMention1(), o->getMention2())){
			wchar_t buffer[300];
			wcscpy(buffer, enttype1.to_string());
			wcscat(buffer, L"-CLOSE");
			Symbol s1 =Symbol(buffer);
			wcscpy(buffer, enttype2.to_string());
			wcscat(buffer, L"-CLOSE");
			Symbol s2 =Symbol(buffer);
			resultArray[1] = _new DTQuintgramFeature(this, state.getTag(), s1, s2, head1, head2);
			return 2;
		}
		

		return 1;

	}

};

#endif
