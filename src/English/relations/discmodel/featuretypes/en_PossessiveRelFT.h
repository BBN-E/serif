// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_POSSESSIVE_REL_FT_H
#define EN_POSSESSIVE_REL_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"


class EnglishPossessiveRelFT : public P1RelationFeatureType {
public:
	EnglishPossessiveRelFT() : P1RelationFeatureType(Symbol(L"poss-relation")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		if (o->hasPossessiveRel()) {
			// option 1
				
			Symbol head1 = o->getMention1()->getNode()->getHeadWord();
			Symbol head2 = o->getMention2()->getNode()->getHeadWord();
			

			// option 2 - use stemmed head words
			/*			
			Symbol head1 = o->getStemmedHeadofMent1();
			Symbol head2 = o->getStemmedHeadofMent2();
			*/

			Symbol enttype1 = o->getMention1()->getEntityType().getName();
			Symbol enttype2 = o->getMention2()->getEntityType().getName();

			
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(),
								  head1, head2);

			resultArray[1] = _new DTTrigramFeature(this, state.getTag(),
			enttype1, head2);
	
			resultArray[2] = _new DTTrigramFeature(this, state.getTag(),
			head1, enttype2);
	
			resultArray[3] = _new DTTrigramFeature(this, state.getTag(),
			enttype1, enttype2);
	
			return 4;
		}

		
		return 0;
	}

};

#endif
