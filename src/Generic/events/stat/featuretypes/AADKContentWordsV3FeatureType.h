#ifndef AA_DK_CONTENTWORDS_V3_FEATURE_TYPE_H
#define AA_DK_CONTENTWORDS_V3_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventAAFeatureType.h"
#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <set>


class AADKContentWordsV3FeatureType : public EventAAFeatureType {
public:
	AADKContentWordsV3FeatureType() : EventAAFeatureType(Symbol(L"aa-contentwords-v3")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state, DTFeature **resultArray) const
	{
		EventAAObservation *o = static_cast<EventAAObservation*>(state.getObservation(0));

		Symbol anchor = o->getAADistributionalKnowledge().anchor();


		unsigned featureIndex = 0;

		std::set<Symbol> contentWords = o->contentWords();
		for(set<Symbol>::iterator it=contentWords.begin(); it!=contentWords.end(); it++) {
			resultArray[featureIndex++] = _new DTTrigramFeature(this, state.getTag(), anchor, (*it));
		}

		return featureIndex;
	}
};

#endif
