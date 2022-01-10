// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ALTMODEL_DESC_HW_FT_H
#define ALTMODEL_DESC_HW_FT_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/MentionSet.h"


class AltModelDescPredictionHWFT : public P1DescFeatureType {
public:
	AltModelDescPredictionHWFT() : P1DescFeatureType(Symbol(L"alt-model-hw")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));

		int nfeat = 0;
		int npred = o->getNAltDecoderPredictions();
		for(int i = 0; i < npred; i++){
			Symbol prediction = o->getNthAltDecoderPrediction(i);
			if (prediction != Symbol(L"NONE")) { //ignore nones, since they will be too diverse
				Symbol name = o->getNthAltDecoderName(i);
				if (nfeat >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION) {
					SessionLogger::warn("DT_feature_limit") 
						<<"AltModelPredictionEntityTypeSubtypeFT discarding features beyond MAX_FEATURES_PER_EXTRACTION";
					break;
				}else{
					resultArray[nfeat++] = _new DTQuadgramFeature(this, state.getTag(), 
											name, prediction, o->getNode()->getHeadWord());
				}
			}
		}
		return nfeat;	
	}
};

#endif
