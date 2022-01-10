// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include

#include "Generic/CASerif/correctanswers/CorrectDocument.h"
#include "common/Sexp.h"
#include "common/UnexpectedInputException.h"
#include "common/InternalInconsistencyException.h"
#include "theories/SynNode.h"


/*#ifndef WIN32
#include <netinet/in.h>
#endif
#include <fcntl.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <DEFT/adept-thrift-cpp/Serializer.h>
#include "DEFTTester.h"


using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace thrift::adept::serialization;
using namespace thrift::adept::common; 

using namespace boost;
*/
CorrectDocument::CorrectDocument(): _entities(0), _values(0), _relations(0) { 
	_n_entities = 0;
	_n_values = 0;
	_n_relations = 0;
}

CorrectDocument::~CorrectDocument() 
{ 
	delete [] _entities;
	delete [] _values;
	delete [] _relations;
}

/*void CorrectDocument::loadFromAdept(HltContentContainer *docAdept) 
{   
     string id = docAdept.idString;

	_id =Symbol(string_to_wstring(id));

     Coreference coref=docAdept.coreferences[0];
	_n_entities=coref.entities.size();

	if( _n_entities > 0 ) 
				_entities = new CorrectEntity[_n_entities];

    for (int j = 0; j < _n_entities; j++ ) {
		_entities[j].loadFromAdept(coref,coref.entities[i]);
	}

	
	_n_values=docAdept->getMentions().size();

	if (_n_values > 0)
		_values = new CorrectValue[_n_values];

	for (int j = 0; j < _n_values; j++) {
		
		_values[j].loadFromAdept(docAdept->getMentions()->get(i));
			}
	
	_n_relations=docAdept.relations.size();

	if (_n_values > 0)
		_relations = new CorrectValue[_n_relations];

	for (int j = 0; j < _n_relations; j++) {
		
		_relations[j].loadFromAdept(docAdept.relations[i]);
	}

	_n_events=docAdept.events.size();

	if (_n_events > 0)
		_events = new CorrectValue[_n_events];

	for (int j = 0; j < _n_events; j++) {
		
		_n_events[j].loadFromAdept(docAdept.events[i]);
	}


	
	_fixNestedNames();

}*/


void CorrectDocument::loadFromSexp(Sexp *docSexp) 
{

	Sexp* docNameSexp = docSexp->getFirstChild();
	
	if (!docNameSexp->isAtom())
		throw UnexpectedInputException("CorrectDocument::loadFromSexp()",
									   "didn't find expected docname in correctAnswerSexp");

	_id = docNameSexp->getValue();

	int i;
	for (i = 1; i < docSexp->getNumChildren(); i++) {
		Sexp *currSexp = docSexp->getNthChild(i);
		Sexp *labelSexp = currSexp->getFirstChild();

		if (!labelSexp->isAtom())
			throw UnexpectedInputException("CorrectDocument::loadFromSexp()",
										   "didn't find expected object type label in correctAnswerSexp");
		
		if (labelSexp->getValue() == Symbol(L"Entities")) {
			_n_entities = currSexp->getNumChildren() - 1;

			if( _n_entities > 0 ) 
				_entities = new CorrectEntity[_n_entities];
				
			for (int j = 0; j < _n_entities; j++ ) {
				Sexp* entitySexp = currSexp->getNthChild(j + 1);
				_entities[j].loadFromSexp(entitySexp);
			}
		}
		if (labelSexp->getValue() == Symbol(L"Values")) {
			_n_values = currSexp->getNumChildren() - 1;

			if (_n_values > 0)
				_values = new CorrectValue[_n_values];

			for (int j = 0; j < _n_values; j++) {
				Sexp* valueSexp = currSexp->getNthChild(j + 1);
				_values[j].loadFromSexp(valueSexp);
			}
		}
		if (labelSexp->getValue() == Symbol(L"Relations")) {
			_n_relations = currSexp->getNumChildren() - 1;

			if ( _n_relations > 0 )
				_relations = new CorrectRelation[_n_relations];

			for (int j = 0; j < _n_relations; j++ ) {
				Sexp *relationSexp = currSexp->getNthChild(j + 1);
				_relations[j].loadFromSexp(relationSexp);
			}
		}
		if (labelSexp->getValue() == Symbol(L"Events")) {
			_n_events = currSexp->getNumChildren() - 1;

			if ( _n_events > 0 )
				_events = new CorrectEvent[_n_events];

			for (int j = 0; j < _n_events; j++ ) {
				Sexp *eventSexp = currSexp->getNthChild(j + 1);
				_events[j].loadFromSexp(eventSexp);
			}
		}
	}
	
	_fixNestedNames();

}


