// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_SIMPLE_PROP_TYPES_FT_H
#define CH_SIMPLE_PROP_TYPES_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DT6gramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Chinese/relations/ch_RelationUtilities.h"

class ChineseSimplePropTypesFT : public P1RelationFeatureType {
public:
	ChineseSimplePropTypesFT() : P1RelationFeatureType(Symbol(L"simple-prop-types")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DT6gramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		RelationPropLink *link = o->getPropLink();
		if (!link->isEmpty() && !link->isNested()) {

			Symbol role1 = link->getArg1Role();
			Symbol role2 = link->getArg2Role();
			Symbol enttype1 = o->getMention1()->getEntityType().getName();
			Symbol enttype2 = o->getMention2()->getEntityType().getName();

			resultArray[0] = _new DT6gramFeature(this, state.getTag(),
				enttype1, enttype2, role1, role2, link->getTopStemmedPred());

			return 1;
		} else return 0;
	}

};

#endif
