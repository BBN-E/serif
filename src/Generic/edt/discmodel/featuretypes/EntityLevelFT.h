// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENTLEVEL_FT_H
#define ENTLEVEL_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"
#include "Generic/theories/EntitySet.h"

// Entity Level: NONE < NAME < DESC < PRON
/* from Mention.h 
	typedef enum {NONE = 0,
				  NAME,
				  PRON,
				  DESC,
				  PART,
				  APPO,
				  LIST,
				  INFL} Type;
*/
class EntityLevelFT : public DTCorefFeatureType {
public:
	EntityLevelFT() : DTCorefFeatureType(Symbol(L"ent-level")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		Entity *entity = o->getEntity();
		const EntitySet *entitySet = o->getEntitySet();
		size_t n_ments = entity->getNMentions();
		Mention::Type mentMentionType, entMentionLevel;
		entMentionLevel = entitySet->getMention(entity->getMention(0))->getMentionType();
		if (entMentionLevel == Mention::NAME) {
			resultArray[0] = _new DTIntFeature(this, state.getTag(), entMentionLevel);
			return 1;
		}// else
		for (size_t i=1; i<n_ments; i++) {
			mentMentionType = entitySet->getMention(entity->getMention(i))->getMentionType();
			if (mentMentionType == Mention::NAME) {
				resultArray[0] = _new DTIntFeature(this, state.getTag(), mentMentionType);
				return 1;
			}// else
			if (mentMentionType == Mention::DESC || mentMentionType == Mention::APPO) {
				entMentionLevel = mentMentionType;
			} else if (mentMentionType>entMentionLevel && entMentionLevel != Mention::DESC && entMentionLevel != Mention::APPO) {
				entMentionLevel = mentMentionType;
			}
		}// for

//		resultArray[0] = _new DTIntFeature(this, state.getTag(), entMentionLevel);
		resultArray[0] = _new DTIntFeature(this, state.getTag(), Mention::NONE);
		return 1;
	}

};
#endif
