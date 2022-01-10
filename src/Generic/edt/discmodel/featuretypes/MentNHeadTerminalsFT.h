// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENT_NHEADTERMINALS_FT_H
#define MENT_NHEADTERMINALS_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"

#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"


class MentNHeadTerminalsFT : public DTCorefFeatureType {
public:
	MentNHeadTerminalsFT() : DTCorefFeatureType(Symbol(L"ment-num-head-terminals")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTNoneCorefObservation *o = static_cast<DTNoneCorefObservation*>(
				state.getObservation(0));
		EntityType mentEntType = o->getMention()->getEntityType();

		Symbol mentEntTypeSymbol;
		if(!mentEntType.isDetermined())
			mentEntTypeSymbol = NO_ENTITY_TYPE;
		else
			mentEntTypeSymbol = mentEntType.getName();

		resultArray[0] = _new DTIntFeature(this, mentEntTypeSymbol, o->getMention()->getHead()->getNTerminals());
		return 1;
	}

};
#endif
