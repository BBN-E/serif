// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENT_ONLYNAMES_AND_UMRATIO_FT_H
#define ENT_ONLYNAMES_AND_UMRATIO_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class EntOnlyNamesAndUMRatioFT : public DTCorefFeatureType {
public:
	EntOnlyNamesAndUMRatioFT() : DTCorefFeatureType(Symbol(L"ent-only-names-and-umratio")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		
		Symbol results[5];
		bool ent_only_names  = DescLinkFeatureFunctions::entityContainsOnlyNames(o->getEntity(), 
			o->getEntitySet());
		

		if(ent_only_names){
			Symbol umratio =DescLinkFeatureFunctions::getUniqueModifierRatio(o->getMention(),
				o->getEntity(), o->getEntitySet());
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), umratio);
			return 1;
		}
		else
			return 0;
	}

};
#endif
