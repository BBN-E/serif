// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_SIMPLE_PROP_WC_FT_H
#define es_SIMPLE_PROP_WC_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuintgramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Spanish/relations/es_RelationUtilities.h"

class SpanishSimplePropWCFT : public P1RelationFeatureType {
public:
	SpanishSimplePropWCFT() : P1RelationFeatureType(Symbol(L"simple-prop-wc")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramIntFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		// later, put in a reversal for in-<sub>, etc.

		RelationPropLink *link = o->getPropLink();
		if (!link->isEmpty() && !link->isNested()) {
			Symbol stemmedPredicate = link->getTopStemmedPred();

			Symbol role1 = link->getArg1Role();
			Symbol role2 = link->getArg2Role();
			//Symbol enttype1 = o->getMention1()->getEntityType().getName();
			//Symbol enttype2 = o->getMention2()->getEntityType().getName();

			//When one of the argument role is <sub>, it will be switched to the first argument.
			//Entity types should be switched accordingly.
			Symbol enttype1 = link->getArg1Ment(o->getMentionSet())->getEntityType().getName();
			Symbol enttype2 = link->getArg2Ment(o->getMentionSet())->getEntityType().getName();

			int nfeatures = 0;
			if (link->getWCTop().c12() != 0)
				resultArray[nfeatures++] = _new DTQuintgramIntFeature(this, state.getTag(),
					enttype1, enttype2, role1, role2, link->getWCTop().c12());

			if (link->getWCTop().c16() != 0)
				resultArray[nfeatures++] = _new DTQuintgramIntFeature(this, state.getTag(),
					enttype1, enttype2, role1, role2, link->getWCTop().c16());

			if (link->getWCTop().c20() != 0)
				resultArray[nfeatures++] = _new DTQuintgramIntFeature(this, state.getTag(),
					enttype1, enttype2, role1, role2, link->getWCTop().c20());

			return nfeatures;
		} else return 0;
	}

};

#endif
