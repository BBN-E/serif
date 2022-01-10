// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_ENTITY_AND_MENTION_TYPES_FT_H
#define AR_ENTITY_AND_MENTION_TYPES_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/theories/MentionSet.h"
#include "Arabic/relations/ar_RelationUtilities.h"



class ArabicEntityAndMentionTypesFT : public P1RelationFeatureType {
public:
	ArabicEntityAndMentionTypesFT() : P1RelationFeatureType(Symbol(L"entity-and-mention-types")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
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
		
		resultArray[0] = _new DTQuintgramFeature(this, state.getTag(),
								  o->getMention1()->getEntityType().getName(), mtype1,
								  o->getMention2()->getEntityType().getName(), mtype2);
		if(RelationUtilities::get()->distIsLessThanCutoff(o->getMention1(), o->getMention2())){
			wchar_t buffer[300];
			wcscpy(buffer, o->getMention1()->getEntityType().getName().to_string());
			wcscat(buffer, L"-CLOSE");
			Symbol s1 =Symbol(buffer);
			wcscpy(buffer, o->getMention2()->getEntityType().getName().to_string());
			wcscat(buffer, L"-CLOSE");
			Symbol s2 =Symbol(buffer);
			resultArray[1] = _new DTQuintgramFeature(this, state.getTag(), s1, mtype1, s2, mtype2);
			return 2;
		}
		else{
			wchar_t buffer[300];
			wcscpy(buffer, o->getMention1()->getEntityType().getName().to_string());
			wcscat(buffer, L"-FAR");
			Symbol s1 =Symbol(buffer);
			wcscpy(buffer, o->getMention2()->getEntityType().getName().to_string());
			wcscat(buffer, L"-FAR");
			Symbol s2 =Symbol(buffer);
			resultArray[1] = _new DTQuintgramFeature(this, state.getTag(), s1, mtype1, s2, mtype2);
			return 2;
		}
		

		return 1;
	}

};

#endif