void CorrectDocument::_fixNestedNames()
{
	int i, j;
	for (i = 0; i < _n_entities; i++ ) {
		CorrectEntity* entity = getEntity(i);
		for (j = 0; j < entity->getNMentions(); j++) {
			CorrectMention* correctMention = entity->getMention(j);
			if (correctMention->getMentionType() != Mention::NAME) continue;
			if (hasNameMentionNestedInside(correctMention)) 
				correctMention->setMentionType(Mention::DESC);
		}
	}
}

bool CorrectDocument::hasNameMentionNestedInside(CorrectMention *mention1)
{
	int i, j;
	for (i = 0; i < _n_entities; i++ ) {
		CorrectEntity* entity = getEntity(i);

		// if it's an EVENT, just skip over it
		if (entity->getEntityType() == NULL) continue; 

		for (j = 0; j < entity->getNMentions(); j++) {
			CorrectMention* mention2 = entity->getMention(j);
			if (!mention2->isName() || mention1 == mention2 ) continue;
			if (mention2->getHeadStartOffset() >= mention1->getHeadStartOffset() &&
				mention2->getHeadEndOffset() <= mention1->getHeadEndOffset())
				return true;
		}
	}
	return false;
}


bool CorrectDocument::isNestedInsideNameMention(CorrectMention *mention1)
{
	int i, j;
	for (i = 0; i < _n_entities; i++ ) {
		CorrectEntity* entity = getEntity(i);

		// if it's an EVENT, just skip over it
		if (entity->getEntityType() == NULL) continue; 

		for (j = 0; j < entity->getNMentions(); j++) {
			CorrectMention* mention2 = entity->getMention(j);
			if (!mention2->isName() || mention1 == mention2 ) continue;
			if (mention1->getStartOffset() >= mention2->getHeadStartOffset() &&
				mention1->getEndOffset() <= mention2->getHeadEndOffset())
				return true;
		}
	}
	return false;
}


CorrectMention * CorrectDocument::getCorrectMentionFromNameSpan(NameSpan *nameSpan)
{
	int i, j;
	for (i = 0; i < _n_entities; i++ ) {
		CorrectEntity* entity = getEntity(i);
		// if it's an EVENT, just skip over it
		if (entity->getEntityType() == NULL) continue; 

		for (j = 0; j < entity->getNMentions(); j++) {
			CorrectMention* mention = entity->getMention(j);
			if (mention->getNameSpan() == nameSpan)
				return mention;
		}
	}

	throw InternalInconsistencyException("CorrectDocument::getCorrectMentionFromNameSpan",
									     "could not find a mention with requested NameSpan");
	return 0;
}

CorrectMention * CorrectDocument::getCorrectMentionFromMentionID(Mention::UID id)
{
	int i, j, k;
	for (i = 0; i < _n_entities; i++ ) {
		CorrectEntity* correctEntity = getEntity(i);

		// if it's an EVENT, just skip over it
		if (correctEntity->getEntityType() == NULL) continue; 

		for (j = 0; j < correctEntity->getNMentions(); j++) {
			CorrectMention* correctMention = correctEntity->getMention(j);
			for (k = 0; k < correctMention->getNSystemMentionIds(); k++) {
				if (correctMention->getSystemMentionId(k) == id) {
					//std::cout << "Getting CorrectMention for " << mention->node->toDebugString(0).c_str() << "\n";
					//CorrectEntity *ce = getCorrectEntityFromCorrectMention(correctMention);
					//std::cout << ce->getNMentions() << "\n";
					return correctMention;
				}
			}
		}
	}
	
	return NULL;
}

CorrectValue * CorrectDocument::getCorrectValueFromValueMentionID(ValueMention::UID id)
{
	for (int i = 0; i < _n_values; i++ ) {
		CorrectValue* correctValue = getValue(i);
		if (correctValue->getSystemValueMentionID() == id)
			return correctValue;
	}	
	return NULL;
}

CorrectEntity * CorrectDocument::getCorrectEntityFromCorrectMention(CorrectMention *cm)
{
	int i, j;
	for (i = 0; i < _n_entities; i++ ) {
		CorrectEntity* correctEntity = getEntity(i);

		// if it's an EVENT, just skip over it
		if (correctEntity->getEntityType() == NULL) continue; 

		for (j = 0; j < correctEntity->getNMentions(); j++) {
			CorrectMention* correctMention = correctEntity->getMention(j);
			if (correctMention == cm)
				return correctEntity;
		}
	}
	throw InternalInconsistencyException("CorrectDocument::getCorrectEntityFromCorrectMention",
									     "could not find a CorrectEntity with requested CorrectMention");
	

	return NULL;

}

CorrectEntity * CorrectDocument::getCorrectEntityFromEntityID(int id) 
{
	int i;
	for (i = 0; i < _n_entities; i++) {
		CorrectEntity* correctEntity = getEntity(i);
		
		// if it's an EVENT, just skip over it
		if (correctEntity->getEntityType() == NULL) continue; 

		if (correctEntity->getSystemEntityID() == id) 
			return correctEntity;
	}
	return NULL;
}
