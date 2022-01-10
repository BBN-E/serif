// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_REF_PROP_FT_H
#define CH_REF_PROP_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Chinese/relations/ch_RelationUtilities.h"

class ChineseRefPropFT : public P1RelationFeatureType {
public:
	ChineseRefPropFT() : P1RelationFeatureType(Symbol(L"ref-prop")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		RelationPropLink *link = o->getPropLink();
		if (!link->isEmpty() && !link->isNested()) {
			Symbol stemmedPredicate = 
				ChineseRelationUtilities::get()->stemPredicate(link->getTopProposition()->getPredSymbol(), 
				link->getTopProposition()->getPredType());

			if (link->getArg1Role() == Argument::REF_ROLE) {
				resultArray[0] = _new DTTrigramFeature(this, state.getTag(),
					o->getMention2()->getEntityType().getName(), stemmedPredicate);
				return 1;
			} else if (link->getArg2Role() == Argument::REF_ROLE) {
				resultArray[0] = _new DTTrigramFeature(this, state.getTag(),
					o->getMention1()->getEntityType().getName(), stemmedPredicate);
				return 1;
			} else return 0;
		} else return 0;
	}

};

#endif
