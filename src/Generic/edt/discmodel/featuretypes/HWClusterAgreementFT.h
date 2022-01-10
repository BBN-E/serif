// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HWCLUSTER_AGREEMENT_RATIO_FT_H
#define HWCLUSTER_AGREEMENT_RATIO_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class HWClusterAgreementFT : public DTCorefFeatureType {
public:
	HWClusterAgreementFT() : DTCorefFeatureType(Symbol(L"hw-cluster-agreement")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		Symbol ratio = DescLinkFeatureFunctions::getClusterScore(o->getMention(), 
			o->getEntity(), o->getEntitySet());
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), ratio);
		return 1;
	}

};
#endif
