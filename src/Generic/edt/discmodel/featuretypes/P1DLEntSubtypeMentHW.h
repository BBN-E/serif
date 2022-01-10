// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P1DL_ENTSUBTYPE_MENTHW_FT_H
#define P1DL_ENTSUBTYPE_MENTHW_FT_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/EntitySubtype.h"
#include "Generic/theories/Mention.h"

#define P1DL_ENT_MAX_SUBTYPES 15

class P1DLEntSubtypeMentHWFT : public DTCorefFeatureType {
public:
	static const int maxSubtypes = P1DL_ENT_MAX_SUBTYPES;

	P1DLEntSubtypeMentHWFT() : DTCorefFeatureType(Symbol(L"p1dl-entsubtype-menthw")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol );
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		Symbol hw = o->getMention()->getNode()->getHeadWord();

		int nsubtypes = 0;
		
		Symbol subtypes[P1DL_ENT_MAX_SUBTYPES];
		Entity* ent = o->getEntity();
		const EntitySet* ents = o->getEntitySet();

		for (int i = 0; i < ent->getNMentions(); i++) {
			Mention* ment = ents->getMention(ent->getMention(i));
			if(ment->getEntitySubtype().isDetermined()){
				Symbol st = ment->getEntitySubtype().getName();
				// filter out duplicates
				bool valid = true;
				for (int m = 0; m < nsubtypes; m++) {
					if (subtypes[m] == st) {
						valid = false;
						break;
					}
				}
				if (valid)
					subtypes[nsubtypes++] = st;
				if (nsubtypes >= maxSubtypes)
					break;
			}
		}
		int jsub;
		for (jsub = 0; jsub<nsubtypes; jsub++){
			if (jsub >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION){
				SessionLogger::warn("DT_feature_limit") 
					<<"P1DLEntSubtypeMentHW discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
				break;
			}
			resultArray[jsub] = _new DTTrigramFeature(this, state.getTag(),
					subtypes[jsub], hw);
		}
		return jsub;	
	}

};
#endif
