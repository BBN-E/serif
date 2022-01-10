// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_VERB_PROP_WC_FT_H
#define EN_VERB_PROP_WC_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/ParamReader.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "English/relations/en_RelationUtilities.h"

class EnglishVerbPropWCFT : public P1RelationFeatureType {
public:
	EnglishVerbPropWCFT() : P1RelationFeatureType(Symbol(L"verb-prop-wc")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramIntFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation *>(
			state.getObservation(0));

		if (o->getVerbinProp() != Symbol(L"NULL") ) {
			Symbol enttype1 = o->getMention1()->getEntityType().getName();
			Symbol enttype2 = o->getMention2()->getEntityType().getName();
			
			int nfeatures = 0;
			if (o->getWCVerbPred().c12() != 0)
				resultArray[nfeatures++] = _new DTTrigramIntFeature(this, state.getTag(),
				enttype1, enttype2, o->getWCVerbPred().c12());

			if (o->getWCVerbPred().c16() != 0)
				resultArray[nfeatures++] = _new DTTrigramIntFeature(this, state.getTag(),
					enttype1, enttype2, o->getWCVerbPred().c16());

			if (o->getWCVerbPred().c20() != 0)
				resultArray[nfeatures++] = _new DTTrigramIntFeature(this, state.getTag(),
					enttype1, enttype2, o->getWCVerbPred().c20());

			return nfeatures;
		} else return 0;
	}

};

#endif
