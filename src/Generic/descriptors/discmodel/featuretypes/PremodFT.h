// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PREMOD_FT_H
#define PREMOD_FT_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"



class PremodFT : public P1DescFeatureType {
public:
	PremodFT() : P1DescFeatureType(Symbol(L"premod")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));

		const SynNode *node = o->getNode();

		for (int j = 0; j < node->getHeadIndex(); j++) {
			if (j >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION) {
					SessionLogger::warn("DT_feature_limit") 
						<<"PremodFT discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
					break;
			}else{	
				resultArray[j] = _new DTBigramFeature(this, state.getTag(), 
				node->getChild(j)->getHeadWord());
			}
		}
		
		return node->getHeadIndex();
	}

};

#endif
