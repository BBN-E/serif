// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TYPES_CLASH_FT_H
#define TYPES_CLASH_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/EntitySubtype.h"
#include "Generic/theories/Mention.h"

// DO not use this feature with pronoun as their type is not yet defined during coref
class TypesClashFT : public DTCorefFeatureType {
public:
	TypesClashFT() : DTCorefFeatureType(Symbol(L"types-clash")) {
	}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		EntityType mentType = o->getMention()->getEntityType();
		EntityType entType = o->getEntity()->getType();
		if(!mentType.isDetermined() || !entType.isDetermined()){
			return 0;
		}

		if(mentType == entType){
//			if(mentType == EntityType::getOtherType() || mentType == EntityType::getUndetType())
//				resultArray[0] = _new DTBigramFeature(this, state.getTag(), MATCH_OTHER_SYM);
//			else
				resultArray[0] = _new DTBigramFeature(this, state.getTag(), MATCH_SYM);
		}else{
				resultArray[0] = _new DTBigramFeature(this, state.getTag(), CLASH_SYM);
		}
		return 1;
	}

};
#endif
