// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.


#include "Generic/common/leak_detection.h"

#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/UTCoref.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"

#include <boost/foreach.hpp>

UTCoref::UTCoref() {

}

UTCoref::~UTCoref() {
   while (entities.length() > 0) {
	  delete entities.removeLast();
   }
}

void UTCoref::addEntity(GrowableArray<const SynNode*> *entity) {
   entities.add(entity);
}

/* if leftMention is already in an entity, add rightMention to that
 *  entity.  Otherwise create a new entity just for the two of them.
 */
void UTCoref::addLink(const SynNode &leftMention, const SynNode &rightMention) {
   if(getEntityFor(rightMention)) {
	  throw UnexpectedInputException("UTCoref.cpp::addLink",
                                     "Right mention was already linked");
   }

   GrowableArray<const SynNode*> *entity;
   if ((entity = getEntityFor(leftMention))) {
	  entity->add(&rightMention);
   }
   else {
	  entity = _new GrowableArray<const SynNode *>();
	  entity->add(&leftMention);
	  entity->add(&rightMention);

	  addEntity(entity);
   }
}

/* Look up which entity mention is in.  Return NULL if there is no such entity */
GrowableArray<const SynNode*> *UTCoref::getEntityFor(const SynNode &mention) const {
   for (int i = 0 ; i < entities.length() ; i++) {
	  for (int j = 0 ; j < entities[i]->length() ; j++) {
		 if ((*entities[i])[j] == &mention) {
			return entities[i];
		 }
	  }
   }
   return NULL;
}

const GrowableArray< GrowableArray<const SynNode*> * >* UTCoref::getEntities() {
   return &entities;
}

/* are these two nodes coreferent? */
bool UTCoref::areLinked(const SynNode &leftMention, const SynNode &rightMention) const {
   GrowableArray<const SynNode*> *leftEntity = getEntityFor(leftMention);
   GrowableArray<const SynNode*> *rightEntity = getEntityFor(rightMention);

   return leftEntity != NULL && leftEntity == rightEntity;
}

UTCoref::UTCoref(StateLoader *stateLoader) {
   int id = stateLoader->beginList(L"UTCoref");
   stateLoader->getObjectPointerTable().addPointer(id, this);

   int n_entities = stateLoader->loadInteger();
   for (int i = 0 ; i < n_entities ; i++) {
	  stateLoader->beginList(L"UTCoref::entity");
	  GrowableArray<const SynNode*> *entity = _new GrowableArray<const SynNode*>();

	  int n_mentions = stateLoader->loadInteger();
	  for (int j = 0 ; j < n_mentions ; j++) {
		 entity->add(static_cast<const SynNode *>(stateLoader->loadPointer()));
	  }
	  addEntity(entity);
	  stateLoader->endList();
   }
   stateLoader->endList();
}

void UTCoref::updateObjectIDTable() const {                                                                                     
   ObjectIDTable::addObject(this);
}

void UTCoref::saveState(StateSaver *stateSaver) const {
   stateSaver->beginList(L"UTCoref", this);
   stateSaver->saveInteger(entities.length());

   for (int i = 0 ; i < entities.length() ; i++ ) {
	  stateSaver->beginList(L"UTCoref::entity");
	  GrowableArray<const SynNode*> &entity = *entities[i];

	  stateSaver->saveInteger(entity.length());
	  for (int j = 0 ; j < entity.length() ; j++) {
		 const SynNode* mention = entity[j];
		 stateSaver->savePointer(mention);
	  }	  
	  stateSaver->endList();
   }
   stateSaver->endList();
}

void UTCoref::resolvePointers(StateLoader *stateLoader) {
   for (int i = 0 ; i < entities.length() ; i++) {
	  GrowableArray<const SynNode*> &entity = *entities[i];
	  for (int j = 0 ; j < entity.length() ; j++) {
		 entity[j] = static_cast<const SynNode *>(
			stateLoader->getObjectPointerTable().getPointer(entity[j]));
	  }
   }
}

const wchar_t* UTCoref::XMLIdentifierPrefix() const {
	return L"utcoref";
}

void UTCoref::saveXML(SerifXML::XMLTheoryElement utcorefElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("UTCoref::saveXML", "Expected context to be NULL");
	SessionLogger::warn("unimplemented_method") << "saveXML method not filled in yet!" << std::endl; // [XX] FILL THIS IN [XX]
}

UTCoref::UTCoref(SerifXML::XMLTheoryElement elem)
{
	using namespace SerifXML;
	SessionLogger::warn("unimplemented_method") << "NOT IMPLEMENTED" << std::endl; // [XX] FILL THIS IN [XX]
}
