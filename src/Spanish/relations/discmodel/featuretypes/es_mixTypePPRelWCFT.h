// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_MIXTYPE_PP_REL_WC_FT_H
#define es_MIXTYPE_PP_REL_WC_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuadgramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Spanish/relations/es_RelationUtilities.h"

class SpanishMixTypePPRelWCFT : public P1RelationFeatureType {
public:
	SpanishMixTypePPRelWCFT() : P1RelationFeatureType(Symbol(L"mixtype-pp-relation-wc")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramIntFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, 
								  SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		if (o->hasPPRel()) {
			Symbol enttype1 = o->getMention1()->getEntityType().getName();
			Symbol enttype2 = o->getMention2()->getEntityType().getName();
	
			int nfeatures = 0;
			if (o->getWCMent1().c12() != 0)
				resultArray[nfeatures++] = _new DTQuadgramIntFeature(this, state.getTag(),
				enttype1, enttype2, o->getPrepinPPRel(), o->getWCMent1().c12());

			if (o->getWCMent1().c16() != 0)
				resultArray[nfeatures++] = _new DTQuadgramIntFeature(this, state.getTag(),
				enttype1, enttype2, o->getPrepinPPRel(), o->getWCMent1().c16());

			if (o->getWCMent1().c20() != 0)
				resultArray[nfeatures++] = _new DTQuadgramIntFeature(this, state.getTag(),
				enttype1, enttype2, o->getPrepinPPRel(), o->getWCMent1().c20());

			return nfeatures;
			
		}else {
			return 0;
		}
	}
};

#endif
