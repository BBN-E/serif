// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTHW_ENTMOD_FT_H
#define MENTHW_ENTMOD_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/theories/SynNode.h"

#include "Generic/theories/Mention.h"


class MentHWEntModFT : public DTCorefFeatureType {
public:
	MentHWEntModFT() : DTCorefFeatureType(Symbol(L"ment-hw-ent-mod")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol );
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(state.getObservation(0));

		Symbol menthw = DescLinkFeatureFunctions::findMentHeadWord(o->getMention()); 
		std::set<Symbol> ent_mods = DescLinkFeatureFunctions::getEntMods(o->getEntity(), o->getEntitySet());
		
		int n_feat = 0;
		for (std::set<Symbol>::iterator it = ent_mods.begin(); it != ent_mods.end(); ++it) {
			resultArray[n_feat++] = _new DTTrigramFeature(this, state.getTag(), menthw, *it);
			if (n_feat == DTFeatureType::MAX_FEATURES_PER_EXTRACTION) break;
		}
		return n_feat;
	}

};
#endif
