// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENTITY_RELATIONS_H
#define ENTITY_RELATIONS_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
//#include "Generic/theories/SynNode.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/DocumentRelMentionSet.h"

class EntityRelationsFT : public DTCorefFeatureType {
public:
	EntityRelationsFT() : DTCorefFeatureType(Symbol(L"entity-relations")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		Entity *entity = o->getEntity();
		EntitySet *entitySet = o->getEntitySet();
		DocumentRelMentionSet *docRelMentSet = o->getDocumentRelMentionSet();
		int n_features = 0;
		bool discard = false;
		for(int i=0; i<docRelMentSet->getNRelMentions(); i++){
			if (n_features >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION){
				discard = true;
				break;
			}
			RelMention *rm = docRelMentSet->getRelMention(i);
			for(int i=0; i<entity->getNMentions(); i++){
				Mention *ment = entitySet->getMention(entity->getMention(i));
				int mentUID = ment->getUID();
				if (n_features >= (DTFeatureType::MAX_FEATURES_PER_EXTRACTION-2)){
					discard = true;
					break;
				}
				if(mentUID == rm->getLeftMention()->getUID()){
					resultArray[n_features++] = _new DTTrigramFeature(this, state.getTag(), rm->getType(), DescLinkFeatureFunctions::LEFT);
				}
				if(mentUID == rm->getRightMention()->getUID()){
					resultArray[n_features++] = _new DTTrigramFeature(this, state.getTag(), rm->getType(), DescLinkFeatureFunctions::RIGHT);
				}
			}
		}
		if (discard) {
			SessionLogger::logger->reportInfoMessage()<<"EntityRelationsFT discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
		}
//		std::cerr<<"*******************************************"<<std::endl;
//		std::cerr<<"checking for state:"<<state.getTag()<<std::endl;

		return n_features;
	}

};
#endif
