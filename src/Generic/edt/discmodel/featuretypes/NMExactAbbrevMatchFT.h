// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NMEXACT_ABBREV_FT_H
#define NMEXACT_ABBREV_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discTagger/DTTrigramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"

#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntityType.h"


/* test whether the head of the mention once abrreviated exactly match the entity head words
 * once abbreviated.
 */
class NMExactAbbrevMatchFT : public DTCorefFeatureType {
public:
	NMExactAbbrevMatchFT() : DTCorefFeatureType(Symbol(L"name-exact-abbrev-match")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramIntFeature(this, SymbolConstants::nullSymbol
			, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		const MentionSymArrayMap *mapper = o->getAbbrevMentionMapper();
		SerifAssert (mapper!=0);
		EntityType mentEntityType = o->getMention()->getEntityType();
		Entity *entity = o->getEntity();

		Symbol mentEntTypeSym;
		if(!mentEntityType.isDetermined()) // this is required
			mentEntTypeSym = NO_ENTITY_TYPE;
		else
			mentEntTypeSym = mentEntityType.getName();
		
		Symbol entityMentionLevel = o->getEntityMentionLevel();

		int n_match = DescLinkFeatureFunctions::testHeadWordsMatch(o->getMentionUID()
			, entity, mapper);
		if(n_match != -1){
			resultArray[0] = _new DTTrigramIntFeature(this, state.getTag(), mentEntTypeSym, entityMentionLevel, n_match);
			return 1;
		}
		else
			return 0;
	}

};
#endif
