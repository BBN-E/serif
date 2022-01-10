// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_ROLES_TYPES_FT_H
#define es_ROLES_TYPES_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Spanish/relations/es_RelationUtilities.h"

class SpanishRolesTypesFT : public P1RelationFeatureType {
public:
	SpanishRolesTypesFT() : P1RelationFeatureType(Symbol(L"roles-types")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
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

			resultArray[0] = _new DTQuintgramFeature(this, state.getTag(),
				role1, role2, o->getMention1()->getEntityType().getName(),
				o->getMention2()->getEntityType().getName());

			return 1;
		} else return 0;
	}

};

#endif
