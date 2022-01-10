// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/actors/ActorEntityScorer.h"
#include "Generic/actors/AWAKEActorInfo.h"
#include "Generic/theories/ActorMentionSet.h"
#include "Generic/common/ParamReader.h"

#include <boost/foreach.hpp>

ActorEntityScorer::ActorEntityScorer() { 
	_low_confidence = ParamReader::getOptionalFloatParamWithDefaultValue("actor_entity_low_confidence", 0.1);
	_medium_confidence = ParamReader::getOptionalFloatParamWithDefaultValue("actor_entity_medium_confidence", 0.3);
	// High scores are reserved -- generally 0.9, 1.0

	_per_edit_distance_low = ParamReader::getOptionalFloatParamWithDefaultValue("per_edit_distance_low", 0.825);
	_per_edit_distance_medium = ParamReader::getOptionalFloatParamWithDefaultValue("per_edit_distance_medium", 0.875);
	_per_edit_distance_high = ParamReader::getOptionalFloatParamWithDefaultValue("per_edit_distance_high", 0.9);

	_org_edit_distance_low = ParamReader::getOptionalFloatParamWithDefaultValue("org_edit_distance_low", 0.825);
	_org_edit_distance_medium = ParamReader::getOptionalFloatParamWithDefaultValue("org_edit_distance_medium", 0.875);
	_org_edit_distance_high = ParamReader::getOptionalFloatParamWithDefaultValue("org_edit_distance_high", 0.9);

	_per_tst_edit_distance_equivalent = ParamReader::getOptionalFloatParamWithDefaultValue("per_tst_edit_distance_equivalent", _per_edit_distance_medium);
	_org_tst_edit_distance_equivalent = ParamReader::getOptionalFloatParamWithDefaultValue("org_tst_edit_distance_equivalent", _org_edit_distance_low);

	_actorInfo = AWAKEActorInfo::getAWAKEActorInfo();
}

ActorEntityScorer::~ActorEntityScorer() { }

