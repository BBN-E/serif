// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P1DL_SUBTYPE_FT_H
#define P1DL_SUBTYPE_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/theories/EntitySubtype.h"
#include "Generic/theories/Mention.h"



class P1DLSubtypeFT : public DTCorefFeatureType {
public:
	P1DLSubtypeFT() : DTCorefFeatureType(Symbol(L"subtype")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));

		EntitySubtype mentSubtype = o->getMention()->getEntitySubtype();
		if (!mentSubtype.isDetermined()) {
			return 0;
		}
		resultArray[0] = _new DTBigramFeature(this, state.getTag(),
			mentSubtype.getName());
		return 1;	
	}

};
#endif
