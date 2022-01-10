// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_SIMPLE_VERB_PROP_FT_H
#define es_SIMPLE_VERB_PROP_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Spanish/relations/es_RelationUtilities.h"

class SpanishSimpleVerbPropFT : public P1RelationFeatureType {
public:
	SpanishSimpleVerbPropFT() : P1RelationFeatureType(Symbol(L"simple-verb-prop")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		if (o->getVerbinProp() != Symbol(L"NULL") ) {
	
			// option 1
			/*
			resultArray[0] = _new DTBigramFeature(this, state.getTag(),
					o->getVerbinProp());
			return 1;
			*/

			// option 2 - use stemmed head words
			resultArray[0] = _new DTBigramFeature(this, state.getTag(),
					o->getStemmedVerbinProp());
			
			return 1;
			
		}else return 0;
	}

};

#endif

