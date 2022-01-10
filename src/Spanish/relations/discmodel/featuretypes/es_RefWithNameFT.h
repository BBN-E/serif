// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_REF_WITH_NAME_FT_H
#define es_REF_WITH_NAME_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Spanish/relations/es_RelationUtilities.h"
#include "Generic/common/SymbolUtilities.h"
#include <sstream>

static Symbol NONEsym = Symbol(L"NONE");
static Symbol OIsym = Symbol(L"OTHER-AFF.Ideology");
static Symbol OEsym = Symbol(L"OTHER-AFF.Ethnic");
static Symbol OOsym = Symbol(L"OTHER-AFF.Other");

class SpanishRefWithNameFT : public P1RelationFeatureType {
public:
	SpanishRefWithNameFT() : P1RelationFeatureType(Symbol(L"ref-with-name")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		if (state.getTag() != NONEsym &&
			state.getTag() != OIsym &&
			state.getTag() != OEsym &&
			state.getTag() != OOsym)
		{
			return 0;
		}

		RelationPropLink *link = o->getPropLink();
		if (!link->isEmpty() && !link->isNested()) {
			const Mention *nameMention = 0;
			EntityType refEntityType = EntityType::getOtherType();
			if (link->getArg1Role() == Argument::REF_ROLE &&
				link->getArg2Role() == Argument::UNKNOWN_ROLE &&
				o->getMention2()->getMentionType() == Mention::NAME) 
			{
				nameMention = o->getMention2();
				refEntityType = o->getMention1()->getEntityType();
			} else if (link->getArg2Role() == Argument::REF_ROLE &&
				link->getArg1Role() == Argument::UNKNOWN_ROLE &&
				o->getMention1()->getMentionType() == Mention::NAME) 
			{
				nameMention = o->getMention1();
				refEntityType = o->getMention2()->getEntityType();

			} else return 0;

			while (nameMention->getChild() != 0)
				nameMention = nameMention->getChild();
			const SynNode *nameNode = nameMention->getNode();

			std::wostringstream nameStr;
			for (int i = 0; i < nameNode->getNChildren(); i++) {
				Symbol word = nameNode->getChild(i)->getHeadWord();
				nameStr << word.to_string();
				if (i < nameNode->getNChildren() - 1)
					nameStr << L"_";
			}
			
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(),
				refEntityType.getName(), Symbol(nameStr.str().c_str()));
			return 1;
			
		} else return 0;
	}

};

#endif
