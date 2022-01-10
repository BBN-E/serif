// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include

#include "relations/RelationTypeSet.h"
#include "Generic/CASerif/correctanswers/CorrectRelation.h"
#include "Generic/CASerif/correctanswers/CorrectRelMention.h"
#include "Generic/CASerif/correctanswers/CASymbolicConstants.h"
#include "common/Sexp.h"
#include "common/UnexpectedInputException.h"



CorrectRelation::CorrectRelation(): _mentions(0) { 
	_system_relation_id = -1;
}

CorrectRelation::~CorrectRelation() 
{
	delete [] _mentions;
}

void CorrectRelation::loadFromSexp(Sexp *relationSexp)
{
	int num_children = relationSexp->getNumChildren();
	if (num_children < 3 )
		throw UnexpectedInputException("CorrectRelation::loadFromSexp()",
									   "relationSexp doesn't have at least 3 children");

	Sexp *typeSexp = relationSexp->getNthChild(0);
	Sexp *classSexp = relationSexp->getNthChild(1);
	Sexp *relationIDSexp = relationSexp->getNthChild(2);

	if (!typeSexp->isAtom() || !classSexp->isAtom() || !relationIDSexp->isAtom())
		throw UnexpectedInputException("CorrectRelation::loadFromSexp()",
									   "Didn't find relation atoms in correctAnswerSexp");


	_annotationID = relationIDSexp->getValue();
	_modality = Symbol();
	_tense = Symbol();

	//if (_isRelationType(typeSexp->getValue())) 
		_relationType = typeSexp->getValue();
	//else 
	//	throw UnexpectedInputException("CorrectRelation::loadFromSexp()", 
	//								   "Unrecognized relation type in correct answer file");
	

	if (classSexp->getValue() == CASymbolicConstants::EXPLICIT_SYM)
		_is_explicit = true;
	else 
		_is_explicit = false;

	_n_mentions = num_children - 3;
	int i;
	
	if (_n_mentions > 0) 
		_mentions = new CorrectRelMention[_n_mentions];

	for (i = 0; i < _n_mentions; i++ ) {
		Sexp* mentionSexp = relationSexp->getNthChild(i + 3);
		_mentions[i].loadFromSexp(mentionSexp);
		_mentions[i].setCorrectRelation(this);
	}
}

/*void CorrectRelation::loadFromAdept(Relation *relationAdept)
{
	int num_children = relationSexp->getNumChildren();
	if (num_children < 3 )
		throw UnexpectedInputException("CorrectRelation::loadFromSexp()",
									   "relationSexp doesn't have at least 3 children");

	Sexp *typeSexp = relationSexp->getNthChild(0);
	Sexp *classSexp = relationSexp->getNthChild(1);
	Sexp *relationIDSexp = relationSexp->getNthChild(2);

	if (!typeSexp->isAtom() || !classSexp->isAtom() || !relationIDSexp->isAtom())
		throw UnexpectedInputException("CorrectRelation::loadFromSexp()",
									   "Didn't find relation atoms in correctAnswerSexp");

     long id = relationAdept.relationId;
	 std::string idString;
    std::stringstream strstream;
    strstream << id;
    strstream >> idString;

	_annotationID = Symbol(string_to_wstring(idString))
	_modality = Symbol();
	_tense = Symbol();


		_relationType = Symbol(string_to_wstring(relationAdept.type.type));
        _is_explicit = true;


	_n_mentions = 1
	
    _mentions = new CorrectRelMention[_n_mentions];
    
	if(r.arguments.size()>=2){
	_mentions[i].loadFromAdept(r.arguments);
	_mentions[i].setCorrectRelation(this);
	}
	
}*/

bool CorrectRelation::_isRelationType(Symbol type) {
	return (RelationTypeSet::getTypeFromSymbol(type) != RelationTypeSet::INVALID_TYPE);
}
