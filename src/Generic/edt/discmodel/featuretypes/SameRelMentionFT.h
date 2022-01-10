// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SAME_REL_MENTION_FT_H
#define SAME_REL_MENTION_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"
#include "Generic/theories/RelMentionSet.h"
//#include "Generic/theories/RelMentionType.h";

class SameRelMentionFT : public DTCorefFeatureType {
public:
	SameRelMentionFT() : DTCorefFeatureType(Symbol(L"in-same-relmention")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		Mention * ment = o->getMention();
		Entity *ent = o->getEntity();
		RelMentionSet *rmSet = o->getDocumentRelMentionSet()->getSentenceRelMentionSet(ment->getSentenceNumber());
		EntitySet *entitySet = o->getEntitySet();
		Symbol mentHW = ment->getNode()->getHeadWord();

		bool found = false;
		Symbol type;
		for(int i=0; i<rmSet->getNRelMentions(); i++){
			if(found) break;
			RelMention *rm = rmSet->getRelMention(i);
			if(rm->getLeftMention()->getUID()==ment->getUID()){
				for(int j=0; j<ent->getNMentions(); j++){
					Mention* entMent = entitySet->getMention(ent->getMention(j)); 
					if(rm->getRightMention()==entMent){
						found=true;
						type = rm->getType();
						break;
					}
				}
			}
			if(found) break;
			if(rm->getRightMention()==ment){
				for(int j=0; j<ent->getNMentions(); j++){
					Mention* entMent = entitySet->getMention(ent->getMention(j)); 
					if(rm->getLeftMention()->getUID()==entMent->getUID()){
						found=true;
						type = rm->getType();
						break;
					}
				}
			}
		}

		if(found){
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), mentHW);
//			cerr<<"feature same-relation fires"<<endl;
			return 1;
		}
		return 0;
	}

};
#endif
