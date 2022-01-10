#ifndef AA_DK_CONTENTWORDS_APPMI_V3_FEATURE_TYPE_H
#define AA_DK_CONTENTWORDS_APPMI_V3_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventAAFeatureType.h"
#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/discTagger/DTTrigramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <set>


class AADKContentWordsAPPmiV3FeatureType : public EventAAFeatureType {
public:
	AADKContentWordsAPPmiV3FeatureType() : EventAAFeatureType(Symbol(L"aa-contentwordsappmi-v3")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramIntFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state, DTFeature **resultArray) const
	{
		EventAAObservation *o = static_cast<EventAAObservation*>(state.getObservation(0));

		Symbol anchor = o->getAADistributionalKnowledge().anchor();

		std::set<Symbol> contentWords = o->contentWords();


		unsigned featureIndex = 0;

		if(o->getAADistributionalKnowledge().useFeatureVariant(Symbol(L"anchor SB"))) {
			int assocPropPmiSB = o->getAADistributionalKnowledge().assocPropPmiSB();
			for(set<Symbol>::iterator it=contentWords.begin(); it!=contentWords.end(); it++) {
				resultArray[featureIndex++] = _new DTTrigramIntFeature(this, state.getTag(), anchor, (*it), assocPropPmiSB);
			}
		}

		// 10/15/2013, Yee Seng Chan: this 'anchor bins' feature is temporarily here as an experiment, alternate way of representing scores
		/*
		if(o->getAADistributionalKnowledge().useFeatureVariant(Symbol(L"anchor bins"))) {
			std::vector<Symbol> assocPropPmiFea = o->getAADistributionalKnowledge().assocPropPmiFea();
			for(set<Symbol>::iterator it=contentWords.begin(); it!=contentWords.end(); it++) {
				for(unsigned i=0; i<assocPropPmiFea.size(); i++) {
					resultArray[featureIndex++] = _new DTQuadgramFeature(this, state.getTag(), anchor, (*it), assocPropPmiFea[i]);
				}
			}
		}
		*/
		return featureIndex;
	}
};

#endif
