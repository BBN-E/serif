// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ATEA_INTERNALCLUTTERFILTER
#define ATEA_INTERNALCLUTTERFILTER

#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/RelationSet.h"
#include "Generic/theories/Relation.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/EventSet.h"
#include "English/clutter/en_ClutterFilter.h"

class ATEAInternalClutterFilter: public ClutterFilter
	, public EntityClutterFilter, public RelationClutterFilter, public EventClutterFilter {
	std::map<int, double> entityScore;
	std::map<int, double> relationScore;
	std::map<int, double> eventScore;

	std::string filterName;

	EntitySet *entitySet;
	RelationSet *relationSet;
	EventSet *eventSet;

public:
	ATEAInternalClutterFilter () :entitySet(0), relationSet(0), eventSet(0) {
		filterName = "ATEAEnglishRuledBasedClutterFilter";
	}

	ATEAInternalClutterFilter (DocTheory *theory) :entitySet(0), relationSet(0), eventSet(0) {
		filterName = "ATEAEnglishRuledBasedClutterFilter";
		filterClutter (theory);
	}

	~ATEAInternalClutterFilter () {
	}

	bool filtered (const Entity *ent, double *score = 0) const {
		double s (0.);
		std::map<int, double>::const_iterator p (entityScore.find(ent->getID()));
		if (p != entityScore.end()) {
			s = p->second;
		}
		else {
			s = checkClutter(ent);
		}
		if (score != 0) *score = s;
		return s > 0.;
	}

	bool filtered (const Relation *rel, double *score = 0) const {
		double s (0.);
		std::map<int, double>::const_iterator p (relationScore.find(rel->getID()));
		if (p != relationScore.end()) {
			s = p->second;
		}
		else {
			s = checkClutter (rel);
		}
		if (score != 0) *score = s;
		return s > 0.;
	}

	bool filtered (const Event *evn, double *score = 0) const {
		if (score != 0) *score = -1.;
		return false;
	}

	void filterClutter (DocTheory *theory) {
		entitySet = theory->getEntitySet();
		relationSet = theory->getRelationSet();
		eventSet = theory->getEventSet();

		entityScore.clear();
		relationScore.clear();
		eventScore.clear();

		eventHandler ();
		relationHandler ();
		entityHandler ();

		applyFilter ();
	}

	void applyFilter () {
		int n_ents = entitySet->getNEntities();
		for (int i = 0; i < n_ents; ++i) {
			Entity *ent (entitySet->getEntity(i));
			ent->applyFilter(filterName, this);
		}

		int n_rels = relationSet->getNRelations();
		for (int i = 0; i < n_rels; i++) {
			Relation *rel = relationSet->getRelation(i);
			rel->applyFilter(filterName, this);
#if 0
			cout << "rel " << rel->getID() << " clutter? " 
				<< rel->isFiltered(EnglishClutterFilter::filterName) << endl;
#endif
		}
	}


	bool is_matched (const Entity *e, const wchar_t *s) const {
		return ::wcscmp(e->getType().getName().to_string(), s) == 0;
	}

	bool is_gpe (const Entity *e) const { return is_matched (e, L"GPE"); }
	bool is_loc (const Entity *e) const { return is_matched (e, L"LOC"); }
	bool is_wea (const Entity *e) const { return is_matched (e, L"WEA"); }
	bool is_veh (const Entity *e) const { return is_matched (e, L"VEH"); }

	bool isNam (const Mention *m) const {
		switch (m->getMentionType()) {
		case Mention::NAME:
			return true;
		case Mention::LIST:
			return isNam (m->getChild());
        default:
            return false;
		}
	}

	bool isNam (const Entity *ent) const {
		for (int i (0); i < ent->mentions.length(); ++i) {
			Mention *m (entitySet->getMention(ent->mentions[i]));
			if (isNam (m)) {
				return true;
			}
		}
		return false;
	}

	bool isRef (const Entity *ent) const {
		int n_rels = relationSet->getNRelations();
		int nrefs (0), clutters (0);
		for (int i = 0; i < n_rels; i++) {
			Relation *rel = relationSet->getRelation(i);		
			if ((ent->getID() == rel->getLeftEntityID()
				|| ent->getID() == rel->getRightEntityID())) {
				if (checkClutter (rel) > 0.) {
					++clutters;
				}
				++nrefs;
			}
		}
#if 0
		if (ent->getID() == 2) {
			cout << "nrefs = " << nrefs << " clutters = " << clutters << endl;
		}
#endif
		return clutters < nrefs;
	}

	bool isRefEvent (const Entity *ent) const {
		std::map<int, double>::const_iterator ref (eventScore.find(ent->getID()));
		return ref != eventScore.end();
	}

	double checkClutter (const Relation *rel) const {
		if (rel->getType() == Symbol(L"IDENT") 
			|| const_cast<Relation *>(rel)->getMentions() == 0)
			return 1.;

		Entity *left (entitySet->getEntity(rel->getLeftEntityID()));
		Entity *right (entitySet->getEntity(rel->getRightEntityID()));

		if ((left->isGeneric() || right->isGeneric()) && !(::wcscmp(rel->getType().to_string(), L"UNKNOWN") == 0))
			return 1.;

		bool isClutter (((!isNam(left) && !isNam(right)) // test2p
						|| (!isNam(left) && (is_gpe (right) || is_loc (right)))) // test3p
						&& !(::wcscmp(rel->getType().to_string(), L"UNKNOWN") == 0)); // test4p

		return isClutter ? 1. : 0.;
	}

	double checkClutter (const Entity *ent) const {
		bool isClutter (!isRef (ent) && !isRefEvent (ent)
						&& !isNam (ent) && !is_wea (ent) && !is_veh (ent));
#if 0
		if (ent->getID() == 33) {
			cout << "isRef = " << isRef (ent) << endl
				<< "isRefEvent = " << isRefEvent (ent) << endl
				<< "isNam = " << isNam(ent) << endl
				<< "is_wea = " << is_wea (ent) << endl
				<< "is_veh = " << is_veh (ent) << endl;
		}
#endif
		return isClutter ? 1. : 0.;
	}

	void relationHandler () {
		int n_rels = relationSet->getNRelations();
		for (int i = 0; i < n_rels; i++) {
			Relation *rel = relationSet->getRelation(i);
			relationScore[rel->getID()] = checkClutter (rel);
		}
	}

	void entityHandler () {
		int n_ents = entitySet->getNEntities();
		for (int i = 0; i < n_ents; ++i) {
			Entity *ent (entitySet->getEntity(i));
			entityScore[ent->getID()] = checkClutter (ent);
		}
	}

	void eventHandler () {
		int n_evns = eventSet->getNEvents();
		for (int i = 0; i < n_evns; ++i) {
			Event *event (eventSet->getEvent(i));
			Event::LinkedEventMention *mentions = event->getEventMentions();
			while (mentions != 0) {
				EventMention *em = mentions->eventMention;
				for (int j = 0; j < em->getNArgs(); ++j) {
					const Mention *mention = em->getNthArgMention(j);
					const Entity *ent = entitySet->getEntityByMention(mention->getUID());
					if (ent != 0 && !ent->isGeneric()) {
						eventScore[ent->getID()] = 1;
					}
				}
				mentions = mentions->next;
			}
		}
	}
};
#endif
