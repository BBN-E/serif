#ifndef AA_DK_ANCHORARGPMI_ANDPROP_V6_FEATURE_TYPE_H
#define AA_DK_ANCHORARGPMI_ANDPROP_V6_FEATURE_TYPE_H

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


class AADKAnchorArgPmiAndPropV6FeatureType : public EventAAFeatureType {
public:
	AADKAnchorArgPmiAndPropV6FeatureType() : EventAAFeatureType(Symbol(L"aa-anchorargpmi-prop-v6")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramIntFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state, DTFeature **resultArray) const
	{
		EventAAObservation *o = static_cast<EventAAObservation*>(state.getObservation(0));

		Symbol anchor = o->getAADistributionalKnowledge().anchor();
		Symbol candidateType = o->getCandidateType();

		Symbol roleInTriggerProp = o->getCandidateRoleInTriggerProp();
		if(roleInTriggerProp.is_null()) {
			roleInTriggerProp = SymbolConstants::nullSymbol;
		}


		unsigned featureIndex = 0;

		if(o->getAADistributionalKnowledge().useFeatureVariant(Symbol(L"anchor SB"))) {
			int anchorArgPmiSB = o->getAADistributionalKnowledge().anchorArgPmiSB();
			resultArray[featureIndex++] = _new DTQuadgramIntFeature(this, state.getTag(), anchor, candidateType, roleInTriggerProp, anchorArgPmiSB);
		}

		// 10/15/2013, Yee Seng Chan: this 'anchor bins' feature is temporarily here as an experiment, alternate way of representing scores
		/*
		if(o->getAADistributionalKnowledge().useFeatureVariant(Symbol(L"anchor bins"))) {
			std::vector<int> anchorArgPmiFea = o->getAADistributionalKnowledge().anchorArgPmiFea();
			for(unsigned i=0; i<anchorArgPmiFea.size(); i++) {
				resultArray[featureIndex++] = _new DTQuadgramIntFeature(this, state.getTag(), anchor, candidateType, roleInTriggerProp, anchorArgPmiFea[i]);
			}
		}
		*/
		return featureIndex;
	}
};

#endif
