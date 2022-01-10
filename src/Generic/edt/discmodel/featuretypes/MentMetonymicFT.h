// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTMETONYMIC_FT_H
#define MENTMETONYMIC_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"

/* fires wether a mention is metonymic. if so it fires it GPE role too.
*/
class MentMetonymicFT: public DTCorefFeatureType {
public:
	MentMetonymicFT() : DTCorefFeatureType(Symbol(L"ment-metonymic")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol
			, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTNoneCorefObservation *o = static_cast<DTNoneCorefObservation*>(
			state.getObservation(0));

		const Mention *ment = o->getMention();
		if(ment->isMetonymyMention()){
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(), ment->getEntityType().getName(), SymbolConstants::nullSymbol);
			if(ment->getEntityType().matchesGPE() && ment->hasRoleType()){
					resultArray[1] = _new DTTrigramFeature(this, state.getTag(), ment->getEntityType().getName(), ment->getRoleType().getName());
				return 2;
			}
			return 1;
		}
		return 0;
	}

};
#endif
