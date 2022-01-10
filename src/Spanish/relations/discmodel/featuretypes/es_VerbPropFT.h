// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_VERB_PROP_FT_H
#define es_VERB_PROP_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Spanish/relations/es_RelationUtilities.h"

class SpanishVerbPropFT : public P1RelationFeatureType {
public:
	SpanishVerbPropFT() : P1RelationFeatureType(Symbol(L"verb-prop")) {}

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
			Symbol head1 = o->getMention1()->getNode()->getHeadWord();
			Symbol head2 = o->getMention2()->getNode()->getHeadWord();
			resultArray[0] = _new DTQuadgramFeature(this, state.getTag(),
					enttype1, enttype2, o->getVerbinProp());
			resultArray[1] = _new DTQuadgramFeature(this, state.getTag(),
					enttype1, head2, o->getVerbinProp());
			resultArray[2] = _new DTQuadgramFeature(this, state.getTag(),
					head1, enttype2, o->getVerbinProp());
			resultArray[3] = _new DTQuadgramFeature(this, state.getTag(),
					head1, head2, o->getVerbinProp());
			return 4;
			*/

			// option 2 - use stemmed head words
			Symbol head1 = o->getStemmedHeadofMent1();
			Symbol head2 = o->getStemmedHeadofMent2();
			resultArray[0] = _new DTQuadgramFeature(this, state.getTag(),
					enttype1, enttype2, o->getStemmedVerbinProp());
			resultArray[1] = _new DTQuadgramFeature(this, state.getTag(),
					enttype1, head2, o->getStemmedVerbinProp());
			resultArray[2] = _new DTQuadgramFeature(this, state.getTag(),
					head1, enttype2, o->getStemmedVerbinProp());
			resultArray[3] = _new DTQuadgramFeature(this, state.getTag(),
					head1, head2, o->getStemmedVerbinProp());
			return 4;
			
		}else return 0;
	}

};

#endif

