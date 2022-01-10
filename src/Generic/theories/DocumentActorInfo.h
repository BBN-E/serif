// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DOCUMENT_ACTOR_INFO_H
#define DOCUMENT_ACTOR_INFO_H

#include "Generic/theories/Theory.h"
#include "Generic/actors/Identifiers.h"

class DocumentCountryActor : public Theory {
public:

	DocumentCountryActor(ActorId actor_id);
	~DocumentCountryActor();

	// State file serialization (not currently implemented)
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	virtual void resolvePointers(StateLoader * stateLoader);

	// SerifXML serialization/deserialization:
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual const wchar_t* XMLIdentifierPrefix() const { return L"documentcountryactor"; }
	explicit DocumentCountryActor(SerifXML::XMLTheoryElement elem);

private:
	ActorId _actor_id;

};

class DocumentActorInfo : public Theory {
	
public:
	DocumentActorInfo();
	~DocumentActorInfo();

	// State file serialization (not currently implemented)
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	virtual void resolvePointers(StateLoader * stateLoader);

	// SerifXML serialization/deserialization:
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual const wchar_t* XMLIdentifierPrefix() const { return L"documentactorinfo"; }
	explicit DocumentActorInfo(SerifXML::XMLTheoryElement elem);

	void takeDocumentCountryActor(DocumentCountryActor *documentCountryActor);
	const DocumentCountryActor *getDocumentCountryActor() const;


private:
	DocumentCountryActor *_documentCountryActor;
};

#endif
