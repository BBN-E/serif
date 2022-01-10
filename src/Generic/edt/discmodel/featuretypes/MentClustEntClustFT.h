// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTCLUST_ENTCLUST_FT_H
#define MENTCLUST_ENTCLUST_FT_H

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


class MentClustEntClustFT : public DTCorefFeatureType {
public:
	MentClustEntClustFT() : DTCorefFeatureType(Symbol(L"ment-clust-ent-clust")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol );
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(state.getObservation(0));
	
		std::set<Symbol> ent_clust = DescLinkFeatureFunctions::findEntClusters(o->getEntity(), o->getEntitySet());
		std::set<Symbol> ment_clust = DescLinkFeatureFunctions::findMentClusters(o->getMention());

		int n_feat = 0;
		for (std::set<Symbol>::iterator it1 = ment_clust.begin(); it1 != ment_clust.end(); ++it1) {
			for (std::set<Symbol>::iterator it2 = ent_clust.begin(); it2 != ent_clust.end(); ++it2) {
                resultArray[n_feat++] = _new DTTrigramFeature(this, state.getTag(), *it1, *it2);
				if (n_feat == DTCorefFeatureType::MAX_FEATURES_PER_EXTRACTION)
					return n_feat;
			}
		}
		return n_feat;
	}

};
#endif
