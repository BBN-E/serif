// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include

#include "Generic/CASerif/correctanswers/CorrectEntity.h"
#include "Generic/CASerif/correctanswers/CASymbolicConstants.h"
#include "common/Sexp.h"
#include "common/UnexpectedInputException.h"
#include "common/SessionLogger.h"


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
CorrectEntity::CorrectEntity() { 
	_system_entity_id = -1;
}

CorrectEntity::~CorrectEntity() 
{
	delete [] _mentions;
	delete _entityType;
}

void CorrectEntity::loadFromSexp(Sexp *entitySexp)
{
	int num_children = entitySexp->getNumChildren();
	if (num_children < 5 )
		throw UnexpectedInputException("CorrectEntity::loadFromSexp()",
									   "entitySexp doesn't have at least 5 children");

	Sexp *typeSexp = entitySexp->getNthChild(0);
	Sexp *subtypeSexp = entitySexp->getNthChild(1);
	Sexp *classSexp = entitySexp->getNthChild(2);
	Sexp *groupFnSexp = entitySexp->getNthChild(3);
	Sexp *entityIDSexp = entitySexp->getNthChild(4);

	if (!typeSexp->isAtom() || !classSexp->isAtom() || 
		!groupFnSexp->isAtom() || !subtypeSexp->isAtom())
		throw UnexpectedInputException("CorrectEntity::loadFromSexp()",
									   "Didn't find entity atoms in correctAnswerSexp");


	_annotationEntityID = entityIDSexp->getValue();

	if (_isEntityType(typeSexp->getValue())) {
		_entityType = new EntityType(typeSexp->getValue());
	} else {
		char message[500];
		sprintf(message, "Invalid entity type: %s", typeSexp->getValue().to_debug_string());
		throw UnexpectedInputException("CorrectEntity::loadFromSexp()", message);
	}
	_symbolicType = typeSexp->getValue();
	_symbolicSubtype = subtypeSexp->getValue();
	_symbolicClass = classSexp->getValue();

	_entitySubtype = EntitySubtype::getUndetType();

	try {
		EntitySubtype eSubtype(_entityType->getName(), _symbolicSubtype);
		_entitySubtype = eSubtype;
	} catch (UnrecoverableException &e) {
		char message[500];
		strncpy(message, e.getSource(), 100);
		strcat(message, ": ");
		strncat(message, e.getMessage(), 200);
		SessionLogger::logger->beginWarning();
		(*SessionLogger::logger) << message << "\n";
	}

	if (classSexp->getValue() == CASymbolicConstants::GEN_SYM)
		_is_generic = true;
	else 
		_is_generic = false;

	if (groupFnSexp->getValue() == CASymbolicConstants::TRUE_SYM)
		_is_group_fn = true;
	else 
		_is_group_fn = false;

	_n_mentions = num_children - 5;
	int i;
	
	if (_n_mentions > 0) 
		_mentions = new CorrectMention[_n_mentions];

	for (i = 0; i < _n_mentions; i++ ) {
		Sexp* mentionSexp = entitySexp->getNthChild(i + 5);
		_mentions[i].loadFromSexp(mentionSexp);
	}

	// this is only necessary for ACE 2004, not ACE 2005
	normalizePremods();
}


/*void CorrectEntity::loadFromAdept(Corefernce *coref, Entity *entityAdept)
{
	

	string typeAdept = entityAdept.entityType.type;
	if(typeAdept.compare("TTL")==0 && _isEntityType(Symbol(L"PER"))){
		_entityType = new EntityType(Symbol(L"PER"));
		_symbolicType = Symbol(L"PER");
		}
	else if(typeAdept.compare("TTL")!=0 && _isEntityType(Symbol(LtypeAdept))){
	   _entityType = new EntityType(Symbol(LtypeAdept));
	   _symbolicType=Symbol(LtypeAdept);
	}
	else {
		char message[500];
		sprintf(message, "Invalid entity type: %s", typeAdept);
		throw UnexpectedInputException("CorrectEntity::loadFromAdept()", message);
	} 


    long id = entityAdept.entityId;

	std::string idString;
    std::stringstream strstream;
    strstream << id;
    strstream >> idString;

	_annotationEntityID = Symbol(string_to_wstring(idString));

	
	
	_symbolicSubtype = Symbol(L"NONE");
	_symbolicClass = Symbol(L"NONE");

	

	
		
		_entitySubtype = Symbol(L"NONE");

		_is_generic = false;
        _is_group_fn = false;

   	  std::vector<EntityMention> entity_mentions= coref.resolvedEntityMentions;
	  std::vector<EntityMention> current_mentions;
	  
     for(unsigned int j=0;j<entity_mentions.size();j++){
	   std::map<long, double> all_ids=entity_mentions[j].entityIdDistribution;
	   
	   map<long,double>::iterator it=all_ids.begin();
      for(;it!=all_ids.end();++it){
          

	     if(entity_id==it->first){  //find the resolved mention
		   _n_mentions+=1;
	       cout<<"key:"<<it->first <<"value:"<<it->second<<"\n";
			current_mentions.push_back(entity_mentions[j]);
			break;
	   }
	 }
	}

	int i;
	
	if (_n_mentions > 0) 
		_mentions = new CorrectMention[_n_mentions];

	for (i = 0; i < _n_mentions; i++ ) {
		_mentions[i].loadFromAdept(current_mentions[i]);
	}

	// this is only necessary for ACE 2004, not ACE 2005
	normalizePremods();
}*/
void CorrectEntity::normalizePremods() {
	for (int i = 0; i < _n_mentions; i++ ) {
		if (!_mentions[i].isUndifferentiatedPremod())
			continue;
		EDTOffset s = _mentions[i].getStartOffset();
		EDTOffset e = _mentions[i].getEndOffset();
		for (int j = 0; j < _n_mentions; j++ ) {
			if (i == j)
				continue;
			// so: if the extent of another mention of the same entity
			// contains this one, it's probably a title type thing --
			// mark it as a desc
			if (_mentions[j].getStartOffset() <= s && _mentions[j].getEndOffset() >= e) {
				_mentions[i].setMentionType(Mention::DESC);
			}
		}
		// otherwise, let's say it's a name
		if (_mentions[i].getMentionType() == Mention::NONE)
			_mentions[i].setMentionType(Mention::NAME);
	}
}

bool CorrectEntity::_isEntityType(Symbol type) {
	return EntityType::isValidEntityType(type);
}
