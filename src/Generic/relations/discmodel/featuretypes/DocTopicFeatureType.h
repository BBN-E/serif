// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DOC_TOPIC_FEATURE_TYPE_H
#define DOC_TOPIC_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/relations/xx_RelationUtilities.h"


class DocTopicFeatureType : public P1RelationFeatureType {
public:
	DocTopicFeatureType() : P1RelationFeatureType(Symbol(L"doc-topic")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		int n_results = 0;
		RelationPropLink *link = o->getPropLink();

		if (o->getDocumentTopic().is_null())
			return n_results;

		resultArray[n_results++] = _new DTTrigramFeature(this, state.getTag(), o->getDocumentTopic(), Symbol(L":NULL"));
		if (!link->isEmpty() && !link->isNested()) 
			resultArray[n_results++] = _new DTTrigramFeature(this, state.getTag(), o->getDocumentTopic(), link->getTopStemmedPred());
		return n_results;
	}
};

#endif
