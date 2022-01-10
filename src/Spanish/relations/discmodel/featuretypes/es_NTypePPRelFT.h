// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_NTYPE_PP_REL_FT_H
#define es_NTYPE_PP_REL_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Spanish/relations/es_RelationUtilities.h"

class SpanishNTypePPRelFT : public P1RelationFeatureType {
public:
	SpanishNTypePPRelFT() : P1RelationFeatureType(Symbol(L"ntype-pp-relation")) {}

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

		if (o->hasPPRel()) {
			Symbol enttype1 = o->getMention1()->getEntityType().getName();
			Symbol enttype2 = o->getMention2()->getEntityType().getName();

			// option 1 
			/*
			Symbol head1 = o->getMention1()->getNode()->getHeadWord();
			Symbol head2 = o->getMention2()->getNode()->getHeadWord();
			*/

			// option 2 - use stem words
			Symbol head1 = o->getStemmedHeadofMent1();
			Symbol head2 = o->getStemmedHeadofMent2();

			if (o->getVerbinPPRel() == Symbol(L"NULL")) {
				resultArray[0] = _new DTQuintgramFeature(this, state.getTag(),
					enttype1, enttype2, o->getPrepinPPRel(), head1);
				
				/*
				resultArray[1] = _new DTQuintgramFeature(this, state.getTag(),
					enttype1, head2, o->getPrepinPPRel(), head1);
				resultArray[2] = _new DTQuintgramFeature(this, state.getTag(),
					head1, enttype2, o->getPrepinPPRel(), head1);
				resultArray[3] = _new DTQuintgramFeature(this, state.getTag(),
					head1, head2, o->getPrepinPPRel(), head1);
				return 4;
				*/
				resultArray[0] = _new DTQuintgramFeature(this, state.getTag(),
					enttype1, enttype2, o->getPrepinPPRel(), enttype1);
				return 2;

			}else return 0;
		}else {
			return 0;
		}
	}

};

#endif

