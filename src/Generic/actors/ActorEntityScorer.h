// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ACTOR_ENTITY_SCORER_H
#define ACTOR_ENTITY_SCORER_H

#include "Generic/theories/ActorEntity.h"
#include "Generic/theories/ActorEntitySet.h"
#include "Generic/actors/ActorInfo.h"

#include <boost/shared_ptr.hpp>

class ActorEntityScorer {

public:
	ActorEntityScorer();
	~ActorEntityScorer();

	// Take the ActorMentions and turn them into a set of scored ActorEntities
	ActorEntitySet* createActorEntitySet(DocTheory *docTheory, std::vector<ProperNounActorMention_ptr> &allActorMentions, ProperNounActorMention_ptr documentCountryActorMention);

	double getLowEditDistanceThreshold(EntityType entityType);
	double getMediumEditDistanceThreshold(EntityType entityType);
	double getHighEditDistanceThreshold(EntityType entityType);
	double getTSTEditDistanceEquivalent(EntityType entityType);
	
private:

	double _low_confidence;
	double _medium_confidence;

	double _per_edit_distance_low;
	double _per_edit_distance_medium;
	double _per_edit_distance_high;

	double _org_edit_distance_low;
	double _org_edit_distance_medium;
	double _org_edit_distance_high;

	double _per_tst_edit_distance_equivalent;
	double _org_tst_edit_distance_equivalent;

	static const int LOW = 1;
	static const int MEDIUM = 2;
	static const int HIGH = 3;

	bool hasHighQualityPatternMatch(ActorEntity_ptr actorEntity, ProperNounActorMention_ptr defaultCountryActorMention);
	bool hasHighQualityGeoresolutionMatch(ActorEntity_ptr actorEntity);
	int editDistanceQuality(ActorEntity_ptr actorEntity);
	ActorEntity_ptr getBestMatch(std::map<ActorId, ActorEntity_ptr> actorEntities);
	ActorEntity_ptr getImportantMatch(std::map<ActorId, ActorEntity_ptr> actorEntities);
	bool countryMatch(ProperNounActorMention_ptr countryActorMention, ProperNounActorMention_ptr actorMention);

	ActorInfo_ptr _actorInfo;
};

typedef boost::shared_ptr<ActorEntityScorer> ActorEntityScorer_ptr;

#endif
