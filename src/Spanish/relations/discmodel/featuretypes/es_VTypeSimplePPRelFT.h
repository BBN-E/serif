// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_VTYPE_SIMPLE_PP_REL_FT_H
#define es_VTYPE_SIMPLE_PP_REL_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Spanish/relations/es_RelationUtilities.h"

class SpanishVTypeSimplePPRelFT : public P1RelationFeatureType {
public:
	SpanishVTypeSimplePPRelFT() : P1RelationFeatureType(Symbol(L"vtype-simple-pp-relation")) {}

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
			if (o->getVerbinPPRel() != Symbol(L"NULL")) {
				// option 1
				// resultArray[0] = _new DTTrigramFeature(this, state.getTag(), o->getPrepinPPRel(), o->getVerbinPPRel());

				// option 2
				resultArray[0] = _new DTTrigramFeature(this, state.getTag(), o->getPrepinPPRel(), o->getStemmedVerbinPPRel());

				return 1;
			}else return 0;
		} else return 0;
	}

};

#endif
