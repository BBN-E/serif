// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/GrowableArray.h"
#include "Generic/edt/discmodel/CorefUtils.h"
#include "Generic/theories/EntitySet.h"

#include "Generic/theories/Entity.h"
#include "Generic/edt/EntityGuess.h"
#include "Generic/edt/LinkGuess.h"

#include <boost/math/special_functions/fpclassify.hpp>


/**
 *  Add the non-ACE entities and entities of undetermined type from
 *  <code>entitySet</code> to a list of candidate entities.  The new
 *  entities should be sorted w.r.t. distance from a target Mention.
 *
 *  @param candidates the array to add to 
 *  @param entitySet the EntitySet
 *  @param ment the target Mention
 *  @param maxEntities the maximum number of entities to add
 *  @return the number of entities added to the array
 */
int CorefUtils::addNearestEntities(GrowableArray<Entity *> &candidates, EntitySet *entitySet , Mention * ment, int maxEntities){
	const GrowableArray<Entity *> &other = entitySet->getEntitiesByType(EntityType::getOtherType());
	const GrowableArray<Entity *> &undet = entitySet->getEntitiesByType(EntityType::getUndetType());
	GrowableArray<Entity *> sortedNonACE(maxEntities);
	for (int i = 0; i < other.length(); i++) {
		Entity *ent = other[i];
		insertEntityIntoSortedArray(sortedNonACE, entitySet, ent, ment, maxEntities);
	}

	for (int i = 0; i < undet.length(); i++) {
		Entity *ent = undet[i];
		insertEntityIntoSortedArray(sortedNonACE, entitySet, ent, ment, maxEntities);
	}

	for (int i = 0; i < sortedNonACE.length(); i++)
		candidates.add(sortedNonACE[i]);

	return sortedNonACE.length();
}



/**
 *  Insert an Entity into an array of entities sorted
 *  ascendingly w.r.t. link distance from a target 
 *  Mention
 *  
 *  @param sortedArray the array into which we want to insert
 *  @param currSolution the current working EntitySet
 *  @param ent the Entity in question
 *  @param ment the target Mention
 *  @param maxEntities the maximum number of entities 
 *                     permitted in <code>sortedArray</code>
 */
void CorefUtils::insertEntityIntoSortedArray(GrowableArray<Entity *> &sortedArray, 
											 const EntitySet *currSolution, 
											 Entity *ent, const Mention *ment, 
											 int maxEntites) 
{

	MentionUID lastID = getLatestMention(currSolution, ent, ment);
	if (lastID.isValid()) {
		// find where the entity should be added
		bool inserted = false;
		for (int j = 0; j < sortedArray.length(); j++) {
			if (lastID < getLatestMention(currSolution, sortedArray[j], ment)) {
				// move the last one down one
				if (sortedArray.length() == maxEntites) {
					for (int k = 0; k < j; k++)
						sortedArray[k] = sortedArray[k+1];
				} else {
					sortedArray.add(sortedArray[sortedArray.length()-1]);
					for (int k = sortedArray.length()-2; k >= j; k--)
						sortedArray[k+1] = sortedArray[k];
				}
				sortedArray[j] = ent;
				inserted = true;
				break;
			}
		}
		if (!inserted && (sortedArray.length() < maxEntites)) {
			sortedArray.add(ent);
		}
		if (sortedArray.length() > maxEntites) {
			Entity *tmp, *nextEnt = sortedArray.removeLast();
			for (int i = sortedArray.length()-1; i >= 0; i--) {
				tmp = sortedArray[i];
				sortedArray[i] = nextEnt;
				nextEnt = tmp;
			}
		}
	}
}

/**
 *  Of all of the mentions in <code>ent</code> that will be linked before 
 *  <code>ment</code> chronologically, find the one that linked last. 
 *  
 *  @param currSolution the current working EntitySet
 *  @param ent the Entity whose Mention we want to identify
 *  @param ment the target Mention
 *  @return the id of the closest Mention or -1 if none found
 */
MentionUID CorefUtils::getLatestMention(const EntitySet *currSolution, const Entity* ent, const Mention *ment) {
	MentionUID lastID;
	for (int j = 0; j < ent->getNMentions(); j++) {
		if (ent->getMention(j) > lastID) {
			Mention * entMention = currSolution->getMention(ent->getMention(j));
			if (ent->getMention(j) < ment->getUID()) {
				lastID = ent->getMention(j);
			}
			// Coref links all names in a sentence, then all descriptors, then all pronouns, so we need to
			// make an exception for mentions that appear in the same sentence as the target mention.
			else if (entMention != 0 && entMention->getSentenceNumber() == ment->getSentenceNumber()) {
				Mention::Type entMentionType = entMention->getMentionType();
				Mention::Type mentType = ment->getMentionType();
				if (entMentionType == Mention::NAME &&
					(mentType == Mention::DESC || mentType == Mention::PRON))
					lastID = ent->getMention(j);
				else if (entMentionType == Mention::DESC && mentType == Mention::PRON) 
					lastID = ent->getMention(j);
			}
		}
	}
	return lastID;
}

/**
 *  @param currSolution the current working EntitySet
 *  @param entity the Entity in question
 *  @param mention the Mention in question
 *  @return false if <code>entity</code> and <code>mention</code>
 *               have both been assigned subtypes and the subtypes
 *               do not match, true otherwise
 */
