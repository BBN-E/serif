// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_PROP_COPULA_DISC_FT_H
#define EN_PROP_COPULA_DISC_FT_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"


class EnglishPropCopulaFT : public P1DescFeatureType {
public:
	EnglishPropCopulaFT() : P1DescFeatureType(Symbol(L"copula-prop")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));
		int n_results = 0;
		for(int i =0; i< o->getNValidProps(); i++){
			Proposition* prop = o->getValidProp(i);
			const Mention* subj = 0;
			const Mention* obj = 0;
			if(prop->getPredType() == Proposition::COPULA_PRED)
			{
				for(int j =0; j< prop->getNArgs(); j++)
				{
					if(prop->getArg(j)->getType() == Argument::MENTION_ARG){
						if((prop->getArg(j)->getRoleSym() == Argument::SUB_ROLE)){
							subj = (prop->getArg(j)->getMention(o->getMentionSet()));
						}
						else if((prop->getArg(j)->getRoleSym() == Argument::OBJ_ROLE)){
							obj = (prop->getArg(j)->getMention(o->getMentionSet()));
						}
					}
				}
			}
			if((subj == 0) || (obj == 0 )){
				;
			}
			else if((subj->getNode() == o->getNode()) || (obj->getNode() == o->getNode())){
				Symbol subj_hw = subj->getNode()->getHeadWord();
				Symbol obj_hw = obj->getNode()->getHeadWord();
				wchar_t buffer[1500];
				if(subj->getMentionType() == Mention::NAME){
					wcscpy(buffer, L"NAME:");
					wcscat(buffer, subj->getEntityType().getName().to_string());
					subj_hw = Symbol(buffer);
				}
				else if(obj->getMentionType() == Mention::NAME){
					wcscpy(buffer, L"NAME:");
					wcscat(buffer, obj->getEntityType().getName().to_string());
					obj_hw = Symbol(buffer);
				}
				if (n_results >= (DTFeatureType::MAX_FEATURES_PER_EXTRACTION-2)){
					SessionLogger::warn("DT_feature_limit") <<"EnglishPropCopulaFT discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
					break;
				}
				resultArray[n_results++] = _new DTTrigramFeature(this, 
									state.getTag(), subj_hw, obj_hw);
				if(subj->getNode() == o->getNode()){
                    resultArray[n_results++] = _new DTTrigramFeature(this, 
									state.getTag(), obj_hw,
									Symbol(L"-BACKEDOFF_FEAT-"));
				}
				else{
                    resultArray[n_results++] = _new DTTrigramFeature(this, 
									state.getTag(), subj_hw,
									Symbol(L"-BACKEDOFF_FEAT-"));

				}
			}
		}
		return n_results;
	}
};

#endif
