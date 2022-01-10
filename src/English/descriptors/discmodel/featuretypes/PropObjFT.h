// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_PROP_OBJ_DISC_FT_H
#define EN_PROP_OBJ_DISC_FT_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"


class EnglishPropObjFT : public P1DescFeatureType {
public:
	EnglishPropObjFT() : P1DescFeatureType(Symbol(L"obj-prop")) {}

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
		bool discard = false;
		for(int i =0; i< o->getNValidProps(); i++){
			Proposition* prop = o->getValidProp(i);
			if (discard) break;

			if(prop->getPredType() == Proposition::VERB_PRED)
			{
				for(int j =0; j< prop->getNArgs(); j++)
				{
					if(prop->getArg(j)->getType() == Argument::MENTION_ARG){
						if((prop->getArg(j)->getRoleSym() == Argument::OBJ_ROLE) && 
							(prop->getArg(j)->getMention(o->getMentionSet())->getNode() == o->getNode()))
						{
							if (n_results >= (DTFeatureType::MAX_FEATURES_PER_EXTRACTION-2)){
								discard = true;
								break;
							}else{
								resultArray[n_results++] = _new DTTrigramFeature(this, 
									state.getTag(), prop->getPredHead()->getHeadWord(),
									o->getNode()->getHeadWord());
								resultArray[n_results++] = _new DTTrigramFeature(this, 
									state.getTag(), prop->getPredHead()->getHeadWord(),
									Symbol(L"-BACKEDOFF_FEAT-"));
							}
						}
					}
				}
			}
		}
		if (discard) {
			SessionLogger::warn("DT_feature_limit") <<"EnglishPropObjFT discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
		}
		return n_results;
	}
};

#endif
