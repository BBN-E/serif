// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTMOD_ENTMOD_FT_H
#define MENTMOD_ENTMOD_FT_H

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


class MentModEntModFT : public DTCorefFeatureType {
public:
	MentModEntModFT() : DTCorefFeatureType(Symbol(L"ment-mod-ent-mod")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol );
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(state.getObservation(0));

		std::set<Symbol> ent_mods = DescLinkFeatureFunctions::getEntMods(o->getEntity(), o->getEntitySet());
		std::set<Symbol> ment_mods = DescLinkFeatureFunctions::getMentMods(o->getMention());

		int n_feat = 0;
		for (std::set<Symbol>::iterator it1 = ment_mods.begin(); it1 != ment_mods.end(); ++it1) {
			for (std::set<Symbol>::iterator it2 = ent_mods.begin(); it2 != ent_mods.end(); ++it2) {
                resultArray[n_feat++] = _new DTTrigramFeature(this, state.getTag(), *it1, *it2);
				if (n_feat == DTFeatureType::MAX_FEATURES_PER_EXTRACTION) 
					return n_feat;
			}
		}
		return n_feat;
	}

};
#endif
