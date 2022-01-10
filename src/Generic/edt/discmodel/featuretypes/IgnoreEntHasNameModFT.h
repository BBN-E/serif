// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENT_HAS_NAME_MOD_FT_H
#define END_HAS_NAME_MOD_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class EntHasNameModFT : public DTCorefFeatureType {
public:
	EntHasNameModFT() : DTCorefFeatureType(Symbol(L"ent-has-name-mod")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		Symbol results[1];
		int num_mod =0;
		num_mod = DescLinkFeatureFunctions::hasModName(o->getMention, o->getEntitySet());
		if(num_mod >1){
			num_mod =1;
		}
		resultArray[0] = _new DTIntFeature(this, state.getTag(), 1);
		return 1;
	}

};
#endif
