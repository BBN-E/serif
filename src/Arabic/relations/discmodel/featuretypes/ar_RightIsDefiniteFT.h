// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_RIGHT_IS_DEFINITE_FT_H
#define AR_RIGHT_IS_DEFINITE_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Arabic/relations/ar_RelationUtilities.h"
#include "Arabic/parse/ar_STags.h"
#include "Arabic/common/ar_WordConstants.h"

class ArabicRightIsDefiniteFT : public P1RelationFeatureType {
public:
	ArabicRightIsDefiniteFT() : P1RelationFeatureType(Symbol(L"right-is-definite")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));
		
		Symbol head2 = o->getMention2()->getNode()->getHeadWord();
		Symbol temp2 = ArabicWordConstants::removeAl(head2);

		if (head2 != temp2) {
			resultArray[0] = _new DTMonogramFeature(this, state.getTag());
			return 1;
		}

		return 0;

	}

};

#endif
