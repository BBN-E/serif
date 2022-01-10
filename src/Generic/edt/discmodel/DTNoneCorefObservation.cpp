// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/MentionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/edt/discmodel/DTNoneCorefObservation.h"
#include "Generic/edt/discmodel/DocumentMentionInformationMapper.h"

const Symbol DTNoneCorefObservation::_className(L"coref");
const Symbol DTNoneCorefObservation::_linkSym(L":LINK");
const Symbol DTNoneCorefObservation::_noLinkSym(L":NOLINK");

/* Observation that holds information that is only related to the mention being cofererenced
 * DTCorefObservation will hold the rest of the information relevant for the entity.
 * DTNoneCorefObservation is used in Ranking where one of the options is not to link.
 * Features can be derived from the mention by itself.
*/
DTObservation *DTNoneCorefObservation::makeCopy() {
	DTNoneCorefObservation *copy = _new DTNoneCorefObservation();
	copyTo(copy);
	return copy;
}

void DTNoneCorefObservation::resetForNewDocument(const EntitySet *entSet){
	_entitySet = entSet;
}

// TODO: add _documentMentionSets to the function
void DTNoneCorefObservation::resetForNewDocument(const EntitySet *entSet, const DocumentMentionInformationMapper *infoMap/*GrowableArray<const MentionSet *> *documentMentionSets*/){
	_entitySet = entSet;
	setDocumentInformationMapper(infoMap);
//	_documentMentionSets = documentMentionSets;
}


void DTNoneCorefObservation::resetForNewSentence(const MentionSet *mentionSet)
{
	_mentionSet = mentionSet;
}


void DTNoneCorefObservation::populate(MentionUID mentionId) 
{
	_ment_id = mentionId;
	_ment = _mentionSet->getMention(mentionId);
}

										   
void DTNoneCorefObservation::copyTo(DTNoneCorefObservation *other){
	other->_ment_id = _ment_id;
	other->_ment = _mentionSet->getMention(_ment_id);
	other->_entitySet = _entitySet;
	other->setDocumentInformationMapper(_infoMap);
}
