// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENT_TYPES_MATCH_FT_H
#define MENT_TYPES_MATCH_FT_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/EntitySubtype.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Mention.h"

// DO not use this feature with pronoun as their type is not yet defined during coref
class NMMentTypesMatchFT : public DTCorefFeatureType {
public:
	NMMentTypesMatchFT() : DTCorefFeatureType(Symbol(L"name-ment-types-match")) {
	}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol
			, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		const EntitySet *entitySet = o->getEntitySet();
		EntityType mentType = o->getMention()->getEntityType();
		Entity *entity = o->getEntity();
		EntityType entMentionType;
		if(!mentType.isDetermined()){
			return 0;
		}

		int nFeatures = 0;
		bool discard = false;
		bool found_match = false;
		for (int i=0; i<entity->getNMentions(); i++) {
			entMentionType  = entitySet->getMention(entity->getMention(i))->getEntityType();
			if (!entMentionType.isDetermined())
				continue;
			if (nFeatures >= (DTFeatureType::MAX_FEATURES_PER_EXTRACTION-1)){
					discard = true;
					break;
			}
			if (mentType == entMentionType){
					resultArray[nFeatures++] = _new DTTrigramFeature(this, state.getTag(), entMentionType.getName(), MATCH_SYM);
					found_match = true;
			}else{
					resultArray[nFeatures++] = _new DTTrigramFeature(this, state.getTag(), entMentionType.getName(), CLASH_SYM);
			}
		}

			if (found_match){
				if (nFeatures >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION){
					discard = true;
				}else{
					resultArray[nFeatures++] = _new DTTrigramFeature(this, state.getTag(), entMentionType.getName(), MATCH_UNIQU_SYM);
				}
			}
		if (discard) {
			SessionLogger::info("nmtmft_disc_0") 
				<<"NMentTypesMatchFT discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
		}
		return nFeatures;
	}

};
#endif
