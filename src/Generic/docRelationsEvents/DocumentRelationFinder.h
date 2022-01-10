// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DOCUMENT_RELATION_FINDER_H
#define DOCUMENT_RELATION_FINDER_H

class DocTheory;
class Sexp;
class RelMention;
class Entity;

#include "Generic/CASerif/correctanswers/CorrectAnswers.h"

class DocumentRelationFinder {
public:
	DocumentRelationFinder(bool use_correct_relations = false);
	~DocumentRelationFinder();
	void cleanup();

	CorrectAnswers *_correctAnswers;

	/**
	 * This does the work. It adds relations to the DocTheory
     **/
	void findSentenceLevelRelations(DocTheory *docTheory);
	void findDocLevelRelations(DocTheory *docTheory);

protected:
	void expandRelationsInListMentions(RelMentionSet* rmSet, DocTheory *docTheory);
	void expandRelationInListMention(RelMentionSet* rmSet, RelMention* rm, const Mention* listMember, const Mention* otherMention, DocTheory *docTheory);

private:
	class StructuralRelationFinder *_structuralRelFinder;
	class RelationFinder *_relationFinder;
	class ZonedRelationFinder *_zonedRelationFinder;
	class FamilyRelationFinder *_familyRelationFinder;
	class RelationTimexArgFinder *_relationTimexArgFinder;
	const bool _use_correct_relations;
	bool _do_family_relations;
	bool _do_employment_relations;
	bool _do_relation_time_attachment;
	bool _relation_time_attachment_use_primary_parse;
	bool _expand_relations_in_list_mentions;
	bool DEBUG;
	UTF8OutputStream _debugStream;

	typedef struct {
		Symbol eventType;
		Symbol firstRole;
		Symbol secondRole;
		Symbol relationType;
	} EventRelationPattern;

	EventRelationPattern *_eventRelationPatterns;
	int _num_event_rel_patterns;

	class RelationObservation *_observation;
	
	bool _use_correct_answers;
};


#endif
