// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CORRECT_ENTITY_H
#define CORRECT_ENTITY_H

#include "Generic/CASerif/correctanswers/CorrectMention.h"
#include "common/Sexp.h"
#include "common/UnexpectedInputException.h"
#include "theories/EntityType.h"


class CorrectEntity
{
private:
	
	int _system_entity_id;
	EntityType *_entityType;
	EntitySubtype _entitySubtype;
	bool _is_generic;
	bool _is_group_fn;
	Symbol _symbolicType;
	Symbol _symbolicSubtype;
	Symbol _symbolicClass;

	int _n_mentions;
	CorrectMention *_mentions;

	Symbol _annotationEntityID;
	bool _isEntityType(Symbol type);

	void normalizePremods();

public:
	CorrectEntity();
	~CorrectEntity();

	void loadFromSexp(Sexp *entitySexp);
	//void loadFromERE(ERE *entityERE);
	int getNMentions() { return _n_mentions; }
	CorrectMention * getMention(size_t index) { return &(_mentions[index]); }
	EntityType * getEntityType() { return _entityType; }
	EntitySubtype getEntitySubtype() { return _entitySubtype; }
	int getSystemEntityID() { return _system_entity_id; }
	void setSystemEntityID(int id) { _system_entity_id = id; }
	bool isGeneric() { return _is_generic; }
	Symbol getSymbolicType() { return _symbolicType; }
	Symbol getSymbolicSubtype() { return _symbolicSubtype; }
	Symbol getSymbolicClass() { return _symbolicClass; }
	Symbol getAnnotationEntityID() { return _annotationEntityID; }

};

#endif
