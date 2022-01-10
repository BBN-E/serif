// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_SIMPLE_PROP_WC_NO_TYPES_FT_H
#define EN_SIMPLE_PROP_WC_NO_TYPES_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "English/relations/en_RelationUtilities.h"

class EnglishSimplePropWCNoTypesFT : public P1RelationFeatureType {
public:
	EnglishSimplePropWCNoTypesFT() : P1RelationFeatureType(Symbol(L"simple-prop-wc-no-types")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramIntFeature(this, SymbolConstants::nullSymbol,
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

			Symbol role1 = link->getArgument1()->getRoleSym();
			Symbol role2 = link->getArgument2()->getRoleSym();

			int nfeatures = 0;
			if (link->getWCTop().c12() != 0)
				resultArray[nfeatures++] = _new DTTrigramIntFeature(this, state.getTag(),
					role1, role2, link->getWCTop().c12());

			if (link->getWCTop().c16() != 0)
				resultArray[nfeatures++] = _new DTTrigramIntFeature(this, state.getTag(),
					role1, role2, link->getWCTop().c16());

			if (link->getWCTop().c20() != 0)
				resultArray[nfeatures++] = _new DTTrigramIntFeature(this, state.getTag(),
					role1, role2, link->getWCTop().c20());

			return nfeatures;
		} else return 0;
	}

};

#endif
