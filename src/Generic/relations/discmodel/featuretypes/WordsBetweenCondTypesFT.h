// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef WORDS_BETWEEN_COND_TYPES_FT_H
#define WORDS_BETWEEN_COND_TYPES_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuintgramStringFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"


class WordsBetweenCondTypesFT : public P1RelationFeatureType {
public:
	WordsBetweenCondTypesFT() : P1RelationFeatureType(Symbol(L"wb-cond-entity-types")) {}

	virtual DTFeature *makeEmptyFeature() const {
		wstring dummy(L"");
		return _new DTQuintgramStringFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, dummy,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		if (o->isTooLong())
			return 0;

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

		Symbol ent1 = o->getMention1()->getEntityType().getName();
		if (o->getMention1()->getEntityType().matchesFAC() ||
			o->getMention1()->getEntityType().matchesGPE() ||
			o->getMention1()->getEntityType().matchesLOC()) 
		{
			ent1 = Symbol(L"GFL");
		}

		Symbol ent2 = o->getMention2()->getEntityType().getName();
		if (o->getMention2()->getEntityType().matchesFAC() ||
			o->getMention2()->getEntityType().matchesGPE() ||
			o->getMention2()->getEntityType().matchesLOC()) 
		{
			ent2 = Symbol(L"GFL");
		}
		
		resultArray[0] = _new DTQuintgramStringFeature(this, state.getTag(),
								  ent1, mtype1,
								  o->getWordsBetween(),
								  ent2, mtype2);
		return 1;
	}

};

#endif
