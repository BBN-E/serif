// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTGENDER_FT_H
#define MENTGENDER_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"

/* output the mention gender as perdicted by the GenderGuesser
*/
class MentGenderFT : public DTCorefFeatureType {
public:
	MentGenderFT() : DTCorefFeatureType(Symbol(L"ment-gender")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTNoneCorefObservation *o = static_cast<DTNoneCorefObservation*>(
			state.getObservation(0));
		const MentionToSymbolMap *genderMap = o->getMentionGenderMapper();

		MentionUID mentUID = o->getMention()->getUID();
		const Symbol *gender = genderMap->get(mentUID);
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), *gender);
		return 1;
	}

};
#endif
