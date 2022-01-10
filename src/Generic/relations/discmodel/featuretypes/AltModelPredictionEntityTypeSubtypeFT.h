// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ALTMODEL_ENTTYPESUBTYPE_FT_H
#define ALTMODEL_ENTTYPESUBTYPE_FT_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"

#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/xx_RelationUtilities.h"
#include "Generic/theories/MentionSet.h"


class AltModelPredictionEntityTypeSubtypeFT : public P1RelationFeatureType {
public:
	AltModelPredictionEntityTypeSubtypeFT() : P1RelationFeatureType(Symbol(L"alt-model-enttype-subtype")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		//only fire if one of the entities has a determined subtype
		if(!(o->getMention1()->getEntitySubtype().isDetermined() || 
			o->getMention1()->getEntitySubtype().isDetermined()))
		{
			return 0;
		}

		
		Symbol ment1Subtype = o->getMention1()->getEntitySubtype().getName();
		Symbol ment2Subtype = o->getMention2()->getEntitySubtype().getName();
		// If we are finding different subtypes than we trained on, map
		// to trained on subtypes
		ment1Subtype = RelationUtilities::get()->mapToTrainedOnSubtype(ment1Subtype);
		ment2Subtype = RelationUtilities::get()->mapToTrainedOnSubtype(ment2Subtype);

		wchar_t buffer[1000];
		Symbol t1 = o->getMention1()->getEntityType().getName();
		if(o->getMention1()->getEntitySubtype().isDetermined()){
			wcscpy(buffer, t1.to_string());
			wcscat(buffer, L":");
			wcscat(buffer, ment1Subtype.to_string());
			t1 = Symbol(buffer);
		}
		Symbol t2 = o->getMention2()->getEntityType().getName();
		if(o->getMention2()->getEntitySubtype().isDetermined()){
			wcscpy(buffer, t2.to_string());
			wcscat(buffer, L":");
			wcscat(buffer, ment2Subtype.to_string());
			t2 = Symbol(buffer);
		}

		int nfeat = 0;
		int npred = o->getNAltDecoderPredictions();
		for(int i = 0; i < npred; i++){
			Symbol prediction = o->getNthAltDecoderPrediction(i);
			if(prediction != Symbol(L"NONE")){ //ignore nones, since they will be too diverse
				Symbol name = o->getNthAltDecoderName(i);
								
				if (nfeat >= (DTFeatureType::MAX_FEATURES_PER_EXTRACTION-1)) {
					SessionLogger::warn("DT_feature_limit") <<"AltModelPredictionEntityTypeSubtypeFT discarding features beyond MAX_FEATURES_PER_EXTRACTION";
					break;
				}else{
					resultArray[nfeat++] = _new DTQuintgramFeature(this, state.getTag(), name, 
					prediction, t1, t2);
				}
			}
		}		
		if (!o->getPatternPrediction().is_null() &&
			o->getPatternPrediction() != Symbol(L"NONE"))
		{
			if (nfeat >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION) {
					SessionLogger::warn("DT_feature_limit") <<"AltModelPredictionEntityTypeSubtypeFT discarding features beyond MAX_FEATURES_PER_EXTRACTION";
			}else{
				resultArray[nfeat++] = _new DTQuintgramFeature(this, state.getTag(), 
					Symbol(L"PATTERN"), o->getPatternPrediction(), t1, t2);
			}
		}
		return nfeat;	
	}
};

#endif
