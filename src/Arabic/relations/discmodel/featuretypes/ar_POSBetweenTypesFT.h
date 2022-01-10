// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_POS_BETWEEN_TYPES_FT_H
#define AR_POS_BETWEEN_TYPES_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DT6gramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"


class ArabicPOSBetweenTypesFT : public P1RelationFeatureType {
public:
	ArabicPOSBetweenTypesFT() : P1RelationFeatureType(Symbol(L"pos-entity-types")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DT6gramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		//std::cerr << "Entered ArabicPOSBetweenTypesFT::extractFeatures()\n";
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
		
		const SynNode *head1 = o->getMention1()->getNode()->getHeadPreterm()->getChild(0);
		const SynNode *head2 = o->getMention2()->getNode()->getHeadPreterm()->getChild(0);
	
		int tok1 = head1->getStartToken();
		int tok2 = head2->getStartToken();

		if (tok2 - tok1 - 1 > 5) {
			resultArray[0] = _new DT6gramFeature(this, state.getTag(),
								  o->getMention1()->getEntityType().getName(), mtype1,
								  RelationObservation::TOO_LONG,
								  o->getMention2()->getEntityType().getName(), mtype2);
		}
		else if (tok2 - tok1 - 1 == 0) {
			resultArray[0] = _new DT6gramFeature(this, state.getTag(),
								  o->getMention1()->getEntityType().getName(), mtype1,
								  RelationObservation::ADJACENT,
								  o->getMention2()->getEntityType().getName(), mtype2);
		}
		else if (tok2 - tok1 - 1 < 0) {
			resultArray[0] = _new DT6gramFeature(this, state.getTag(),
								  o->getMention1()->getEntityType().getName(), mtype1,
								  RelationObservation::CONFUSED,
								  o->getMention2()->getEntityType().getName(), mtype2);
		}
		else {
			std::wstring str = L"";
			const SynNode *node = head1->getNextTerminal();
			while (node != head2 && node != 0) {
				if (wcscmp(str.c_str(), L""))
					str += L"-";
				const SynNode *preterm = node->getParent();
				if (preterm == 0) {
					std::cerr << "Parent is null\n";
					std::cerr.flush();
				}
				Symbol tag = preterm->getTag();
				str += preterm->getTag().to_string();
				node = node->getNextTerminal();
			}

			resultArray[0] = _new DT6gramFeature(this, state.getTag(),
									o->getMention1()->getEntityType().getName(), mtype1,
									Symbol(str.c_str()),
									o->getMention2()->getEntityType().getName(), mtype2);
		}
		return 1;
	}

};

#endif
