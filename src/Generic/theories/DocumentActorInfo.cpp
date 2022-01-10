// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/DocumentActorInfo.h"
#include "Generic/actors/Identifiers.h"

DocumentCountryActor::DocumentCountryActor(ActorId actor_id) : _actor_id (actor_id)
{ }

DocumentCountryActor::~DocumentCountryActor() { }

void DocumentCountryActor::updateObjectIDTable() const {
	throw InternalInconsistencyException("DocumentCountryActor::updateObjectIDTable",
		"DocumentCountryActor does not currently have state file support");
}
void DocumentCountryActor::saveState(StateSaver * stateSaver) const {
	throw InternalInconsistencyException("DocumentCountryActor::saveState",
		"DocumentCountryActor does not currently have state file support");
}
void DocumentCountryActor::resolvePointers(StateLoader * stateLoader) {
	throw InternalInconsistencyException("DocumentCountryActor::resolvePointers",
		"DocumentCountryActor does not currently have state file support");
}

void DocumentCountryActor::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	using namespace SerifXML;

	elem.setAttribute<size_t>(X_actor_uid, _actor_id.getId());
	elem.setAttribute<Symbol>(X_actor_db_name, _actor_id.getDbName());
}

DocumentCountryActor::DocumentCountryActor(SerifXML::XMLTheoryElement elem) {
	using namespace SerifXML;

	elem.loadId(this); 
	_actor_id = ActorId(static_cast<ActorId::id_type>(elem.getAttribute<size_t>(X_actor_uid)), elem.getAttribute<Symbol>(X_actor_db_name));
}


DocumentActorInfo::DocumentActorInfo() : _documentCountryActor(0) { }
DocumentActorInfo::~DocumentActorInfo() { 
	delete _documentCountryActor;
}

void DocumentActorInfo::updateObjectIDTable() const {
	throw InternalInconsistencyException("DocumentActorInfo::updateObjectIDTable",
		"DocumentActorInfo does not currently have state file support");
}
void DocumentActorInfo::saveState(StateSaver * stateSaver) const {
	throw InternalInconsistencyException("DocumentActorInfo::saveState",
		"DocumentActorInfo does not currently have state file support");
}
void DocumentActorInfo::resolvePointers(StateLoader * stateLoader) {
	throw InternalInconsistencyException("DocumentActorInfo::resolvePointers",
		"DocumentActorInfo does not currently have state file support");
}

void DocumentActorInfo::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	using namespace SerifXML;

	if (_documentCountryActor)
		elem.saveChildTheory(X_DefaultCountryActor, _documentCountryActor);
}

DocumentActorInfo::DocumentActorInfo(SerifXML::XMLTheoryElement elem) 
: _documentCountryActor(0) 
{
	using namespace SerifXML;

	elem.loadId(this); 
	XMLTheoryElement documentCountryActorElem = elem.getOptionalUniqueChildElementByTagName(X_DefaultCountryActor);
	if (!documentCountryActorElem.isNull())
		_documentCountryActor = _new DocumentCountryActor(documentCountryActorElem);
}

void DocumentActorInfo::takeDocumentCountryActor(DocumentCountryActor *documentCountryActor) {
	_documentCountryActor = documentCountryActor;
}

const DocumentCountryActor *DocumentActorInfo::getDocumentCountryActor() const {
	return _documentCountryActor;
}	
