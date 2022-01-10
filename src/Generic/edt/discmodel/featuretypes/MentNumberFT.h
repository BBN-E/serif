// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTNUMBER_FT_H
#define MENTNUMBER_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"

// fires the mention 'number' as is predicted.
class MentNumberFT : public DTCorefFeatureType {
public:
	MentNumberFT() : DTCorefFeatureType(Symbol(L"ment-number")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTNoneCorefObservation *o = static_cast<DTNoneCorefObservation*>(
			state.getObservation(0));
		const MentionToSymbolMap *numberMap = o->getMentionNumberMapper();

		MentionUID mentUID = o->getMention()->getUID();
		const Symbol *number = numberMap->get(mentUID);
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), *number);
		return 1;
	}

};
#endif
