// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTHW_ENTITY_CO_RELATION_FT_H
#define MENTHW_ENTITY_CO_RELATION_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/theories/SynNode.h"

#include "Generic/theories/Mention.h"

class MentHWEntityCoRelationFT : public DTCorefFeatureType {
public:
	MentHWEntityCoRelationFT() : DTCorefFeatureType(Symbol(L"ment-hw-entity-co-relation")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		Mention *ment = o->getMention();
		Symbol menthw = ment->getNode()->getHeadWord();

		Symbol type = DescLinkFeatureFunctions::checkCommonRelationsTypes(ment, o->getEntity(),
			o->getEntitySet(), o->getDocumentRelMentionSet());
		if(type != DescLinkFeatureFunctions::NONE_SYMBOL)
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(), menthw, type);
		else
			return 0;

		return 1;
	}

};
#endif
