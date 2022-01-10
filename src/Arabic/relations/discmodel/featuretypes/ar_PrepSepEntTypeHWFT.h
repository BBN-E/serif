// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_PREPSEP_ENTTYPE_HW_FT_H
#define AR_PREPSEP_ENTTYPE_HW_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"


class ArabicPrepSepEntTypeHWFT : public P1RelationFeatureType {
public:
	ArabicPrepSepEntTypeHWFT() : P1RelationFeatureType(Symbol(L"prep-sep-enttype-hw")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DT6gramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol , SymbolConstants::nullSymbol, 
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		//std::cerr << "Entered POSBetweenTypesFT::extractFeatures()\n";
		//std::cerr.flush();
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		Symbol mtype1 = Symbol(L"CONFUSED");
		if (o->getMention1()->getMentionType() == Mention::NAME)
			mtype1 = Symbol(L"NAME");
		else if (o->getMention1()->getMentionType() == Mention::DESC)
			mtype1 = Symbol(L"DESC");
		else if (o->getMention1()->getMentionType() == Mention::PRON)
			mtype1 = Symbol(L"PRON");

		Symbol mtype2 = Symbol(L"CONFUSED");
		if (o->getMention2()->getMentionType() == Mention::NAME)
			mtype2 = Symbol(L"NAME");
		else if (o->getMention2()->getMentionType() == Mention::DESC)
			mtype2 = Symbol(L"DESC");
		else if (o->getMention2()->getMentionType() == Mention::PRON)
			mtype2 = Symbol(L"PRON");
		const Mention* m1 = o->getMention1();
		const Mention* m2 = o->getMention2();
	
		int dist = RelationUtilities::get()->calcMentionDist(m1, m2);
	
		if (dist == 2) {
			//get the 'btwn' tok #
			int start_ment1 = RelationUtilities::get()->getMentionStartToken(m1);
			int start_ment2 = RelationUtilities::get()->getMentionStartToken(m2);
			int end_ment1 = RelationUtilities::get()->getMentionEndToken(m1);
			int end_ment2 = RelationUtilities::get()->getMentionEndToken(m2);
			if((start_ment1 < 0) || (end_ment1 < 0) || (start_ment2 < 0) || (end_ment2 < 0) ){
				return 0;
			}
			int tok_btwn = -1;
			if(start_ment2 > end_ment1){
				tok_btwn = end_ment1+1;
			}
			else if(start_ment1 >end_ment2){
				tok_btwn = end_ment2 + 1;
			}
			else{
				return 0;
			}
			int nresults =0;
			if(LanguageSpecificFunctions::isPrepPOSLabel(o->getPOS(tok_btwn))){
                const SynNode* preterm = o->getMention1()->getNode()->getHeadPreterm();

				resultArray[0] = _new DT6gramFeature(this, state.getTag(),
								  o->getMention1()->getEntityType().getName(), mtype1,
								  o->getMention2()->getNode()->getHeadWord(), mtype2, 
								  o->getToken(tok_btwn));
				resultArray[1] = _new DT6gramFeature(this, state.getTag(),
								  o->getMention1()->getNode()->getHeadWord(), mtype1,
								  o->getMention2()->getEntityType().getName(),  mtype2, 
								  o->getToken(tok_btwn));
				resultArray[2] = _new DT6gramFeature(this, state.getTag(),
								  o->getMention1()->getEntityType().getName(), mtype1,
								  o->getMention2()->getEntityType().getName(),  mtype2, 
								  o->getToken(tok_btwn));
				nresults = 3;
				return nresults;
			}
		}
		return 0;
		
	}

};

#endif
