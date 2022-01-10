// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_POSSESSIVE_WC_FT_H
#define es_POSSESSIVE_WC_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/ParamReader.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Spanish/relations/es_RelationUtilities.h"

class SpanishPossessiveWCFT : public P1RelationFeatureType {
public:
	SpanishPossessiveWCFT() : P1RelationFeatureType(Symbol(L"poss-wc")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramIntFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		if (o->hasPossessiveRel()) {
			Symbol enttype1 = o->getMention1()->getEntityType().getName();
			Symbol enttype2 = o->getMention2()->getEntityType().getName();
			
			int nfeatures = 0;
			if (o->getWCMent2().c12() != 0)
				resultArray[nfeatures++] = _new DTTrigramIntFeature(this, state.getTag(),
					enttype1, enttype2, o->getWCMent2().c12());

			if (o->getWCMent2().c16() != 0)
				resultArray[nfeatures++] = _new DTTrigramIntFeature(this, state.getTag(),
					enttype1, enttype2, o->getWCMent2().c16());

			if (o->getWCMent2().c20() != 0)
				resultArray[nfeatures++] = _new DTTrigramIntFeature(this, state.getTag(),
					enttype1, enttype2, o->getWCMent2().c20());

			return nfeatures;
		} else return 0;
	}

};

#endif
