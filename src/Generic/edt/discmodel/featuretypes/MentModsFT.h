// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENT_MODS_FT_H
#define MENT_MODS_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"

class MentModsFT : public DTCorefFeatureType {
public:
	MentModsFT() : DTCorefFeatureType(Symbol(L"ment-mods")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(state.getObservation(0));

		std::set<Symbol> results = DescLinkFeatureFunctions::getMentMods(o->getMention());

		int n_feat = 0;
		for (std::set<Symbol>::iterator it = results.begin(); it != results.end(); ++it) {
			resultArray[n_feat++] = _new DTBigramFeature(this, state.getTag(), *it);
			if (n_feat == DTCorefFeatureType::MAX_FEATURES_PER_EXTRACTION) break;
		}
		return n_feat;
	}

};
#endif
