// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTHW_ENT_LST_PRO_FT_H
#define MENTHW_ENT_LST_PRO_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/theories/SynNode.h"

#include "Generic/theories/Mention.h"

/** fires for the entity last pronoun and the mention head-word (pronoun)
*/
class MentHWEntLastPronFT : public DTCorefFeatureType {
public:
	MentHWEntLastPronFT() : DTCorefFeatureType(Symbol(L"ment-hw-ent-lst-pron")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol );
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));

		Symbol menthw = DescLinkFeatureFunctions::findMentHeadWord(o->getMention()); 
		Symbol ent_pro = DescLinkFeatureFunctions::findEntLastPronoun(o->getEntity(), o->getEntitySet());
		if (!ent_pro.is_null()) {
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(), menthw, ent_pro);
			return 1;
		}
		return 0;
	}

};
#endif