bool CorefUtils::subtypeMatch(const EntitySet *currSolution,
						      const Entity *entity, const Mention *mention) 
{
	EntitySubtype entSubtype = currSolution->guessEntitySubtype(entity);
	EntitySubtype mentSubtype = mention->getEntitySubtype();
	if ((entSubtype.isDetermined() && mentSubtype.isDetermined()) && 
		 mentSubtype != entSubtype)
	{
		return false;
	}
	return true;
}

/**
 *  Find the ACE mention level for an Entity.
 *
 *  Note: this method currently only distinguishes 
 *  entities of Mention::NAME level from all others.
 *
 *  @param entitySet the current EntitySet
 *  @param entity the Entity in question
 *  @return Mention::NAME if the entity contains at least
 *          one name Mention, Mention::NONE otherwise
 */
Mention::Type CorefUtils::getEntityMentionLevel(const EntitySet *entitySet, const Entity *entity) {
	size_t n_ments = entity->getNMentions();
	Mention::Type mentType;
	Mention::Type entType = entitySet->getMention(entity->getMention(0))->getMentionType();
	if (entType == Mention::NAME)
		return Mention::NAME;
	for (size_t i=1; i<n_ments; i++) {
		mentType = entitySet->getMention(entity->getMention(i))->getMentionType();
		if (mentType == Mention::NAME) {
			return Mention::NAME;
		}// else
		if (mentType == Mention::DESC || mentType == Mention::APPO) {
//			entType = mentType;
		} else if (mentType>entType && entType != Mention::DESC && entType != Mention::APPO) {
//			entType = mentType;
		}
	}// for
	return Mention::NONE;
}

/**
 *  Given an array of EntityGuess objects with scores, compute
 *  the confidence for each score and set the corresponding
 *  <code>linkConfidence</code> field for each EntityGuess
 * 
 *  @param results the array of EntityGuess object
 *  @param nResults the length of the array
 */
void CorefUtils::computeConfidence(EntityGuess results[], int nResults) {

	if (nResults < 3) { // set confidence to maximum
		for (int i = 0; i < nResults; i++) {
			results[i].linkConfidence = static_cast<float>(1.0);
		}
		return;
	}

	float normalizer = static_cast<float>(((fabs(results[2].score))+ fabs(results[1].score))/2);
	if (normalizer <=0.0) return; // to make sure we are not dividing by 0;
	float sec_best_conf = static_cast<float>(results[1].score/normalizer);
	float best_conf = static_cast<float>(results[0].score/normalizer-sec_best_conf);
	// this is to prevent overflow in the exponent
	float divider = (best_conf > 600) ? best_conf/600 : 1;

	float sum_of_confs = 0.0;
	for (int i = 0; i < nResults; i++) {
		// normalizing the scores
		results[i].linkConfidence = static_cast<float>(results[i].score/normalizer);
		results[i].linkConfidence -= sec_best_conf;
		results[i].linkConfidence /= divider;
		results[i].linkConfidence = exp(results[i].linkConfidence);
		sum_of_confs += results[i].linkConfidence;
	}

	for (int i = 0; i < nResults; i++) {
		// normalizing the scores
		results[i].linkConfidence /= sum_of_confs;
		// just set it to 1.0 for now if we're getting NaN
		if (boost::math::isnan(results[i].linkConfidence))
			results[i].linkConfidence = static_cast<float>(1.0);
	}
	
}

/**
 *  Given an array of LinkGuess objects with scores, compute
 *  the confidence for each score and set the corresponding
 *  <code>linkConfidence</code> field for each LinkGuess
 * 
 *  @param results the array of LinkGuess object
 *  @param nResults the length of the array
 */
void CorefUtils::computeConfidence(LinkGuess results[], int nResults) {

	if (nResults < 3) { // set confidence to maximum
		for (int i = 0; i < nResults; i++) {
			results[i].linkConfidence = static_cast<float>(1.0);
		}
		return;
	}

	float normalizer = static_cast<float>((fabs(results[2].score)+ fabs(results[1].score))/2);
	if (normalizer <= 0.0) return; // to make sure we are not dividing by 0;
	float sec_best_conf = static_cast<float>(results[1].score/normalizer);
	float best_conf = static_cast<float>(results[0].score/normalizer-sec_best_conf);
	// this is to prevent overflow in the exponent
	float divider = (best_conf > 600) ? best_conf/600 : 1;

	float sum_of_confs = 0.0;
	for (int i = 0; i < nResults; i++) {
		// normalizing the scores
		results[i].linkConfidence = static_cast<float>(results[i].score/normalizer);
		results[i].linkConfidence -= sec_best_conf;
		results[i].linkConfidence /= divider;
		results[i].linkConfidence = exp(results[i].linkConfidence);
		sum_of_confs += results[i].linkConfidence;
	}

	for (int i = 0; i < nResults; i++) {
		// normalizing the scores
		results[i].linkConfidence /= sum_of_confs;
		// just set it to 1.0 for now if we're getting NaN
		if (boost::math::isnan(results[i].linkConfidence))
			results[i].linkConfidence = static_cast<float>(1.0);
	}
	
}
