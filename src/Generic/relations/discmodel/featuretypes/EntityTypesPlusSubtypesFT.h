// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENTITY_TYPES_PLUS_SUBTYPES_FT_H
#define ENTITY_TYPES_PLUS_SUBTYPES_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/common/version.h"


class EntityTypesPlusSubtypesFT : public P1RelationFeatureType {
public:
	EntityTypesPlusSubtypesFT() : P1RelationFeatureType(Symbol(L"entity-types-plus-subtypes")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(state.getObservation(0));
		if (! SerifVersion::isArabic()) {
			//#ifndef ARABIC_LANGUAGE 
			//Arabic doesn't have propositions, but this feature is still useful, 
			//so ignore prop link requirement

			// we only fire this feature if there is a proplink
			if (o->getPropLink()->isEmpty())
				return 0;
		}
		//#endif
		// I'm putting this English-specific because I'm not sure if it will
		// screw everybody else up
		if (SerifVersion::isEnglish()) {
			//#ifdef ENGLISH_LANGUAGE
			if (!o->getMention1()->getEntitySubtype().isDetermined() ||
				!o->getMention2()->getEntitySubtype().isDetermined())
			{
				return 0;
			}
			//#endif
		}

		Symbol stype1 = o->getMention1()->getEntitySubtype().getName();
		Symbol stype2 = o->getMention2()->getEntitySubtype().getName();
		
		resultArray[0] = _new DTQuintgramFeature(this, state.getTag(),
								  o->getMention1()->getEntityType().getName(), stype1,
								  o->getMention2()->getEntityType().getName(), stype2);
		return 1;
	}

};

#endif
