// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HW_MATCH_GPEMODMATCH_FT_H
#define HW_MATCH_GPEMODMATCH_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"


class HWMatchGPEModMatchFT : public DTCorefFeatureType {
public:
	HWMatchGPEModMatchFT() : DTCorefFeatureType(Symbol(L"p1desclinker-hw-match-gpe-mod-match")) {
	}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		
		Symbol results[5];
		const Mention* thismention = o->getMention();
		const MentionSet* thismentionset = o->getMentionSet();
		Entity* thisentity = o->getEntity();
		const EntitySet* thisentityset = o->getEntitySet();
		bool  val  = DescLinkFeatureFunctions::testHeadWordMatch( thismention, thisentity, 
			thisentityset );
		if(!val)
			return 0;
		std::vector<const Mention*> mention_gpe_names = DescLinkFeatureFunctions::getGPEModNames(thismention);
		if (mention_gpe_names.empty()) {
			return 0;
		}



		//get Entity Names
		
		int nment_in_ent = thisentity->getNMentions();
		Symbol name_buffer_1[10];
		Symbol name_buffer_2[10];
		for(int i=0; i<nment_in_ent; i++){
			Mention* m = thisentityset->getMention(thisentity->getMention(i));
			std::vector<const Mention*> entity_gpe_names = DescLinkFeatureFunctions::getModNames(m);
			for(size_t j=0; j< entity_gpe_names.size(); j++){
				//get gpe_name
				int n1 = entity_gpe_names[j]->getNode()->getTerminalSymbols(name_buffer_1,10);
				for(size_t k=0; k<mention_gpe_names.size(); k++){
					int n2 = mention_gpe_names[k]->getNode()->getTerminalSymbols(name_buffer_2, 10);
					if(n1 == n2){
						bool allmatch = true;
						for(int p = 0; p< n1; p++){
							if(name_buffer_1[p] != name_buffer_2[p]){
								allmatch = false;
								break;
							}
						}
						if(allmatch){
							resultArray[0] = _new DTMonogramFeature(this, state.getTag());
							return 1;
						}
					}
				}
			}
		}
		return 0;
	}

};
#endif
