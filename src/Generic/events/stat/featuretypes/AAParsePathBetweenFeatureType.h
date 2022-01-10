// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AA_PARSE_PATH_BTWN_FT_H
#define AA_PARSE_PATH_BTWN_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventAAFeatureType.h"
#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"


class AAParsePathBetweenFeatureType : public EventAAFeatureType {
public:
	AAParsePathBetweenFeatureType() : EventAAFeatureType(Symbol(L"parse-path-btwn")) {}
	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol, 
									SymbolConstants::nullSymbol,
									SymbolConstants::nullSymbol,
									SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventAAObservation *o = static_cast<EventAAObservation*>(
			state.getObservation(0));

		Symbol candPath = o->getConnectingCandParsePath();
		Symbol triggerPath = o->getConnectingTriggerParsePath();

		resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), 
			o->getEventType(), candPath, triggerPath);

		return 1;
	}

};

#endif
