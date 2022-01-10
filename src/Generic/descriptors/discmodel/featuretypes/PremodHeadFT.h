// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PREMOD_HEAD_FT_H
#define PREMOD_HEAD_FT_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"


class PremodHeadFT : public P1DescFeatureType {
public:
	PremodHeadFT() : P1DescFeatureType(Symbol(L"premod-head")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));

		const SynNode *node = o->getNode();

		int j = 0;
		for (j = 0; j < node->getHeadIndex(); j++) {
			//in cases of very long lists, there are more 'pre-mods' than the Array size
			if( j >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION){
				SessionLogger::info("premodheadft_0") 
					<<"PremodHeadFT discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
				break;
			}
			const char* word = node->getHeadWord().to_debug_string();
			resultArray[j] = _new DTTrigramFeature(this, state.getTag(), 
				node->getHeadWord(),
				node->getChild(j)->getHeadWord());
		}
		
		return j;
	}

};

#endif
