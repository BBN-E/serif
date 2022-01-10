// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENT_ONLYNAMES_AND_ENTTYPE_FT_H
#define ENT_ONLYNAMES_AND_ENTTYPE_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class EntOnlyNamesAndEntTypeFT : public DTCorefFeatureType {
public:
	EntOnlyNamesAndEntTypeFT() : DTCorefFeatureType(Symbol(L"ent-only-names-and-ent-type")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		
		bool ent_only_names  = DescLinkFeatureFunctions::entityContainsOnlyNames(o->getEntity(), 
			o->getEntitySet());
		

		if(ent_only_names){
			Symbol enttype = o->getEntity()->type.getName();
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), enttype);
			return 1;
		}
		else
			return 0;
	}

};
#endif