ActorEntitySet* ActorEntityScorer::createActorEntitySet(DocTheory *docTheory, std::vector<ProperNounActorMention_ptr> &allActorMentions, ProperNounActorMention_ptr documentCountryActorMention) {
	// Iterate over entities, assigning at least one ActorEntity to each one
	// that has a name mention or a high quality actor match

	ActorEntitySet *aes = _new ActorEntitySet();

	EntitySet *es = docTheory->getEntitySet();
	for (int i = 0; i < es->getEntities().length(); i++) {
		const Entity *entity = es->getEntity(i);

		std::vector<ProperNounActorMention_ptr> entityActorMentions;
		for (int j = 0; j < entity->getNMentions(); j++) {
			MentionUID mention_uid = entity->getMention(j);
			Mention *mention = es->getMention(mention_uid);
			ActorMentionSet *ams = docTheory->getSentenceTheory(mention->getSentenceNumber())->getActorMentionSet();
			std::vector<ActorMention_ptr> actorMentions = ams->findAll(mention_uid);
			BOOST_FOREACH(ActorMention_ptr am, actorMentions) {
				ProperNounActorMention_ptr pnam = boost::dynamic_pointer_cast<ProperNounActorMention>(am);
				assert(pnam);
				entityActorMentions.push_back(pnam);
			}
		}

		// Create ActorEntity for each unique actor represented in entityActorMentions
		std::vector<ActorEntity_ptr> actorEntities;
		std::map<ActorId, ActorEntity_ptr> actorMap;
		BOOST_FOREACH(ProperNounActorMention_ptr pnam, entityActorMentions) {
			ActorId aid = pnam->getActorId();
			if (actorMap.find(aid) == actorMap.end()) {
				ActorEntity_ptr actorEntity = boost::make_shared<ActorEntity>(entity, Symbol(), ActorEntity::ActorIdentifiers(aid, _actorInfo->getActorName(aid)));
				actorEntity->addActorMention(pnam);
				actorMap[aid] = actorEntity;
				actorEntities.push_back(actorEntity);
			} else {
				actorMap[aid]->addActorMention(pnam);
			}
		}

		bool need_unmatched_actor = true;
		std::map<ActorId, ActorEntity_ptr> highQualityMatches;
		std::map<ActorId, ActorEntity_ptr> mediumQualityMatches;
		std::map<ActorId, ActorEntity_ptr> lowQualityMatches;

		// High quality pattern matches and georesultion matches
		BOOST_FOREACH(ActorEntity_ptr actorEntity, actorEntities) {
			if (hasHighQualityPatternMatch(actorEntity, documentCountryActorMention) || hasHighQualityGeoresolutionMatch(actorEntity)) {
				need_unmatched_actor = false;
				highQualityMatches[actorEntity->getActorId()] = actorEntity;
			}
		}

		// High quality edit-distance/token-subset-trees matches
		if (highQualityMatches.size() == 0) {
			BOOST_FOREACH(ActorEntity_ptr actorEntity, actorEntities) {
				if (editDistanceQuality(actorEntity) == HIGH) 
					highQualityMatches[actorEntity->getActorId()] = actorEntity;
			}
		}

		// Medium quality edit-distance/token-subset-trees matches
		BOOST_FOREACH(ActorEntity_ptr actorEntity, actorEntities) {
			if (highQualityMatches.find(actorEntity->getActorId()) == highQualityMatches.end() &&
				(editDistanceQuality(actorEntity) == MEDIUM || editDistanceQuality(actorEntity) == HIGH))
			{
				mediumQualityMatches[actorEntity->getActorId()] = actorEntity;
			}
		}

		// Everything else goes in lowQualityMatches
		BOOST_FOREACH(ActorEntity_ptr actorEntity, actorEntities) {
			if (highQualityMatches.find(actorEntity->getActorId()) == highQualityMatches.end() &&
				mediumQualityMatches.find(actorEntity->getActorId()) == mediumQualityMatches.end()) 
			{
				lowQualityMatches[actorEntity->getActorId()] = actorEntity;
			}
		}

		if (!entity->hasNameMention(es) && highQualityMatches.size() == 0)
			continue;

		// First check to see if we have a importance score based match
		// If we do, then just set that one to 0.9 and everything 
		// thing else to _low_confidence
		ActorEntity_ptr importantMatch = getImportantMatch(highQualityMatches);
		if (importantMatch) {
			if (highQualityMatches.size() + mediumQualityMatches.size() + lowQualityMatches.size() == 1) {
				importantMatch->setConfidence(1.0);
				aes->addActorEntity(importantMatch);
				continue;
			}
			importantMatch->setConfidence(0.9);
			aes->addActorEntity(importantMatch);
			std::map<ActorId, ActorEntity_ptr>::iterator iter;
			for (iter = highQualityMatches.begin(); iter != highQualityMatches.end(); iter++) {
				ActorEntity_ptr actorEntity = iter->second;
				if (actorEntity == importantMatch)
					continue;
				actorEntity->setConfidence(_low_confidence);
				aes->addActorEntity(actorEntity);
			}
			for (iter = mediumQualityMatches.begin(); iter != mediumQualityMatches.end(); iter++) {
				ActorEntity_ptr actorEntity = iter->second;
				actorEntity->setConfidence(_low_confidence);
				aes->addActorEntity(actorEntity);
			}
			for (iter = lowQualityMatches.begin(); iter != lowQualityMatches.end(); iter++) {
				ActorEntity_ptr actorEntity = iter->second;
				actorEntity->setConfidence(_low_confidence);
				aes->addActorEntity(actorEntity);
			}
			
			continue;
		}

		// Set high quality match scores
		double high_quality_score = 1.0;
		if (need_unmatched_actor || mediumQualityMatches.size() > 0 || lowQualityMatches.size() > 0)
			high_quality_score = 0.9;
		if (highQualityMatches.size() > 1)
			high_quality_score = 0.5;

		std::map<ActorId, ActorEntity_ptr>::iterator iter;
		for (iter = highQualityMatches.begin(); iter != highQualityMatches.end(); iter++) {
			ActorEntity_ptr actorEntity = iter->second;
			actorEntity->setConfidence(high_quality_score);
			aes->addActorEntity(actorEntity);
		}
		if (highQualityMatches.size() > 1) {
			ActorEntity_ptr best = getBestMatch(highQualityMatches);
			if (best && best->getConfidence() < 0.9)
				best->setConfidence(best->getConfidence() + 0.1);
		}

		// Set medium quality match scores
		for (iter = mediumQualityMatches.begin(); iter != mediumQualityMatches.end(); iter++) {
			ActorEntity_ptr actorEntity = iter->second;
			actorEntity->setConfidence(_medium_confidence);
			aes->addActorEntity(actorEntity);
		}

		// Set low quality match scores
		for (iter = lowQualityMatches.begin(); iter != lowQualityMatches.end(); iter++) {
			ActorEntity_ptr actorEntity = iter->second;
			actorEntity->setConfidence(_low_confidence);
			aes->addActorEntity(actorEntity);
		}
		
		// Unmatched actor
		if (need_unmatched_actor) {
			ActorEntity_ptr actorEntity = boost::make_shared<ActorEntity>(entity, Symbol());
			double unmatched_score = 1.0;
			if (lowQualityMatches.size() > 0 || mediumQualityMatches.size() > 0)
				unmatched_score = 0.9;
			if (highQualityMatches.size() > 0)
				unmatched_score = 0.1;

			actorEntity->setConfidence(unmatched_score);
			aes->addActorEntity(actorEntity);
		}
	}

	return aes;
}

