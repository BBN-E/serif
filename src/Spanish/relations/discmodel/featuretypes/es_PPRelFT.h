// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_PP_REL_FT_H
#define es_PP_REL_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DT6gramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Spanish/relations/es_RelationUtilities.h"

class SpanishPPRelFT : public P1RelationFeatureType {
public:
	SpanishPPRelFT() : P1RelationFeatureType(Symbol(L"pp-relation")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DT6gramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		if (o->hasPPRel()) {
			Symbol enttype1 = o->getMention1()->getEntityType().getName();
			Symbol enttype2 = o->getMention2()->getEntityType().getName();

			if (o->getVerbinPPRel() != Symbol(L"NULL")) {
				// option 1
				/*
				resultArray[0] = _new DT6gramFeature(this, state.getTag(),
					enttype1, enttype2, Symbol(L"verbPPRel"), o->getPrepinPPRel(), o->getVerbinPPRel());
				*/

				// option 2 -- use stemmed word
				resultArray[0] = _new DT6gramFeature(this, state.getTag(),
					enttype1, enttype2, Symbol(L"verbPPRel"), o->getPrepinPPRel(), o->getStemmedVerbinPPRel());


				return 1;
			}else{
				// option 1
				//Symbol head1 = o->getMention1()->getNode()->getHeadWord();
				
				// option 2 -- use stemmed word
				Symbol head1 = o->getStemmedHeadofMent1();

				resultArray[0] = _new DT6gramFeature(this, state.getTag(),
					enttype1, enttype2, Symbol(L"nounPPRel"), o->getPrepinPPRel(), head1);
				resultArray[1] = _new DT6gramFeature(this, state.getTag(),
					enttype1, enttype2, Symbol(L"nounPPRel"), o->getPrepinPPRel(), enttype1);
				return 2;
			}
			
		}else {
			return 0;
		}
	}

};

#endif
