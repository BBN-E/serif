// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_SIMPLE_PROP_HW_FT_H
#define es_SIMPLE_PROP_HW_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Spanish/relations/es_RelationUtilities.h"

class SpanishSimplePropHWFT : public P1RelationFeatureType {
public:
	SpanishSimplePropHWFT() : P1RelationFeatureType(Symbol(L"simple-prop-hw")) {}

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

		RelationPropLink *link = o->getPropLink();
		if (!link->isEmpty() && !link->isNested()) {
			Symbol head1 = o->getMention1()->getNode()->getHeadWord();
			Symbol head2 = o->getMention2()->getNode()->getHeadWord();
			Symbol enttype1 = o->getMention1()->getEntityType().getName();
			Symbol enttype2 = o->getMention2()->getEntityType().getName();

			resultArray[0] = _new DTQuadgramFeature(this, state.getTag(),
				enttype1, head2, link->getTopStemmedPred());

			resultArray[1] = _new DTQuadgramFeature(this, state.getTag(),
				head1, enttype2, link->getTopStemmedPred());

			return 2;
		} else return 0;
	}

};

#endif
