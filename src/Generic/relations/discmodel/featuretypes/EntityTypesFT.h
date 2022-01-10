// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENTITY_TYPES_FT_H
#define ENTITY_TYPES_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/Mention.h"

#include "Generic/relations/xx_RelationUtilities.h"
#include <map>


class EntityTypesFT : public P1RelationFeatureType {
private:
	// These two maps are used to cache the mapping from entity type name 
	// (such as GPE) to feature symbols (such as GPE-CLOSE or GPE-FAR).
	// Since this is just a cache, we declare them mutable (which allows
	// us to modify them in a const method).
	typedef Symbol::HashMap<Symbol> SymSymMap;
	mutable SymSymMap _closeEntityTypeSymbols;
	mutable SymSymMap _farEntityTypeSymbols;
	Symbol closeEntityTypeSymbol(const Symbol &entityTypeName) const {
		Symbol& result = _closeEntityTypeSymbols[entityTypeName];
		if (result.is_null())
			result = Symbol(entityTypeName.to_string()+std::wstring(L"-CLOSE"));
		return result;
	}
	Symbol farEntityTypeSymbol(const Symbol &entityTypeName) const {
		Symbol& result = _farEntityTypeSymbols[entityTypeName];
		if (result.is_null())
			result = Symbol(entityTypeName.to_string()+std::wstring(L"-FAR"));
		return result;
	}
public:
	EntityTypesFT() : P1RelationFeatureType(Symbol(L"entity-types")) {
	}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));
		const Mention *m1 = o->getMention1();
		const Mention *m2 = o->getMention2();
		const Symbol& m1EntityTypeName = m1->getEntityType().getName();
		const Symbol& m2EntityTypeName = m2->getEntityType().getName();

		resultArray[0] = _new DTTrigramFeature(this, state.getTag(), m1EntityTypeName,
						       m2EntityTypeName);
		if(RelationUtilities::get()->distIsLessThanCutoff(m1, m2)){
			Symbol s1 = closeEntityTypeSymbol(m1EntityTypeName);
			Symbol s2 = closeEntityTypeSymbol(m2EntityTypeName);
			resultArray[1] = _new DTTrigramFeature(this, state.getTag(), s1, s2);
			return 2;
		}
		else {
			Symbol s1 = farEntityTypeSymbol(m1EntityTypeName);
			Symbol s2 = farEntityTypeSymbol(m2EntityTypeName);
			resultArray[1] = _new DTTrigramFeature(this, state.getTag(), s1, s2);
			return 2;
		}

		return 1;
	}

};

#endif
