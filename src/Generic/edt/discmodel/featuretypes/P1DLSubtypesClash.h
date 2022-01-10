// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P1DL_SUBTYPESCLASH_FT_H
#define P1DL_SUBTYPESCLASH_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/theories/EntitySubtype.h"
#include "Generic/theories/Mention.h"


class P1DLSubtypesClashFT : public DTCorefFeatureType {
public:
	P1DLSubtypesClashFT() : DTCorefFeatureType(Symbol(L"p1dl-subtypesclash")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		EntitySubtype mentSubtype = o->getMention()->getEntitySubtype();
		if(!mentSubtype.isDetermined()){
			return 0;
		}
		const EntitySet* entitySet = o->getEntitySet();
		for(int i= 0; i< o->getEntity()->getNMentions(); i++){
			Mention* oth =  entitySet->getMention(o->getEntity()->getMention(i));
			if(!oth->getEntitySubtype().isDetermined())
				continue;
			if(mentSubtype != oth->getEntitySubtype()){
				resultArray[0] = _new DTMonogramFeature(this, state.getTag());
				return 1;
			}
		}
		return 0;
	}

};
#endif
