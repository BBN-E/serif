// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_SIMPLE_PROP_FT_H
#define es_SIMPLE_PROP_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Spanish/relations/es_RelationUtilities.h"

class SpanishSimplePropFT : public P1RelationFeatureType {
public:
	SpanishSimplePropFT() : P1RelationFeatureType(Symbol(L"simple-prop")) {}

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
			Symbol role1 = link->getArg1Role();
			Symbol role2 = link->getArg2Role();
			
			resultArray[0] = _new DTQuadgramFeature(this, state.getTag(),
				role1, role2, link->getTopStemmedPred());

			return 1;
		} else return 0;
	}

};

#endif
