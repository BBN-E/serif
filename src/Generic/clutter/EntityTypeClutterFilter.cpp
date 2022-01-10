// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "Generic/clutter/EntityTypeClutterFilter.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/RelationSet.h"
#include "Generic/theories/Relation.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/EventSet.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/DocTheory.h"

#include <string>

EntityTypeClutterFilter::EntityTypeClutterFilter(const wchar_t *type_str) : type(Symbol(type_str))
	, filterName(getFilterName(type_str))
{
}

void EntityTypeClutterFilter::filterClutter (DocTheory* docTheory){
	entitySet = docTheory->getEntitySet();
	relationSet = docTheory->getRelationSet();
	getMentions(docTheory);
	eventSet = docTheory->getEventSet();

	handleEntities();
	handleRelations();
	handleEvents();
}

void EntityTypeClutterFilter::handleEntities() {
	int n_ents = entitySet->getNEntities();
	for (int i = 0; i < n_ents; ++i) {
		Entity *ent (entitySet->getEntity(i));
		ent->applyFilter(filterName, this);
	}
}

void EntityTypeClutterFilter::handleRelations () {
	int n_rels = relationSet->getNRelations();
	for (int i = 0; i < n_rels; i++) {
		Relation *rel = relationSet->getRelation(i);
		rel->applyFilter(filterName, this);
	}
}

void EntityTypeClutterFilter::handleEvents () {
	int n_evns = eventSet->getNEvents();
	for (int i = 0; i < n_evns; ++i) {
		Event *event (eventSet->getEvent(i));
		event->applyFilter(filterName, this);
	}
}
	
void EntityTypeClutterFilter::getMentions(DocTheory *docTheory){
	numMentionSets = docTheory->getNSentences();
	for (int i=0; i< numMentionSets; i++) {
		SentenceTheory * sentTheory = docTheory->getSentenceTheory(i);
		mentionSets[i] = sentTheory->getMentionSet(); 
	}
}

std::string EntityTypeClutterFilter::getFilterName (const wchar_t *wtype_str){
	char const *type_str = Symbol(wtype_str).to_debug_string();
	std::string nameString = "remove-type-";
	nameString += type_str;
	return nameString;
}


bool EntityTypeClutterFilter::filtered (const Mention *ment, double *score) const {
	double s (0.);
	if (ment->getEntityType() == type) {
		s = 1.;
	}
	if (score != 0) *score = s;
	return s > 0.;
}

bool EntityTypeClutterFilter::filtered (const Entity *ent, double *score) const {
	double s (0.);
	if (ent->getType() == type) {
		s = 1.;
	}
	if (score != 0) *score = s;
	return s > 0.;
}

bool EntityTypeClutterFilter::filtered (const Relation *rel, double *score) const {
	double s (0.);
	Entity *left (entitySet->getEntity(rel->getLeftEntityID()));
	Entity *right (entitySet->getEntity(rel->getRightEntityID()));
	if (left->getType() == type || right->getType() == type) {
		s = 1.;
	}
	if (score != 0) *score = s;
	return s > 0.;
}

bool EntityTypeClutterFilter::filtered (const Event *eve, double *score) const {
	double s (0.);
	Event::LinkedEventMention const *mentions = eve->getEventMentions();
	while (mentions != 0) {
		int n_args = mentions->eventMention->getNArgs();
		for (int j = 0; j < n_args; j++) {
			const Mention *ment = mentions->eventMention->getNthArgMention(j);
			if (type == ment->getEntityType())
			{
				s = 1.;
			}
		}
		mentions = mentions->next;
	}
	if (score != 0) *score = s;
	return s > 0.;
}
