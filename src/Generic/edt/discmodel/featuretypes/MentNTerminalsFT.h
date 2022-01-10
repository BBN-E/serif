// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENT_NTERMINALS_FT_H
#define MENT_NTERMINALS_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"

#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"


class MentNTerminalsFT : public DTCorefFeatureType {
public:
	MentNTerminalsFT() : DTCorefFeatureType(Symbol(L"ment-num-terminals")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTNoneCorefObservation *o = static_cast<DTNoneCorefObservation*>(
				state.getObservation(0));
		EntityType mentEntType = o->getMention()->getEntityType();
		Symbol mentEntTypeSymbol = (!mentEntType.isDetermined()) ? NO_ENTITY_TYPE : mentEntType.getName();

		n_terms = o->getMention()->getNode()->getNTerminals();
		if (n_terms > 10)
			n_terms =10;

		resultArray[0] = _new DTIntFeature(this, mentEntTypeSymbol, n_terms);
		return 1;
	}

	mutable int n_terms;
};
#endif
