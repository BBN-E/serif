// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_SIMPLE_VERB_PROP_HEAD_FT_H
#define EN_SIMPLE_VERB_PROP_HEAD_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "English/relations/en_RelationUtilities.h"

class EnglishSimpleVerbPropwithTypeFT : public P1RelationFeatureType {
public:
	EnglishSimpleVerbPropwithTypeFT() : P1RelationFeatureType(Symbol(L"simple-verb-prop-type")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		if (o->getVerbinProp() != Symbol(L"NULL") ) {
			Symbol enttype1 = o->getMention1()->getEntityType().getName();
			Symbol enttype2 = o->getMention2()->getEntityType().getName();

			// option 1
			/*
			resultArray[0] = _new DTQuadgramFeature(this, state.getTag(),
					enttype1, enttype2, o->getVerbinProp());
			return 1;
			*/

			// option 2 - use stemmed head words
			resultArray[0] = _new DTQuadgramFeature(this, state.getTag(),
					enttype1, enttype2, o->getStemmedVerbinProp());
			
			return 1;
			
		}else return 0;
	}

};

#endif

