// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_PP_REL_WC_FT_H
#define EN_PP_REL_WC_FT_H

#include "common/Symbol.h"
#include "common/SymbolConstants.h"
#include "relations/discmodel/P1RelationFeatureType.h"
#include "discTagger/DTQuintgramIntFeature.h"
#include "discTagger/DTState.h"
#include "relations/discmodel/RelationObservation.h"
#include "theories/MentionSet.h"
#include "relations/RelationUtilities.h"

class EnglishPPRelWCFT : public P1RelationFeatureType {
public:
	EnglishPPRelWCFT() : P1RelationFeatureType(Symbol(L"pp-relation-wc")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramIntFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
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

			if (o->getVerbinPPRel() != Symbol(L"NULL")) {

				int nfeatures = 0;
				if (o->getWCVerb().c12() != 0)
					resultArray[nfeatures++] = _new DTQuintgramIntFeature(this, state.getTag(),
					enttype1, enttype2, Symbol(L"verbPPRel"), o->getPrepinPPRel(), o->getWCVerb().c12());

				if (o->getWCVerb().c16() != 0)
					resultArray[nfeatures++] = _new DTQuintgramIntFeature(this, state.getTag(),
					enttype1, enttype2, Symbol(L"verbPPRel"), o->getPrepinPPRel(), o->getWCVerb().c16());

				if (o->getWCVerb().c20() != 0)
					resultArray[nfeatures++] = _new DTQuintgramIntFeature(this, state.getTag(),
					enttype1, enttype2, Symbol(L"verbPPRel"), o->getPrepinPPRel(), o->getWCVerb().c20());

				return nfeatures;
			}else{
				int nfeatures = 0;
				if (o->getWCMent1().c12() != 0)
					resultArray[nfeatures++] = _new DTQuintgramIntFeature(this, state.getTag(),
					enttype1, enttype2, Symbol(L"nounPPRel"), o->getPrepinPPRel(), o->getWCMent1().c12());

				if (o->getWCMent1().c16() != 0)
					resultArray[nfeatures++] = _new DTQuintgramIntFeature(this, state.getTag(),
					enttype1, enttype2, Symbol(L"nounPPRel"), o->getPrepinPPRel(), o->getWCMent1().c16());

				if (o->getWCMent1().c20() != 0)
					resultArray[nfeatures++] = _new DTQuintgramIntFeature(this, state.getTag(),
					enttype1, enttype2, Symbol(L"nounPPRel"), o->getPrepinPPRel(), o->getWCMent1().c20());

				return nfeatures;
			}
		}else {
			return 0;
		}
	}
};

#endif
