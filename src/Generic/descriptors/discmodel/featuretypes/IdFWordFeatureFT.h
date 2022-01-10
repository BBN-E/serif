// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_P1DESC_IdFWORD_FEATURE_TYPE_H
#define D_T_P1DESC_IdFWORD_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/IdFWordFeatures.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"


class IdFWordFeatureFT : public P1DescFeatureType {
private:
	static IdFWordFeatures* _wordFeatures;
public:
	IdFWordFeatureFT() : P1DescFeatureType(Symbol(L"idfwordfeat")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));

		Symbol wordfeat = o->getIdFWordFeature();	
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), wordfeat);
		return 1;
	}

};
#endif
