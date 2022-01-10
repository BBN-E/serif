#ifndef AA_DK_ASSOCPROPPMI_ANDPROP_V1_FEATURE_TYPE_H
#define AA_DK_ASSOCPROPPMI_ANDPROP_V1_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventAAFeatureType.h"
#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/discTagger/DTBigramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <vector>


class AADKAssocPropPmiAndPropV1FeatureType : public EventAAFeatureType {
public:
	AADKAssocPropPmiAndPropV1FeatureType() : EventAAFeatureType(Symbol(L"aa-assocproppmi-prop-v1")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramIntFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state, DTFeature **resultArray) const
	{
		EventAAObservation *o = static_cast<EventAAObservation*>(state.getObservation(0));

		Symbol roleInTriggerProp = o->getCandidateRoleInTriggerProp();
		if(roleInTriggerProp.is_null()) {
			roleInTriggerProp = SymbolConstants::nullSymbol;
		}


		unsigned featureIndex = 0;

		if(o->getAADistributionalKnowledge().useFeatureVariant(Symbol(L"anchor SB"))) {
			int assocPropPmiSB = o->getAADistributionalKnowledge().assocPropPmiSB();
			resultArray[featureIndex++] = _new DTBigramIntFeature(this, state.getTag(), roleInTriggerProp, assocPropPmiSB);
		}

		// 10/15/2013, Yee Seng Chan: this 'anchor bins' feature is temporarily here as an experiment, alternate way of representing scores
		/*
		if(o->getAADistributionalKnowledge().useFeatureVariant(Symbol(L"anchor bins"))) {
			std::vector<Symbol> assocPropPmiFea = o->getAADistributionalKnowledge().assocPropPmiFea();
			for(unsigned i=0; i<assocPropPmiFea.size(); i++) {
				resultArray[featureIndex++] = _new DTTrigramFeature(this, state.getTag(), roleInTriggerProp, assocPropPmiFea[i]);
			}
		}
		*/
		return featureIndex;
	}
};

#endif
