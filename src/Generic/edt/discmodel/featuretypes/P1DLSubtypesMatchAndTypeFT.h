// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P1DL_SUBTYPESMATCHANDTYPE_FT_H
#define P1DL_SUBTYPESMATCHANDTYPE_FT_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/theories/EntitySubtype.h"
#include "Generic/theories/Mention.h"


class P1DLSubtypesMatchAndTypeFT : public DTCorefFeatureType {
public:
	P1DLSubtypesMatchAndTypeFT() : DTCorefFeatureType(Symbol(L"subtypesmatch-and-type")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol
			, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		const Mention *ment = o->getMention();
		EntitySubtype mentSubtype = ment->getEntitySubtype();
		if(!mentSubtype.isDetermined()){
			return 0;
		}
		// get the mention EntityType
		EntityType mentEntType = ment->getEntityType();
		Symbol mentEntTypeSymbol = mentEntType.isDetermined() ? mentEntType.getName(): NO_ENTITY_TYPE;

		const EntitySet* entitySet = o->getEntitySet();
		int n_feat = 0;
		for(int i= 0; i< o->getEntity()->getNMentions(); i++){
			Mention* oth =  entitySet->getMention(o->getEntity()->getMention(i));
			if(!oth->getEntitySubtype().isDetermined())
				continue;
			// get the entity's EntityType
			EntityType othType = oth->getEntityType();
			if (!othType.isDetermined())
				continue;
			if(mentEntTypeSymbol != othType.getName())
				continue;
			if(mentSubtype == oth->getEntitySubtype()){
				if (n_feat >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION){
					SessionLogger::warn("DT_feature_limit") 
						<<"P1DLSubtypesMatchAndTypeFT discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
					break;
				}else{
					resultArray[n_feat++] = _new DTTrigramFeature(this, state.getTag(), mentEntTypeSymbol, mentSubtype.getName());
				}
			}
		}
		return n_feat;
	}

};
#endif
