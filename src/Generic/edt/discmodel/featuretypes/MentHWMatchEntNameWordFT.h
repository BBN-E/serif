// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTHW_MATCHENTNAMEWORD_FT_H
#define MENTHW_MATCHENTNAMEWORD_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"
#include "Generic/theories/EntitySet.h"


class MentHWMatchEntNameWordFT : public DTCorefFeatureType {
public:
	MentHWMatchEntNameWordFT() : DTCorefFeatureType(Symbol(L"ment-hw-match-name-word")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		
		Symbol ment_hw = o->getMention()->getNode()->getHeadWord();
		//by the time descriptor recognition is called, all names will be in their
		//final entities, 
		const EntitySet* es = o->getEntitySet();
		//std::cout<<"entity set: "<<std::endl;
		//es->dump(std::cout, 3);
		//std::cout<<std::endl;
		Symbol name_words[20];
		int i, j, k;
		int match_count = 0;
		int match_entity_id = 0;
		for(i = 0; i< es->getNEntities(); i++){
			Entity* oth_ent = es->getEntity(i);
			//std::cout<<"      entity : "<<i<<std::endl;
			//oth_ent->dump(std::cout, 8);
			//std::cout<<std::endl;
		
			bool found_match =false;
			for(j = 0; j< oth_ent->getNMentions(); j++){
				if(found_match){
					break;
				}
				Mention* ent_ment = es->getMention(oth_ent->getMention(j));
				
				//std::cout<<"      mention : "<<j<<std::endl;
				//ent_ment->dump(std::cout, 11);
				///std::cout<<std::endl;
				if(ent_ment->getMentionType() == Mention::NAME){
					int nwords = ent_ment->getHead()->getTerminalSymbols(name_words, 20);
					for(k = 0; k< nwords; k++){
						if(ment_hw == name_words[k]){
							found_match = true;
							match_count++;
							match_entity_id = oth_ent->getID();
						}
					}
				}
			}
		}
		if(match_count != 1){
			return 0;
		}
		if(match_entity_id != o->getEntity()->getID()){
			return 0;
		}
		for(i =0; i< o->getEntity()->getNMentions(); i++){
			Mention* ent_ment = o->getEntitySet()->getMention(o->getEntity()->getMention(i));
			if(ent_ment->getMentionType() == Mention::NAME){
				int nwords = ent_ment->getHead()->getTerminalSymbols(name_words, 20);
				for(j = 0; j< nwords; j++){
					if(ment_hw == name_words[j]){
						//found a match
						resultArray[0] = _new DTBigramFeature(this, state.getTag(),
							ment_hw );
						resultArray[1] = _new DTBigramFeature(this, state.getTag(),
							o->getMention()->getEntityType().getName() );
						resultArray[2] = _new DTBigramFeature(this, state.getTag(),
							Symbol(L"-BACKOFF-") );
						return 3;
					}
				}
			}
		}
		return 0;
	}

};
#endif
