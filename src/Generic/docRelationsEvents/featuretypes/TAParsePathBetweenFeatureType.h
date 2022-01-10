// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TA_PARSE_PATH_BTWN_FT_H
#define TA_PARSE_PATH_BTWN_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/docRelationsEvents/RelationTimexArgFeatureType.h"
#include "Generic/docRelationsEvents/RelationTimexArgObservation.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"

/* This is currently broken , but it's not used so it's not compiled.
Instead of using a DTQuadgramFeature, it should use a DTBigram2StringFeature
which is not currently implemented */

class TAParsePathBetweenFeatureType : public RelationTimexArgFeatureType {
public:
	TAParsePathBetweenFeatureType() : RelationTimexArgFeatureType(Symbol(L"parse-path-btwn")) {}
	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol, 
									SymbolConstants::nullSymbol,
									SymbolConstants::nullSymbol,
									SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationTimexArgObservation *o = static_cast<RelationTimexArgObservation*>(
			state.getObservation(0));

		Symbol candPath = o->getConnectingCandParsePath();
		Symbol predicatePath = o->getConnectingPredicateParsePath();

		resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), 
			o->getRelationType(), candPath, predicatePath);

		return 1;
	}

};

#endif
