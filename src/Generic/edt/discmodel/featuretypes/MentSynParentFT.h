// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENT_SYN_PARENT_FT_H
#define MENT_SYN_PARENT_FT_H

#include "Generic/common/Symbol.h"
//#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
//#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class MentSynParentFT : public DTCorefFeatureType {
public:
	MentSynParentFT() : DTCorefFeatureType(Symbol(L"ment-syn-parent")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTNoneCorefObservation *o = static_cast<DTNoneCorefObservation*>(
			state.getObservation(0));
		const SynNode *parent = o->getMention()->getNode()->getParent();

		if(parent!=0){
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), parent->getTag());
			return 1;
		}
		return 0;
	}

};
#endif
