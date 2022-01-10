// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENTITYTYPE_CLUTTER_FILTER_H
#define ENTITYTYPE_CLUTTER_FILTER_H

#include "Generic/clutter/ClutterFilter.h"
#include "Generic/theories/Relation.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/Mention.h"

#include <string>

class EntityType;
class DocTheory;
class EntitySet;
class RelationSet;
class EventSet;
class MentionSet;

/* filters (marks) mentions, entities and relations that have a specific EntityType.
 * currently used for the ace2008 evaluation where types WEA and VEH are not marked.
 */
class EntityTypeClutterFilter : public ClutterFilter
	, public EntityClutterFilter, public RelationClutterFilter, public EventClutterFilter {
public:	
	EntityTypeClutterFilter(const wchar_t *type_str);
	~EntityTypeClutterFilter() {};
	virtual void filterClutter (DocTheory* docTheory);
	const std::string getFilterName() { return filterName; };
	static std::string getFilterName(const wchar_t *type_str);

protected:
	EntityType type;
	const std::string filterName;

	EntitySet *entitySet;
	RelationSet *relationSet;
	EventSet *eventSet;
	int numMentionSets;
	MentionSet* mentionSets[MAX_DOCUMENT_SENTENCES];
	void getMentions(DocTheory *docTheory);

	void handleEntities();
	void handleRelations();
	void handleEvents();

	bool filtered (const Mention *ment, double *score) const;
	bool filtered (const Entity *ent, double *score) const;
	bool filtered (const Relation *rel, double *score) const;
	bool filtered (const Event *event, double *score) const;
};

#endif
