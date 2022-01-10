#ifndef AA_DK_ASSOCPROPSIM_ANDPROP_V21_FEATURE_TYPE_H
#define AA_DK_ASSOCPROPSIM_ANDPROP_V21_FEATURE_TYPE_H

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


class AADKAssocPropSimAndPropV21FeatureType : public EventAAFeatureType {
public:
	AADKAssocPropSimAndPropV21FeatureType() : EventAAFeatureType(Symbol(L"aa-assocpropsim-prop-v21")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramIntFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state, DTFeature **resultArray) const
	{
		EventAAObservation *o = static_cast<EventAAObservation*>(state.getObservation(0));

		Symbol anchor = o->getAADistributionalKnowledge().anchor();

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
			int assocPropSimSB = o->getAADistributionalKnowledge().assocPropSimSB();
			if(roleInTriggerProp.is_null() && candidateRoleInCP.is_null() && eventRoleInCP.is_null()) 
				resultArray[featureIndex++] = _new DTQuintgramIntFeature(this, state.getTag(), anchor, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, assocPropSimSB);
			else if(roleInTriggerProp.is_null() && (!candidateRoleInCP.is_null() || !eventRoleInCP.is_null()))
				resultArray[featureIndex++] = _new DTQuintgramIntFeature(this, state.getTag(), anchor, SymbolConstants::nullSymbol, candidateRoleInCPFea, eventRoleInCPFea, assocPropSimSB);
			else if(!roleInTriggerProp.is_null() && candidateRoleInCP.is_null() && eventRoleInCP.is_null())
				resultArray[featureIndex++] = _new DTQuintgramIntFeature(this, state.getTag(), anchor, roleInTriggerProp, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, assocPropSimSB);
			else {
				resultArray[featureIndex++] = _new DTQuintgramIntFeature(this, state.getTag(), anchor, roleInTriggerPropFea, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, assocPropSimSB);
				resultArray[featureIndex++] = _new DTQuintgramIntFeature(this, state.getTag(), anchor, SymbolConstants::nullSymbol, candidateRoleInCPFea, eventRoleInCPFea, assocPropSimSB);
			}
		}

		// 10/15/2013, Yee Seng Chan: this 'anchor bins' feature is temporarily here as an experiment, alternate way of representing scores
		/*
		if(o->getAADistributionalKnowledge().useFeatureVariant(Symbol(L"anchor bins"))) {
			std::vector<Symbol> assocPropSimFea = o->getAADistributionalKnowledge().assocPropSimFea();
			for(unsigned i=0; i<assocPropSimFea.size(); i++) {
				if(roleInTriggerProp.is_null() && candidateRoleInCP.is_null() && eventRoleInCP.is_null()) 
					resultArray[featureIndex++] = _new DT6gramFeature(this, state.getTag(), anchor, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, assocPropSimFea[i]);
				else if(roleInTriggerProp.is_null() && (!candidateRoleInCP.is_null() || !eventRoleInCP.is_null()))
					resultArray[featureIndex++] = _new DT6gramFeature(this, state.getTag(), anchor, SymbolConstants::nullSymbol, candidateRoleInCPFea, eventRoleInCPFea, assocPropSimFea[i]);
				else if(!roleInTriggerProp.is_null() && candidateRoleInCP.is_null() && eventRoleInCP.is_null())
					resultArray[featureIndex++] = _new DT6gramFeature(this, state.getTag(), anchor, roleInTriggerProp, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, assocPropSimFea[i]);
				else {
					resultArray[featureIndex++] = _new DT6gramFeature(this, state.getTag(), anchor, roleInTriggerPropFea, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, assocPropSimFea[i]);
					resultArray[featureIndex++] = _new DT6gramFeature(this, state.getTag(), anchor, SymbolConstants::nullSymbol, candidateRoleInCPFea, eventRoleInCPFea, assocPropSimFea[i]);
				}
			}
		}
		*/
		return featureIndex;
	}
};

#endif
