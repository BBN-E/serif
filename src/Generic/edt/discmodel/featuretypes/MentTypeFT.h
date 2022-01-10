// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTENTTYPE_FT_H
#define MENTENTTYPE_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"


class MentTypeFT : public DTCorefFeatureType {
public:
	MentTypeFT() : DTCorefFeatureType(Symbol(L"ment-ent-type")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTNoneCorefObservation *o = static_cast<DTNoneCorefObservation*>(
			state.getObservation(0));
		EntityType entType = o->getMention()->getEntityType();
		if (!entType.isDetermined())
			return 0;
		Symbol mentEntTypeSym = entType.getName();
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), mentEntTypeSym);
		return 1;
	}

};
#endif