bool ActorEntityScorer::hasHighQualityPatternMatch(ActorEntity_ptr actorEntity, ProperNounActorMention_ptr defaultCountryActorMention) {
	std::vector<ProperNounActorMention_ptr> &actorMentions = actorEntity->getActorMentions();
	BOOST_FOREACH(ProperNounActorMention_ptr actorMention, actorMentions) {
		if (actorMention->getPatternMatchScore() > 0.0 && actorMention->getPatternMatchScore() + actorMention->getAssociationScore() > 0.0) 
			return true;
	}
	return false;
}

bool ActorEntityScorer::countryMatch(ProperNounActorMention_ptr countryActorMention, ProperNounActorMention_ptr actorMention) {
	if (countryActorMention == ProperNounActorMention_ptr()) 
		return false;

	std::vector<ActorId> associatedCountries = _actorInfo->getAssociatedCountryActorIds(actorMention->getActorId());
	if (associatedCountries.size() == 0) 
		return false;

	BOOST_FOREACH(ActorId aid, associatedCountries) {
		if (aid == countryActorMention->getActorId())
			return true;
	}

	return false;
}

bool ActorEntityScorer::hasHighQualityGeoresolutionMatch(ActorEntity_ptr actorEntity) { 
	std::vector<ProperNounActorMention_ptr> &actorMentions = actorEntity->getActorMentions();
	BOOST_FOREACH(ProperNounActorMention_ptr actorMention, actorMentions) {
		if (actorMention->getGeoresolutionScore() > 0.0)
			return true;
	}
	return false;
}

int ActorEntityScorer::editDistanceQuality(ActorEntity_ptr actorEntity) {
	std::vector<ProperNounActorMention_ptr> &actorMentions = actorEntity->getActorMentions();

	BOOST_FOREACH(ProperNounActorMention_ptr actorMention, actorMentions) {
		EntityType entityType = actorMention->getEntityMention()->getEntityType();
		if (!entityType.matchesPER() && !entityType.matchesORG())
			return LOW;

		double score = actorMention->getEditDistanceScore();

		if (entityType.matchesPER() && score >= _per_edit_distance_high)
			return HIGH;
		if (entityType.matchesORG() && score >= _org_edit_distance_high)
			return HIGH;
	}

	BOOST_FOREACH(ProperNounActorMention_ptr actorMention, actorMentions) {
		EntityType entityType = actorMention->getEntityMention()->getEntityType();
		if (!entityType.matchesPER() && !entityType.matchesORG())
			return LOW;

		double score = actorMention->getEditDistanceScore();

		if (entityType.matchesPER() && score >= _per_edit_distance_medium)
			return MEDIUM;
		if (entityType.matchesORG() && score >= _org_edit_distance_medium)
			return MEDIUM;
	}
	return LOW;
}

