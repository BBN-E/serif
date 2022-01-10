// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CORRECT_DOCUMENT_H
#define CORRECT_DOCUMENT_H

#include "common/Symbol.h"
#include "common/Sexp.h"
#include "Generic/CASerif/correctanswers/CorrectEntity.h"
#include "Generic/CASerif/correctanswers/CorrectValue.h"
#include "Generic/CASerif/correctanswers/CorrectMention.h"
#include "Generic/CASerif/correctanswers/CorrectRelation.h"
#include "Generic/CASerif/correctanswers/CorrectRelMention.h"
#include "Generic/CASerif/correctanswers/CorrectEvent.h"
#include "theories/NameSpan.h"
#include "theories/Mention.h"

class CorrectDocument
{

private:
	Symbol _id;
	int _n_entities;
	CorrectEntity *_entities;
	int _n_values;
	CorrectValue *_values;
	int _n_relations;
	CorrectRelation *_relations;
	int _n_events;
	CorrectEvent *_events;

	// cycles through all the mentions and turns any name that has
	// any other names nested inside of it into a Nominal. This is 
	// necessary, since our system assumes we don't find any nested 
	// names
	void _fixNestedNames();
	

public:
	CorrectDocument();
	~CorrectDocument();

	void loadFromSexp(Sexp *docSexp);
	
	Symbol getID() { return _id; }
	int getNEntities() { return _n_entities; }
	CorrectEntity * getEntity(size_t index) { return &(_entities[index]); }
	int getNValues() { return _n_values; }
	CorrectValue * getValue(size_t index) { return &(_values[index]); }
	int getNRelations() { return _n_relations; }
	CorrectRelation * getRelation(size_t index) { return &(_relations[index]); }
	int getNEvents() { return _n_events; }
	CorrectEvent * getEvent(size_t index) { return &(_events[index]); }

	CorrectMention * getCorrectMentionFromNameSpan(NameSpan *nameSpan);
	CorrectMention * getCorrectMentionFromMentionID(Mention::UID id);
	CorrectEntity * getCorrectEntityFromCorrectMention(CorrectMention *cm);
	CorrectEntity * getCorrectEntityFromEntityID(int id);
	CorrectValue * getCorrectValueFromValueMentionID(ValueMention::UID id);


	bool hasNameMentionNestedInside(CorrectMention *cm);
	bool isNestedInsideNameMention(CorrectMention *cm);
};

#endif
