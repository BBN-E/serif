// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_ADJ_NATIONALITY_H
#define AR_ADJ_NATIONALITY_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Arabic/common/ar_NationalityRecognizer.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/EntitySet.h"


class ArabicAdjNationality : public DTCorefFeatureType {
public:
	ArabicAdjNationality() : DTCorefFeatureType(Symbol(L"adj-nationality")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		const Mention* ment= o->getMention();
		Symbol ment_hw = o->getMention()->getNode()->getHeadWord();
		ArabicNationalityRecognizer::isNationalityWord(ment_hw);
		int ment_start = ment->getNode()->getHeadPreterm()->getStartToken();
		int ment_end = ment->getNode()->getHeadPreterm()->getEndToken();
		int sent_num = o->getMention()->getSentenceNumber();
		// ?? Symbol name_words[20];
		Symbol location;
		int nresults = 0;
		for(int i =0; i< o->getEntity()->getNMentions(); i++){
			Mention* ent_ment = o->getEntitySet()->getMention(o->getEntity()->getMention(i));
			if(ent_ment->getSentenceNumber() != sent_num){
				continue;
			}
			int ent_ment_start = ent_ment->getNode()->getHeadPreterm()->getStartToken();
			int ent_ment_end = ent_ment->getNode()->getHeadPreterm()->getEndToken();
			if(ent_ment_start == (ment_end + 1)){
				location = Symbol(L"-NEXT-");
			}
			else if(ent_ment_end == (ment_start - 1)){
				location = Symbol(L"-PREV-");
			}
			else{
				continue;
			}

			if(ent_ment->getMentionType() != Mention::NAME){
				continue;
			}
			Symbol ent_ment_hw = ent_ment->getNode()->getHeadWord();
			if (nresults >= (DTFeatureType::MAX_FEATURES_PER_EXTRACTION-1)){
				SessionLogger::warn("DT_feature_limit") <<"ar_AdjNationality discarding features beyond MAX_FEATURES_PER_EXTRACTION";
				break;
			}
			resultArray[nresults++] = _new DTQuadgramFeature(this, state.getTag(),
				ment_hw, ent_ment->getNode()->getHeadWord(), location );
			resultArray[nresults++] = _new DTQuadgramFeature(this, state.getTag(),
				ment_hw, ent_ment->getEntityType().getName(), location );
			//resultArray[nresults++] = _new DTQuadgramFeature(this, state.getTag(),
			//	Symbol(L"-BACKOFF-"), ent_ment->getEntityType().getName(), location );
			//resultArray[nresults++] = _new DTQuadgramFeature(this, state.getTag(),
			///				Symbol(L"-BACKOFF-"), Symbol(L"-BACKOFF-"), location );
		}
		return nresults;
	}

};
#endif
