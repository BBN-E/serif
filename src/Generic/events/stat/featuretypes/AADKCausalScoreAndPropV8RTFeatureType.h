#ifndef AA_DK_CAUSALSCORE_ANDPROP_V8_RT_FEATURE_TYPE_H
#define AA_DK_CAUSALSCORE_ANDPROP_V8_RT_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventAAFeatureType.h"
#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/discTagger/DTQuadgramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <vector>


class AADKCausalScoreAndPropV8RTFeatureType : public EventAAFeatureType {
public:
	AADKCausalScoreAndPropV8RTFeatureType() : EventAAFeatureType(Symbol(L"aa-causalscore-prop-v8-rt")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramIntFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state, DTFeature **resultArray) const
	{
		EventAAObservation *o = static_cast<EventAAObservation*>(state.getObservation(0));

		Symbol reducedEventType = o->getReducedEventType();

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
			int causalScoreSB = o->getAADistributionalKnowledge().causalScoreSB();
			resultArray[featureIndex++] = _new DTQuadgramIntFeature(this, state.getTag(), reducedEventType, candidateRoleInCP, eventRoleInCP, causalScoreSB);
		}

		// 10/15/2013, Yee Seng Chan: this 'anchor bins' feature is temporarily here as an experiment, alternate way of representing scores
		/*
		if(o->getAADistributionalKnowledge().useFeatureVariant(Symbol(L"anchor bins"))) {
			std::vector<Symbol> causalScoreFea = o->getAADistributionalKnowledge().causalScoreFea();
			for(unsigned i=0; i<causalScoreFea.size(); i++) {
				resultArray[featureIndex++] = _new DTQuintgramFeature(this, state.getTag(), reducedEventType, candidateRoleInCP, eventRoleInCP, causalScoreFea[i]);
			}
		}
		*/
		return featureIndex;
	}
};

#endif
