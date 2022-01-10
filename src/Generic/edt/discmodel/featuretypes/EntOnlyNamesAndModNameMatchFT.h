// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENT_ONLYNAMES_AND_MOD_NAME_MATCH_FT_H
#define ENT_ONLYNAMES_AND_MOD_NAME_MATCH_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class EntOnlyNamesModNameMatchFT : public DTCorefFeatureType {
public:
	EntOnlyNamesModNameMatchFT() : DTCorefFeatureType(Symbol(L"ent-only-names-and-mod-name-match")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		
		Symbol results[5];
		bool ment_has_modname = DescLinkFeatureFunctions::hasModName(o->getMention());
		if(!ment_has_modname)
			return 0;
		bool ent_only_names  = DescLinkFeatureFunctions::entityContainsOnlyNames(o->getEntity(), 
			o->getEntitySet());
		if(!ent_only_names)
			return 0;
		bool nameclash = DescLinkFeatureFunctions::hasModNameEntityClash( o->getMention(),
			o->getEntity(), o->getEntitySet());

		if(ment_has_modname && ent_only_names && !nameclash){
			resultArray[0] = _new DTMonogramFeature(this, state.getTag());
			return 1;
		}
		else
			return 0;
	}

};
#endif
