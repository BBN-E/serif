// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATION_PROMOTER_H
#define RELATION_PROMOTER_H


#include "Generic/common/limits.h"
#include "Generic/common/DebugStream.h"
class DocTheory;
class Relation;
class EntitySet;
class EventSet;
class Mention;
class Event;
class Entity;
class EventLinker;

class RelationPromoter {
public:
	RelationPromoter();
	void promoteRelations(DocTheory* docTheory);

	DebugStream _debugStream;

private:	
	Relation *_relationList[MAX_DOCUMENT_RELATIONS];
	int _n_relations;
	Relation *findRelationInList(int leftid, int rightid);
	Relation *findRelationWithSameHeads(const Mention *leftMention,
		const Mention *rightMention);
	bool isSubsumedUnderEvent(EventSet *eventSet,
		const Mention *leftMention,
		const Mention *rightMention);

	bool _merge_across_type;
	bool _allow_events_to_subsume_relations;
	bool _ignore_relations_with_matching_heads;
	bool ADD_IMAGINARY_RELATIONS;
	Symbol _imaginaryRelationType;
	int _imaginary_threshold;
	void addImaginaryRelations(const EntitySet *entitySet, int n_sentences);
	void addImaginaryRelation(const EntitySet *entitySet, Entity *entity1, 
		Entity *entity2, int sent, int id);

	EventLinker *_eventLinker;

};

#endif
