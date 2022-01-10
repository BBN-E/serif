// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_PREP_STACK_FT_H
#define es_PREP_STACK_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Spanish/relations/es_RelationUtilities.h"

class SpanishPrepStackFT : public P1RelationFeatureType {
public:
	SpanishPrepStackFT() : P1RelationFeatureType(Symbol(L"prep-stack")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		RelationPropLink *link = o->getPropLink();
		const MentionSet* mentSet = o->getMentionSet();
		//we're in a bad place, our relation arguments aren't mentions!
		if((link->getArg1Ment(mentSet) == 0) || 
			(link->getArg2Ment(mentSet) == 0))
		{
				return 0;
		}

		if (!link->isEmpty() && !link->isNested()) {
			Symbol role2 = link->getArg2Role();

			if (role2 == Argument::IOBJ_ROLE ||
				role2 == Argument::LOC_ROLE ||
				role2 == Argument::MEMBER_ROLE ||
				role2 == Argument::OBJ_ROLE ||
				role2 == Argument::POSS_ROLE ||
				role2 == Argument::REF_ROLE ||
				role2 == Argument::SUB_ROLE ||
				role2 == Argument::TEMP_ROLE ||
				role2 == Argument::UNKNOWN_ROLE)
				return 0;
			
			Proposition *prop  = link->getTopProposition();
			if(prop->getArg(0)->getType() != Argument::MENTION_ARG){
				//std::cout<<"non mention arg 0 prepStackFT"<<std::endl;
				return 0;
			}
			if (prop->getNArgs() >= 2 &&
				prop->getArg(0)->getMention(mentSet) == link->getArg1Ment(mentSet))
			{
				for (int i = 1; i < prop->getNArgs(); i++) {
					if (prop->getArg(i)->getType() != Argument::MENTION_ARG)
						continue;
					if (prop->getArg(i)->getMention(mentSet) == link->getArg2Ment(mentSet))
						return 0;
					Symbol otherRole = prop->getArg(i)->getRoleSym();
					if (otherRole == Argument::IOBJ_ROLE ||
						otherRole == Argument::LOC_ROLE ||
						otherRole == Argument::MEMBER_ROLE ||
						otherRole == Argument::OBJ_ROLE ||
						otherRole == Argument::POSS_ROLE ||
						otherRole == Argument::REF_ROLE ||
						otherRole == Argument::SUB_ROLE ||
						otherRole == Argument::TEMP_ROLE ||
						otherRole == Argument::UNKNOWN_ROLE)
						continue;
					if (prop->getArg(i)->getMention(o->getMentionSet())->getNode()->getStartToken() <
						prop->getArg(0)->getMention(o->getMentionSet())->getNode()->getStartToken())
						continue;
                    
					resultArray[0] = _new DTMonogramFeature(this, state.getTag());
					return 1;
				}
			}
			return 0;
		} else return 0;
	}

};

#endif