ActorEntity_ptr ActorEntityScorer::getBestMatch(std::map<ActorId, ActorEntity_ptr> actorEntities) {
	double highest_pattern_match_score = -10000000.0;
	double highest_importance_score_for_pm_match = -10000000.0;
	double highest_georesolution_score = -10000000.0;
	double highest_edit_distance_score = -10000000.0;
	ActorId lowest_actor_id;

	std::set<ActorEntity_ptr> bestPatternMatches;
	ActorEntity_ptr bestGeoresolutionMatch = ActorEntity_ptr();
	ActorEntity_ptr bestEditDistanceMatch = ActorEntity_ptr();
	ActorEntity_ptr lowestActorId = ActorEntity_ptr();

	// Look for best matches of the different categories
	std::map<ActorId, ActorEntity_ptr>::iterator iter;
	for (iter = actorEntities.begin(); iter != actorEntities.end(); iter++) {
		ActorEntity_ptr actorEntity = iter->second;
		BOOST_FOREACH(ProperNounActorMention_ptr actorMention, actorEntity->getActorMentions()) {

			// Pattern match score (we remove the actor-id tiebreak that was unwisely added at the mention level
			//   so that we can prefer to break ties using importance score where possible... then we break ties
			//   by actor ID). This only works if we assume that the actor-id tiebreak is <1.0
			// We gather all the pattern matches with the best scores and then we're going to tiebreak later
			double pattern_match_score = ceil(actorMention->getPatternMatchScore()) + actorMention->getAssociationScore();
			if (pattern_match_score > highest_pattern_match_score && actorMention->getPatternMatchScore() > 0.0) {
				highest_pattern_match_score = pattern_match_score;
				bestPatternMatches.clear();
				bestPatternMatches.insert(actorEntity);
				// We will, below, restrict to only those entities with this good an importance score, so that
				//  will be our first level of tiebreak
				highest_importance_score_for_pm_match = actorMention->getImportanceScore();
			} else if (pattern_match_score == highest_pattern_match_score && actorMention->getPatternMatchScore() > 0.0) {
				bestPatternMatches.insert(actorEntity);				
				if (actorMention->getImportanceScore() > highest_importance_score_for_pm_match)
					highest_importance_score_for_pm_match = actorMention->getImportanceScore();
			}  

			// Georesolution score (tie break, lowest ActorId)
			double georesolution_score = actorMention->getGeoresolutionScore();
			if (georesolution_score == highest_georesolution_score &&
				bestGeoresolutionMatch &&
				actorEntity->getActorId() < bestGeoresolutionMatch->getActorId())
			{
				bestGeoresolutionMatch = actorEntity;
			}
			if (georesolution_score > highest_georesolution_score && georesolution_score > 0.0) {
				highest_georesolution_score = georesolution_score;
				bestGeoresolutionMatch = actorEntity;
			}

            // Edit distance score (tie break, lowest ActorId)
			double edit_distance_score = actorMention->getEditDistanceScore();
			if (edit_distance_score == highest_edit_distance_score && 
				bestEditDistanceMatch && 
				actorEntity->getActorId() < bestEditDistanceMatch->getActorId())
			{
				bestEditDistanceMatch = actorEntity;
			}
			if (edit_distance_score > highest_edit_distance_score && edit_distance_score > 0.0) {
				highest_edit_distance_score = edit_distance_score;
				bestEditDistanceMatch = actorEntity;
			}

			// Lowest ActorId
		    if (lowest_actor_id == ActorId() || actorMention->getActorId() < lowest_actor_id) {
				lowest_actor_id = actorMention->getActorId();
				lowestActorId = actorEntity;
			}
		}
	}

	if (bestPatternMatches.size() > 0) {

		ActorId lowest_actor_id_pm;
		ActorEntity_ptr lowestActorIdPM = ActorEntity_ptr();

		for (std::set<ActorEntity_ptr>::iterator iter = bestPatternMatches.begin(); iter != bestPatternMatches.end(); iter++) {
			BOOST_FOREACH(ProperNounActorMention_ptr actorMention, (*iter)->getActorMentions()) {
				if (highest_importance_score_for_pm_match > 0 && actorMention->getImportanceScore() < highest_importance_score_for_pm_match)
					continue;
				if (lowest_actor_id_pm == ActorId() || actorMention->getActorId() < lowest_actor_id_pm) {
					lowest_actor_id_pm = actorMention->getActorId();
					lowestActorIdPM = *iter;
				}
			}
		}

		return lowestActorIdPM;
	}

	if (bestGeoresolutionMatch)
		return bestGeoresolutionMatch;
	if (bestEditDistanceMatch)
		return bestEditDistanceMatch;
	return lowestActorId;
}

