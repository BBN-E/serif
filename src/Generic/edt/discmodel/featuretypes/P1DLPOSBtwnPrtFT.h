// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P1DL_POS_BTWN_PRT_FT
#define P1DL_POS_BTWN_PRT_FT

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/EntitySubtype.h"
#include "Generic/theories/Mention.h"



class P1DLPOSBtwnPrtFT : public DTCorefFeatureType {
public:
	P1DLPOSBtwnPrtFT() : DTCorefFeatureType(Symbol(L"p1dl-posbtwn-part")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));


		//EntitySet* ents = o->getEntitySet();
		Entity* ent = o->getEntity();
		int nresults = 0;
		int n_same_sent =0;
		int n_adjacent = 0;
		const Mention* linkment = o->getMention();
		//std::cout<<"postbtwn extractor"<<std::endl;
		for(int j = 0; j <ent->getNMentions(); j++){
			EntityType othtype = o->getEntitySet()->getMention(ent->getMention(j))->getEntityType();
			if(linkment->getEntityType() != othtype){
				return 0;
			}
		}
		for (int i = 0; i < ent->getNMentions(); i++) {	
			Mention* ment = o->getEntitySet()->getMention(
				ent->getMention(i));
			Symbol menttype;
			if(ment->getMentionType() == Mention::NAME){
				menttype = Symbol(L"NAME");
			} 
			else if(ment->getMentionType() == Mention::DESC){
				menttype = Symbol(L"DESC");
			}
			else{
				continue;
			}
			if(ment->getSentenceNumber() == linkment->getSentenceNumber()){
		
				n_same_sent++;
				Symbol result_sym = DescLinkFeatureFunctions::getPatternBetween(
					o->getMentionSet(),
					linkment, ment);
				if((result_sym.is_null()) || (menttype.is_null()) ){
				}
				else if (nresults >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION){
					SessionLogger::warn("DT_feature_limit") 
						<<"P1DLPOSBtwnPrtFT discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
					break;
				}else{
					resultArray[nresults++] = _new DTTrigramFeature(this, state.getTag(), result_sym, 
							ment->getHead()->getHeadWord());
				}
				
			}
		}
		
		return nresults;
	}
		



};
#endif
