// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_LEFT_IS_DEFINITE_FT_H
#define AR_LEFT_IS_DEFINITE_FT_H

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

class ArabicLeftIsDefiniteFT : public P1RelationFeatureType {
public:
	ArabicLeftIsDefiniteFT() : P1RelationFeatureType(Symbol(L"left-is-definite")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));
		
		Symbol head1 = o->getMention1()->getNode()->getHeadWord();
		Symbol temp1 = ArabicWordConstants::removeAl(head1);

		if (head1 != temp1) {
			resultArray[0] = _new DTMonogramFeature(this, state.getTag());
			return 1;
		}

		return 0;

	}

};

#endif
