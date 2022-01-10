// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P1DL_MENTSUBTYPE_ENTHW_FT_H
#define P1DL_MENTSUBTYPE_ENTHW_FT_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/theories/EntitySubtype.h"
#include "Generic/theories/Mention.h"

#define P1DL_MENT_MAX_HWS 15

class P1DLMentSubtypeEntHWFT : public DTCorefFeatureType {
public:
	static const int maxHWS = P1DL_MENT_MAX_HWS;

	P1DLMentSubtypeEntHWFT() : DTCorefFeatureType(Symbol(L"p1dl-mentsubtype-enthw")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol );
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
		std::set<Symbol> hws = DescLinkFeatureFunctions::getEntHeadWords(o->getEntity(), o->getEntitySet());

		int nfeat = 0;
		for (std::set<Symbol>::iterator it = hws.begin(); it != hws.end(); ++it) {
			if (nfeat >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION) {
				SessionLogger::warn("DT_feature_limit") 
					<<"P1DLMentSubTypeEntHW discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
				break;
			} else {
				resultArray[nfeat++] = _new DTTrigramFeature(this, state.getTag(),
								mentSubtype.getName(), *it);
			}
		}
		return nfeat;

		/*
		//this includes duplicates	
		int nfeat = 0;
		for(int i= 0; i< o->getEntity()->getNMentions(); i++){
			Mention* oth =  entitySet->getMention(o->getEntity()->getMention(i));
			resultArray[nfeat++] = _new DTTrigramFeature(this, state.getTag(),
				mentSubtype.getName(), oth->getNode()->getHeadWord());
		}

		return nfeat;
		*/
	}

};
#endif
