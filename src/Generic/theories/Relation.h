// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATION_H
#define RELATION_H

#include "Generic/theories/Theory.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/Attribute.h"

#include <iostream>
#include <string>
#include <map>

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;

class Relation;

class RelationClutterFilter {
public:
	virtual bool filtered (const Relation *, double *score = 0) const = 0;
};

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED Relation : public Theory {
public:
	int getID() const { return _ID; }
	int getLeftEntityID() const { return _left_entity_ID; }
	int getRightEntityID() const { return _right_entity_ID; }
	Symbol getType() const { return _type; }
	Symbol getRawType() const;

	ModalityAttribute getModality() { return _modality; }
	TenseAttribute getTense() { return _tense; }
	void setModality(ModalityAttribute modality) { _modality = modality; }
	void setTense(TenseAttribute tense) { _tense = tense; }

	void applyFilter (const std::string& filterName, RelationClutterFilter *filter);
	bool isFiltered (const std::string& filterName) const;
	double getFilterScore (const std::string& filterName) const;

	/** Return a confidence score for this relation.  Currently, this is
	 * defined as the maximum of the scores of the RelMentions that 
	 * are contained in this Relation. */
	float getConfidenceScore() const; 

public:
	struct LinkedRelMention {
		RelMention *relMention;
        LinkedRelMention* next;
		LinkedRelMention(RelMention *ment)
			: relMention(ment), next(0) {}
		~LinkedRelMention() { delete next; }
    };

	Relation(RelMention *ment, int l_id, int r_id, int relid) :
		_modality(Modality::ASSERTED), 
		_tense(Tense::UNSPECIFIED) ,
		_filters()
	{
		_relMentions = _new LinkedRelMention(ment);
		_left_entity_ID = l_id;
		_right_entity_ID = r_id;
		_type = ment->getType();
		_ID = relid;
	}

	Relation(Relation &other, int set_offset, int entity_offset, int sent_offset, std::vector<RelMentionSet*> mergedRelMentionSets);

	~Relation() {
		delete _relMentions;
	}

	void addRelMention(RelMention *ment) {
		LinkedRelMention *last = _relMentions;
		while (last->next != 0)
			last = last->next;
		last->next = _new LinkedRelMention(ment);
	}

	const LinkedRelMention * getMentions() const { return _relMentions; }

	std::wstring toString() const;
	std::string toDebugString() const;

	void dump(UTF8OutputStream &out, int indent) const;
	void dump(std::ostream &out, int indent) const;

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	Relation(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit Relation(SerifXML::XMLTheoryElement elem, int relation_id);
	const wchar_t* XMLIdentifierPrefix() const;

	// for now, just take the modality/tense of the first mention
	void setModalityFromMentions() { _modality = getMentions()->relMention->getModality(); }
	void setTenseFromMentions() { _modality = getMentions()->relMention->getModality(); }
	
protected:
	int _ID;
	Symbol _type;
	int _left_entity_ID;
	int _right_entity_ID;
	ModalityAttribute _modality;
	TenseAttribute _tense;

	std::map<std::string, double> _filters;

	LinkedRelMention *_relMentions;

};

#endif
