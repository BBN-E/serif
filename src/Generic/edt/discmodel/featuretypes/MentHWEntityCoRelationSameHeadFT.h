// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTHW_ENTITY_CO_RELATION_SAME_HEAD_FT_H
#define MENTHW_ENTITY_CO_RELATION_SAME_HEAD_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/theories/SynNode.h"

#include "Generic/theories/Mention.h"


class MentHWEntityCoRelationSameHeadFT : public DTCorefFeatureType {
public:
	MentHWEntityCoRelationSameHeadFT() : DTCorefFeatureType(Symbol(L"ment-hw-entity-co-relation-same-hw2")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		Mention *ment = o->getMention();
		Symbol mentHW = ment->getNode()->getHeadWord();
		Symbol type, hw2;
		Symbol pos;

		bool found = DescLinkFeatureFunctions::checkCommonRelationsTypesPosAnd2ndHW(ment/*, o->getMentionSet()*/, o->getEntity(),
																  o->getEntitySet(), o->getDocumentRelMentionSet(),
																  type, hw2, pos);
		if(found){
			resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), mentHW, type, pos);
		}else{
			return 0;
		}

		return 1;
	}

};
#endif
