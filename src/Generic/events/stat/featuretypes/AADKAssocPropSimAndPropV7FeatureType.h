#ifndef AA_DK_ASSOCPROPSIM_ANDPROP_V7_FEATURE_TYPE_H
#define AA_DK_ASSOCPROPSIM_ANDPROP_V7_FEATURE_TYPE_H

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
#include <vector>


class AADKAssocPropSimAndPropV7FeatureType : public EventAAFeatureType {
public:
	AADKAssocPropSimAndPropV7FeatureType() : EventAAFeatureType(Symbol(L"aa-assocpropsim-prop-v7")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramIntFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state, DTFeature **resultArray) const
	{
		EventAAObservation *o = static_cast<EventAAObservation*>(state.getObservation(0));

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
			int assocPropSimSB = o->getAADistributionalKnowledge().assocPropSimSB();
			resultArray[featureIndex++] = _new DTTrigramIntFeature(this, state.getTag(), candidateRoleInCP, eventRoleInCP, assocPropSimSB);
		}

		// 10/15/2013, Yee Seng Chan: this 'anchor bins' feature is temporarily here as an experiment, alternate way of representing scores
		/*
		if(o->getAADistributionalKnowledge().useFeatureVariant(Symbol(L"anchor bins"))) {
			std::vector<Symbol> assocPropSimFea = o->getAADistributionalKnowledge().assocPropSimFea();
			for(unsigned i=0; i<assocPropSimFea.size(); i++) {
				resultArray[featureIndex++] = _new DTQuadgramFeature(this, state.getTag(), candidateRoleInCP, eventRoleInCP, assocPropSimFea[i]);
			}
		}
		*/
		return featureIndex;
	}
};

#endif
