// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_NESTED_PROP_FT_H
#define EN_NESTED_PROP_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DT6gramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "English/relations/en_RelationUtilities.h"

class EnglishNestedPropFT : public P1RelationFeatureType {
public:
	EnglishNestedPropFT() : P1RelationFeatureType(Symbol(L"nested-prop")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DT6gramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		RelationPropLink *link = o->getPropLink();
		if (!link->isEmpty() && link->isRightNested()) {

			Symbol role1 = link->getArg1Role();
			Symbol intermed = link->getIntermedArgRole();
			Symbol role2 = link->getArg2Role();

			resultArray[0] = _new DT6gramFeature(this, state.getTag(),
				role1, link->getTopStemmedPred(), intermed, Symbol(L"___"), role2);

			return 1;
		} else return 0;
	}

};

#endif
