#ifndef AA_DK_ANCHORARGPMI_ANDPROP_V20_RT_FEATURE_TYPE_H
#define AA_DK_ANCHORARGPMI_ANDPROP_V20_RT_FEATURE_TYPE_H

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


class AADKAnchorArgPmiAndPropV20RTFeatureType : public EventAAFeatureType {
public:
	AADKAnchorArgPmiAndPropV20RTFeatureType() : EventAAFeatureType(Symbol(L"aa-anchorargpmi-prop-v20-rt")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramIntFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state, DTFeature **resultArray) const
	{
		EventAAObservation *o = static_cast<EventAAObservation*>(state.getObservation(0));

		Symbol reducedEventType = o->getReducedEventType();

		Symbol roleInTriggerProp = o->getCandidateRoleInTriggerProp();
		Symbol roleInTriggerPropFea = roleInTriggerProp;
		if(roleInTriggerProp.is_null())
			roleInTriggerPropFea = SymbolConstants::nullSymbol;

		Symbol candidateRoleInCP = o->getCandidateRoleInCP();
		Symbol candidateRoleInCPFea = candidateRoleInCP;
		if(candidateRoleInCP.is_null())
			candidateRoleInCPFea = SymbolConstants::nullSymbol;

		Symbol eventRoleInCP = o->getEventRoleInCP();
		Symbol eventRoleInCPFea = eventRoleInCP;
		if(eventRoleInCP.is_null())
			eventRoleInCPFea = SymbolConstants::nullSymbol;


		unsigned featureIndex = 0;

		if(o->getAADistributionalKnowledge().useFeatureVariant(Symbol(L"anchor SB"))) {
			int anchorArgPmiSB = o->getAADistributionalKnowledge().anchorArgPmiSB();
			if(roleInTriggerProp.is_null() && candidateRoleInCP.is_null() && eventRoleInCP.is_null()) 
				resultArray[featureIndex++] = _new DTQuintgramIntFeature(this, state.getTag(), reducedEventType, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, anchorArgPmiSB);
			else if(roleInTriggerProp.is_null() && (!candidateRoleInCP.is_null() || !eventRoleInCP.is_null()))
				resultArray[featureIndex++] = _new DTQuintgramIntFeature(this, state.getTag(), reducedEventType, SymbolConstants::nullSymbol, candidateRoleInCPFea, eventRoleInCPFea, anchorArgPmiSB);
			else if(!roleInTriggerProp.is_null() && candidateRoleInCP.is_null() && eventRoleInCP.is_null())
				resultArray[featureIndex++] = _new DTQuintgramIntFeature(this, state.getTag(), reducedEventType, roleInTriggerProp, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, anchorArgPmiSB);
			else {
				resultArray[featureIndex++] = _new DTQuintgramIntFeature(this, state.getTag(), reducedEventType, roleInTriggerPropFea, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, anchorArgPmiSB);
				resultArray[featureIndex++] = _new DTQuintgramIntFeature(this, state.getTag(), reducedEventType, SymbolConstants::nullSymbol, candidateRoleInCPFea, eventRoleInCPFea, anchorArgPmiSB);
			}
		}

		// 10/15/2013, Yee Seng Chan: this 'anchor bins' feature is temporarily here as an experiment, alternate way of representing scores
		/*
		if(o->getAADistributionalKnowledge().useFeatureVariant(Symbol(L"anchor bins"))) {
			std::vector<Symbol> anchorArgPmiFea = o->getAADistributionalKnowledge().anchorArgPmiFea();
			for(unsigned i=0; i<anchorArgPmiFea.size(); i++) {
				if(roleInTriggerProp.is_null() && candidateRoleInCP.is_null() && eventRoleInCP.is_null()) 
					resultArray[featureIndex++] = _new DTQuintgramIntFeature(this, state.getTag(), reducedEventType, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, anchorArgPmiFea[i]);
				else if(roleInTriggerProp.is_null() && (!candidateRoleInCP.is_null() || !eventRoleInCP.is_null()))
					resultArray[featureIndex++] = _new DTQuintgramIntFeature(this, state.getTag(), reducedEventType, SymbolConstants::nullSymbol, candidateRoleInCPFea, eventRoleInCPFea, anchorArgPmiFea[i]);
				else if(!roleInTriggerProp.is_null() && candidateRoleInCP.is_null() && eventRoleInCP.is_null())
					resultArray[featureIndex++] = _new DTQuintgramIntFeature(this, state.getTag(), reducedEventType, roleInTriggerProp, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, anchorArgPmiFea[i]);
				else {
					resultArray[featureIndex++] = _new DTQuintgramIntFeature(this, state.getTag(), reducedEventType, roleInTriggerPropFea, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, anchorArgPmiFea[i]);
					resultArray[featureIndex++] = _new DTQuintgramIntFeature(this, state.getTag(), reducedEventType, SymbolConstants::nullSymbol, candidateRoleInCPFea, eventRoleInCPFea, anchorArgPmiFea[i]);
				}
			}
		}
		*/
		return featureIndex;
	}
};

#endif
