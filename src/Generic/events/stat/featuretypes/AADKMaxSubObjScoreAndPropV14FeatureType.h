#ifndef AA_DK_MAXSUBOBJSCORE_ANDPROP_V14_FEATURE_TYPE_H
#define AA_DK_MAXSUBOBJSCORE_ANDPROP_V14_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventAAFeatureType.h"
#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/discTagger/DTQuintgramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <vector>


class AADKMaxSubObjScoreAndPropV14FeatureType : public EventAAFeatureType {
public:
	AADKMaxSubObjScoreAndPropV14FeatureType() : EventAAFeatureType(Symbol(L"aa-maxsubobjscore-prop-v14")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramIntFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state, DTFeature **resultArray) const
	{
		EventAAObservation *o = static_cast<EventAAObservation*>(state.getObservation(0));

		Symbol eventType = o->getEventType();

		Symbol roleInTriggerProp = o->getCandidateRoleInTriggerProp();
		if(roleInTriggerProp.is_null()) {
			roleInTriggerProp = SymbolConstants::nullSymbol;
		}

		Symbol candidateRoleInCP = o->getCandidateRoleInCP();
		if(candidateRoleInCP.is_null()) {
			candidateRoleInCP = SymbolConstants::nullSymbol;
		}

		Symbol eventRoleInCP = o->getEventRoleInCP();
		if(eventRoleInCP.is_null()) {
			eventRoleInCP = SymbolConstants::nullSymbol;
		}


		unsigned featureIndex = 0;

		if(o->getAADistributionalKnowledge().useFeatureVariant(Symbol(L"anchor SB"))) {
			int maxSubObjScoreSB = o->getAADistributionalKnowledge().maxSubObjScoreSB();
			resultArray[featureIndex++] = _new DTQuintgramIntFeature(this, state.getTag(), eventType, roleInTriggerProp, candidateRoleInCP, eventRoleInCP, maxSubObjScoreSB);
		}

		// 10/15/2013, Yee Seng Chan: this 'anchor bins' feature is temporarily here as an experiment, alternate way of representing scores
		/*
		if(o->getAADistributionalKnowledge().useFeatureVariant(Symbol(L"anchor bins"))) {
			std::vector<Symbol> maxSubObjScoreFea = o->getAADistributionalKnowledge().maxSubObjScoreFea();
			for(unsigned i=0; i<maxSubObjScoreFea.size(); i++) {
				resultArray[featureIndex++] = _new DT6gramFeature(this, state.getTag(), eventType, roleInTriggerProp, candidateRoleInCP, eventRoleInCP, maxSubObjScoreFea[i]);
			}
		}
		*/
		return featureIndex;
	}
};

#endif
