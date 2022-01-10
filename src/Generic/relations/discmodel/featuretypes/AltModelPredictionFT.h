// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ALTMODEL_FT_H
#define ALTMODEL_FT_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/relations/xx_RelationUtilities.h"

class AltModelPredictionFT : public P1RelationFeatureType {
public:
	AltModelPredictionFT() : P1RelationFeatureType(Symbol(L"alt-model")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		int nfeat = 0;
		int npred = o->getNAltDecoderPredictions();
		for(int i = 0; i < npred; i++){
			Symbol prediction = o->getNthAltDecoderPrediction(i);
			if(prediction != Symbol(L"NONE")){ //ignore nones, since they will be too diverse
				Symbol name = o->getNthAltDecoderName(i);
				if (nfeat >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION) {
					SessionLogger::warn("DT_feature_limit") <<"AltModelPredictionFT discarding features beyond MAX_FEATURES_PER_EXTRACTION";
					break;
				} else {
					resultArray[nfeat++] = _new DTTrigramFeature(this, state.getTag(), name, prediction);
				}
			}
		}
		return nfeat;	
	}
};

#endif
