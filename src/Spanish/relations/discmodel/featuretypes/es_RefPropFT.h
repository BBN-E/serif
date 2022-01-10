// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_REF_PROP_FT_H
#define es_REF_PROP_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Spanish/relations/es_RelationUtilities.h"

class SpanishRefPropFT : public P1RelationFeatureType {
public:
	SpanishRefPropFT() : P1RelationFeatureType(Symbol(L"ref-prop")) {}

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
		const MentionSet* mentSet = o->getMentionSet();
		if (!link->isEmpty() && !link->isNested()) {
			int index = -1;
			if (link->getArg1Role() == Argument::REF_ROLE) {
				index = link->getArg1Ment(mentSet)->getIndex();
			} else if (link->getArg2Role() == Argument::REF_ROLE) {
				index = link->getArg2Ment(mentSet)->getIndex();
			} 
			if (index != -1) {
				if (o->getMention1()->getIndex() == index) {
					resultArray[0] = _new DTTrigramFeature(this, state.getTag(),
						o->getMention2()->getEntityType().getName(), 
						link->getTopStemmedPred());
					return 1;
				} else if (o->getMention2()->getIndex() == index) {
					resultArray[0] = _new DTTrigramFeature(this, state.getTag(),
						o->getMention1()->getEntityType().getName(), 
						link->getTopStemmedPred());
					return 1;
				} 
			} 
		} 
		
		return 0;
	}

};

#endif
