// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ALTMODEL_ENTTYPE_FT_H
#define ALTMODEL_ENTTYPE_FT_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/relations/xx_RelationUtilities.h"

class AltModelPredictionEntityTypesFT : public P1RelationFeatureType {
public:
	AltModelPredictionEntityTypesFT() : P1RelationFeatureType(Symbol(L"alt-model-enttype")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		int nfeat = 0;
		bool discard = false;
		int npred = o->getNAltDecoderPredictions();
		for(int i = 0; i < npred; i++){
			Symbol prediction = o->getNthAltDecoderPrediction(i);
			if(prediction != Symbol(L"NONE")){ //ignore nones, since they will be too diverse
				Symbol name = o->getNthAltDecoderName(i);
				if (nfeat >= (DTFeatureType::MAX_FEATURES_PER_EXTRACTION-1)) {
					discard = true;
					break;
				}else{
					resultArray[nfeat++] = _new DTQuintgramFeature(this, state.getTag(), name, prediction,
						o->getMention1()->getEntityType().getName(), 
						o->getMention2()->getEntityType().getName());
				}
			}
		}
		if (!o->getPatternPrediction().is_null() &&
			o->getPatternPrediction() != Symbol(L"NONE"))
		{
			if (nfeat >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION) {
				discard = true;
			}else{
				resultArray[nfeat++] = _new DTQuintgramFeature(this, state.getTag(), 
					Symbol(L"PATTERN"), o->getPatternPrediction(),
						o->getMention1()->getEntityType().getName(), 
						o->getMention2()->getEntityType().getName());
			}
		}
		if (discard){
			SessionLogger::warn("DT_feature_limit") <<"AltModelPredictionEntityTypesFT discarding features beyond MAX_FEATURES_PER_EXTRACTION";
		}
		return nfeat;	
	}
};

#endif
