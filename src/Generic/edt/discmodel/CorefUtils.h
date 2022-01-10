// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef COREF_UTILS_H
#define COREF_UTILS_H

#include "Generic/theories/Mention.h"

class EntitySet;
class Entity;

struct EntityGuess;
struct LinkGuess;

class CorefUtils {
public:
	static int addNearestEntities(GrowableArray<Entity *> &candidates, EntitySet *entitySet , Mention * ment, int maxEntities);
	static void insertEntityIntoSortedArray(GrowableArray<Entity *> &sortedArray, const EntitySet *currSolution, Entity *ent, const Mention *ment, int maxEntities);
	static MentionUID getLatestMention(const EntitySet *currSolution, const Entity* ent, const Mention *ment);
	static bool subtypeMatch(const EntitySet *currSolution, const Entity *entity, const Mention *mention);
	static Mention::Type getEntityMentionLevel(const EntitySet *entitySet, const Entity *entity);
	static void computeConfidence(EntityGuess results[], int nResults);
	static void computeConfidence(LinkGuess results[], int nResults);
};

#endif
