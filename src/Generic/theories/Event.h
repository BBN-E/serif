// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_H
#define EVENT_H

#include "Generic/theories/Theory.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/EventEntityRelation.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/ValueSet.h"
#include "Generic/common/Attribute.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"

#include <string>
#include <map>
#include <vector>

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;
class EventEntityRelation;
class Event;
class EventClutterFilter {
public:
	virtual bool filtered (const Event *, double *score = 0) const = 0;
};

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED Event : public Theory {
public:
	int getID() { return _ID; }
	Symbol getType() const { return _type; }
	Symbol getAnnotationID() { return _annotationID; }
	void setAnnotationID(Symbol sym) { _annotationID = sym; }

	struct LinkedEventMention {
		EventMention *eventMention;
        LinkedEventMention* next;
		LinkedEventMention(EventMention *ment)
			: eventMention(ment), next(0) {}
		~LinkedEventMention() { 
			delete next; 
		}
    };

	// holds the entity/value owners of the argument mentions
	// and their corresponding roles
	struct EventArguments {
		std::vector<const Entity*> entities;
		std::vector<const Value*> values;
		std::vector<Symbol> entity_roles;
		std::vector<Symbol> value_roles;
	};

	Event(int eventid) 
		: _eventMentions(0), _type(SymbolConstants::nullSymbol), 
		 _ID(eventid),
		 _annotationID(SymbolConstants::nullSymbol),
		 _modality(Modality::ASSERTED), _tense(Tense::UNSPECIFIED),
		 _genericity(Genericity::SPECIFIC), _polarity(Polarity::POSITIVE) {}

	Event(Event &other);
	Event(const Event &other, int set_offset, int sent_offset, std::vector<EventMentionSet*> mergedEventMentionSets);
	~Event();

	void addEventMention(EventMention *ment);
	void addEventMentionPointer(EventMention *ment);
	void mergeInEvent(Event *otherEvent);
	void removeEventMentions() { _eventMentions = 0; }
	std::wstring toString();
	std::string toDebugString();


	void dump(UTF8OutputStream &out, int indent);
	void dump(std::ostream &out, int indent);

	LinkedEventMention * getEventMentions() const { return _eventMentions; }
	float getScore();

	void consolidateEERelations(EntitySet *entitySet);

	const Mention *getFirstMentionForSlot(Symbol slotName);

	int getNumConsolidatedEERelations() { return (int) _consolidatedRelations.size(); }
	
	EventEntityRelation *getConsolidatedEERelation(int n) { return _consolidatedRelations[n]; }

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	Event(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit Event(SerifXML::XMLTheoryElement elem, int event_id);
	const wchar_t* XMLIdentifierPrefix() const;

	void getArgumentEntities(EventArguments* ea, const EntitySet* es, const ValueSet* vs, std::set<MentionUID> ms) const;
	void saveArgumentEntitiesXML(SerifXML::XMLTheoryElement elem, const DocTheory *docTheory) const;
	std::set<MentionUID> getDocumentMentionIDs(const DocTheory *docTheory) const;
    	bool _isPrintedMention(const Mention *ment, std::set<MentionUID> docMentions) const;

	GenericityAttribute getGenericity() { return _genericity; }
	PolarityAttribute getPolarity() { return _polarity; }
	TenseAttribute getTense() { return _tense; }
	ModalityAttribute getModality() { return _modality; }

	void setGenericity(GenericityAttribute genericity) { _genericity = genericity; }
	void setPolarity(PolarityAttribute polarity) { _polarity = polarity; }
	void setTense(TenseAttribute tense) { _tense = tense; }
	void setModality(ModalityAttribute modality) { _modality = modality; }

	/**
	 * generic filter interface
	 */
	void applyFilter (const std::string& filterName, EventClutterFilter *filter);
	bool isFiltered (const std::string& filterName) const;
	double getFilterScore (const std::string& filterName) const;
	const std::map<std::string, double>& getFilterMap() const { return _filters; }

	void setModalityFromMentions();
	void setTenseFromMentions();
	void setPolarityFromMentions();
	void setGenericityFromMentions();

protected:
	int _ID;
	Symbol _type;
	Symbol _annotationID;
	ModalityAttribute _modality;
	TenseAttribute _tense;
	PolarityAttribute _polarity;
	GenericityAttribute _genericity;

	LinkedEventMention *_eventMentions;
	/*
	 * generic filters
	 */
	std::map<std::string, double> _filters;

    	std::vector<EventEntityRelation*> _consolidatedRelations;

};

#endif
