// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_MIXTYPE_SIMPLE_PP_REL_FT_H
#define es_MIXTYPE_SIMPLE_PP_REL_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Spanish/relations/es_RelationUtilities.h"

class SpanishMixTypeSimplePPRelFT : public P1RelationFeatureType {
public:
	SpanishMixTypeSimplePPRelFT() : P1RelationFeatureType(Symbol(L"mixtype-simple-pp-relation")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		if (o->hasPPRel()) {
			// option 1 
			//Symbol head1 = o->getMention1()->getNode()->getHeadWord();
			
			// option 2 - use stemmed head words
			Symbol head1 = o->getStemmedHeadofMent1();
						
			Symbol enttype1 = o->getMention1()->getEntityType().getName();
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(),
				o->getPrepinPPRel(), head1);
			resultArray[1] = _new DTTrigramFeature(this, state.getTag(),
				o->getPrepinPPRel(), enttype1);
			return 2;
		} else return 0;
	}
};

#endif