ActorEntity_ptr ActorEntityScorer::getImportantMatch(std::map<ActorId, ActorEntity_ptr> actorEntities) {
	ActorEntity_ptr mostImportant = ActorEntity_ptr();
	double highest_importance_score = 0.0;
	std::map<ActorId, ActorEntity_ptr>::iterator iter;
	for (iter = actorEntities.begin(); iter != actorEntities.end(); iter++) {
		ActorEntity_ptr actorEntity = iter->second;
		double importance_score = actorEntity->getImportanceScore();
		if (importance_score > highest_importance_score) {
			mostImportant = actorEntity;
			highest_importance_score = importance_score;
		}
	}
	if (!mostImportant || highest_importance_score < 20.0)
		return ActorEntity_ptr();

	// If all other scores are < 10
	// OR
	// If there is a gap fof at least 20 between mostImportant and all others 
	// THEN
	// return mostImportant
	bool found_high_score = false;
	bool found_small_gap = false;
	for (iter = actorEntities.begin(); iter != actorEntities.end(); iter++) {
		ActorEntity_ptr actorEntity = iter->second;
		if (actorEntity == mostImportant)
			continue;
		double importance_score = actorEntity->getImportanceScore();
		if (importance_score >= 10.0)
			found_high_score = true;
		if (highest_importance_score - importance_score < 20.0)
			found_small_gap = true;
	}
	if (!found_high_score) 
		return mostImportant;
	if (!found_small_gap) 
		return mostImportant;

	return ActorEntity_ptr();
}

double ActorEntityScorer::getLowEditDistanceThreshold(EntityType entityType) {
	if (entityType.matchesPER())
		return _per_edit_distance_low;
	if (entityType.matchesORG())
		return _org_edit_distance_low;
	return 0.825;
}

double ActorEntityScorer::getMediumEditDistanceThreshold(EntityType entityType) {
	if (entityType.matchesPER())
		return _per_edit_distance_medium;
	if (entityType.matchesORG())
		return _org_edit_distance_medium;
	return 0.875;
}

double ActorEntityScorer::getHighEditDistanceThreshold(EntityType entityType) {
	if (entityType.matchesPER())
		return _per_edit_distance_high;
	if (entityType.matchesORG())
		return _org_edit_distance_high;
	return 0.9;
}

double ActorEntityScorer::getTSTEditDistanceEquivalent(EntityType entityType) {
	if (entityType.matchesPER())
		return _per_tst_edit_distance_equivalent;
	if (entityType.matchesORG())
		return _org_tst_edit_distance_equivalent;
	return 0.8;
}
