#ifndef AA_DK_ANCHORARGSIM_ANDPROP_V19_FEATURE_TYPE_H
#define AA_DK_ANCHORARGSIM_ANDPROP_V19_FEATURE_TYPE_H

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


class AADKAnchorArgSimAndPropV19FeatureType : public EventAAFeatureType {
public:
	AADKAnchorArgSimAndPropV19FeatureType() : EventAAFeatureType(Symbol(L"aa-anchorargsim-prop-v19")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramIntFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state, DTFeature **resultArray) const
	{
		EventAAObservation *o = static_cast<EventAAObservation*>(state.getObservation(0));


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
			int anchorArgSimSB = o->getAADistributionalKnowledge().anchorArgSimSB();

			if(roleInTriggerProp.is_null() && candidateRoleInCP.is_null() && eventRoleInCP.is_null()) 
				resultArray[featureIndex++] = _new DTQuadgramIntFeature(this, state.getTag(), SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, anchorArgSimSB);
			else if(roleInTriggerProp.is_null() && (!candidateRoleInCP.is_null() || !eventRoleInCP.is_null()))
				resultArray[featureIndex++] = _new DTQuadgramIntFeature(this, state.getTag(), SymbolConstants::nullSymbol, candidateRoleInCPFea, eventRoleInCPFea, anchorArgSimSB);
			else if(!roleInTriggerProp.is_null() && candidateRoleInCP.is_null() && eventRoleInCP.is_null())
				resultArray[featureIndex++] = _new DTQuadgramIntFeature(this, state.getTag(), roleInTriggerProp, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, anchorArgSimSB);
			else {
				resultArray[featureIndex++] = _new DTQuadgramIntFeature(this, state.getTag(), roleInTriggerPropFea, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, anchorArgSimSB);
				resultArray[featureIndex++] = _new DTQuadgramIntFeature(this, state.getTag(), SymbolConstants::nullSymbol, candidateRoleInCPFea, eventRoleInCPFea, anchorArgSimSB);
			}
		}

		// 10/15/2013, Yee Seng Chan: this 'anchor bins' feature is temporarily here as an experiment, alternate way of representing scores
		/*
		if(o->getAADistributionalKnowledge().useFeatureVariant(Symbol(L"anchor bins"))) {
			std::vector<Symbol> anchorArgSimFea = o->getAADistributionalKnowledge().anchorArgSimFea();
			for(unsigned i=0; i<anchorArgSimFea.size(); i++) {
				if(roleInTriggerProp.is_null() && candidateRoleInCP.is_null() && eventRoleInCP.is_null()) 
					resultArray[featureIndex++] = _new DTQuintgramFeature(this, state.getTag(), SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, anchorArgSimFea[i]);
				else if(roleInTriggerProp.is_null() && (!candidateRoleInCP.is_null() || !eventRoleInCP.is_null()))
					resultArray[featureIndex++] = _new DTQuintgramFeature(this, state.getTag(), SymbolConstants::nullSymbol, candidateRoleInCPFea, eventRoleInCPFea, anchorArgSimFea[i]);
				else if(!roleInTriggerProp.is_null() && candidateRoleInCP.is_null() && eventRoleInCP.is_null())
					resultArray[featureIndex++] = _new DTQuintgramFeature(this, state.getTag(), roleInTriggerProp, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, anchorArgSimFea[i]);
				else {
					resultArray[featureIndex++] = _new DTQuintgramFeature(this, state.getTag(), roleInTriggerPropFea, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, anchorArgSimFea[i]);
					resultArray[featureIndex++] = _new DTQuintgramFeature(this, state.getTag(), SymbolConstants::nullSymbol, candidateRoleInCPFea, eventRoleInCPFea, anchorArgSimFea[i]);
				}
			}
		}
		*/
		return featureIndex;
	}
};

#endif
