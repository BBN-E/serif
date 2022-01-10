// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/results/ResultCollector.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/CBRNEWordConstants.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/OStringStream.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/SexpReader.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/RelationSet.h"
#include "Generic/theories/Relation.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/EventSet.h"
#include "Generic/theories/Event.h"
#include "Generic/Cbrne/results/CBRNEResultCollector.h"
#include <math.h>
#include <iostream>
//#include "Generic/common/StringTransliterator.h"
#include "Generic/linuxPort/serif_port.h"

using namespace std;

CBRNEResultCollector::CBRNEResultCollector() : _docTheory(0)
{
	loadRelationTypeNameMap();
	loadRelationFilter();
}

CBRNEResultCollector::~CBRNEResultCollector() {
	delete _relationTypeMap;
	delete _relationFilter;
}

void CBRNEResultCollector::loadRelationTypeNameMap() {
	_relationTypeMap = NULL;
	std::string filename = ParamReader::getParam("relation_type_map");
	if (filename.empty())
		return;

	Symbol key[1], value[1];
	// read in file
	//int i;
	SexpReader reader(filename.c_str());
	_relationTypeMap = _new SymArrayMap(300);

	while(reader.hasMoreTokens()) {
		reader.getLeftParen();
		//UTF8Token relationValue, relationKey, slot1Key, slot2Key;
		value[0] = reader.getWord().symValue();
        // one key only, for now
		//reader.getLeftParen();
		key[0] = reader.getWord().symValue();
		//key[1] = reader.getWord().symValue();
		//key[2] = reader.getWord().symValue();
		reader.getRightParen();

		if(value[0] != SymbolConstants::nullSymbol) {

			SymArray *arrKey = _new SymArray(key, 1);
			SymArray *arrValue = _new SymArray(value, 1);

			if((*_relationTypeMap)[arrKey]== NULL)
				(*_relationTypeMap)[arrKey] = arrValue;
			else {
				SymArray *oldValue = (*_relationTypeMap)[arrKey];
				(*_relationTypeMap)[arrKey] = arrValue;
				delete oldValue;
			}
		}

    }

	reader.closeFile();
}

void CBRNEResultCollector::loadRelationFilter() {
	_relationFilter = NULL;	
	//std::string filename = ParamReader::getParam("relation_filter");
	//if (filename.empty())
	//	return;
	//Symbol value[3];
	//// read in file
	////int i;
	//SexpReader reader(filename);
	//_relationTypeMap = _new SymArraySet(300);

	//while(reader.hasMoreTokens()) {
	//	reader.getLeftParen();
	//	value[0] = reader.getWord().symValue();
	//	value[1] = reader.getWord().symValue();
	//	value[2] = reader.getWord().symValue();
	//	reader.getRightParen();
	//
	//	SymArray *arrValue = _new SymArray(value, 3);

	//	if((*_relationFilter)[arrKey]== NULL)
	//		(*_relationTypeMap)[arrKey] = arrValue;
	//	else {
	//		SymArray *oldValue = (*_relationTypeMap)[arrKey];
	//		(*_relationTypeMap)[arrKey] = arrValue;
	//		delete oldValue;
	//	}
 //
 //   }

	//reader.closeFile();
}

void CBRNEResultCollector::loadDocTheory(DocTheory *docTheory) {
	_docTheory = docTheory;
}

void CBRNEResultCollector::produceOutput(std::wstring *results) {
	OStringStream stream(*results);
	produceOutput(stream);
}


void CBRNEResultCollector::produceOutput(const wchar_t *output_dir,
                                         const wchar_t *document_filename)
{
	if(!output_dir || !document_filename)
		return;
	wstring output_file = wstring(output_dir) + LSERIF_PATH_SEP + wstring(document_filename) + L".CBRNE.xml";

	UTF8OutputStream stream;
	stream.open(output_file.c_str());
	produceOutput(stream);
	stream.close();
}

void CBRNEResultCollector::produceOutput(OutputStream &stream) {
	appendBeginDocTag(stream, L"");

	// steps:
	//1. write entities
	//2. write entity mentions
	//3. write relation mentions
	//4. write relation slots
	//5. write events/eventmentions/event slots/eventmention slots.
	//6. write sentences


	const EntitySet *entitySet = _docTheory->getEntitySet();
    int i;
	int nKnownEntities = entitySet->getNEntities();
	const int nSentences = _docTheory->getNSentences();

	// 1A. WRITE KNOWN ENTITIES
	for (i=0; i<nKnownEntities; i++) {
		Entity *thisEntity = entitySet->getEntity(i);
		if (isValidEntity(thisEntity))
			appendEntity(stream, thisEntity);
	}

	// 1B. WRITE "CONSTRUCTED" ENTITIES FROM UNLINKED MENTIONS
	int nExtraEntities = 0;
	for(i=0; i<nSentences; i++) {
		const MentionSet *mentions = _docTheory->getSentenceTheory(i)->getMentionSet();
		const EntitySet *entities = _docTheory->getEntitySet();
		int j;
		for (j=0; j<mentions->getNMentions(); j++) {
			Mention *mention = mentions->getMention(j);
			if(!isLinkedEntityMention(mention) && isValidEntityMention(mention)) {
				// currentUID is nKnownEntities + nExtraEntities
				appendExtraEntity(stream, nKnownEntities + nExtraEntities, mention, i);
//				appendMention(mention, NULL, nKnownEntities + nExtraEntities);
				nExtraEntities++;
			}
		}
	}

	// 2. WRITE ALL ENTITY MENTIONS
	nExtraEntities = 0;
	for(i=0; i<nSentences; i++) {
		const MentionSet *mentions = _docTheory->getSentenceTheory(i)->getMentionSet();
		const EntitySet *entities = _docTheory->getEntitySet();
		int j;
		for (j=0; j<mentions->getNMentions(); j++) {
			Mention *mention = mentions->getMention(j);
			if(isValidEntityMention(mention)) {
				if(!isLinkedEntityMention(mention)) {
	//				appendExtraEntity(nKnownEntities + nExtraEntities, mention);

					// nKnownEntities is the first index of an extra entity
					// because UID's are zero-based
					appendMention(stream, mention, NULL, nKnownEntities + nExtraEntities);
					nExtraEntities++;
				}
				else {
					Entity *entity = entities->getEntityByMention(mention->getUID());
					appendMention(stream, mention, entity);
				}
			}
		}
	}

	// 3A. WRITE NON-UNK RELATION MENTIONS
	const RelationSet *relationSet = _docTheory->getRelationSet();
	RelMentionUID maxRelationUID;
	for (i=0; i<relationSet->getNRelations(); i++) {
		Relation *thisRelation = relationSet->getRelation(i);
		const Relation::LinkedRelMention *iterator = thisRelation->getMentions();
		while (iterator != NULL) {
			RelMention *thisMention = iterator->relMention;
			if(isValidRelationMention(thisMention)) {
				if(maxRelationUID < thisMention->getUID())
					maxRelationUID = thisMention->getUID();
				appendRelMention(stream, thisMention, thisRelation);
			}
			iterator = iterator->next;
		}
	}

	// 3B. WRITE "UNK" RELATION MENTIONS
	int nVerbRelations = 0;
	for (i=0; i<nSentences; i++) {
		int j;
		const PropositionSet *propSet = _docTheory->getSentenceTheory(i)->getPropositionSet();
		for (j=0; j<propSet->getNPropositions(); j++) {
			const Proposition *proposition = propSet->getProposition(j);
			RelMentionUID uid(maxRelationUID.toInt() + (++nVerbRelations));
			if(isValidProposition(proposition, i))
				appendVerbRelation(stream, proposition, uid, i);
		}
	}

	// 4A. WRITE NON-UNK RELATION SLOTS
	for (i=0; i<relationSet->getNRelations(); i++) {
		const Relation *thisRelation = relationSet->getRelation(i);
		const Relation::LinkedRelMention *iterator = thisRelation->getMentions();
		while (iterator != NULL) {
			RelMention *thisMention = iterator->relMention;
			if(isValidRelationMention(thisMention)) {
		        appendSlots(stream, thisMention, thisRelation);
			}
			iterator = iterator->next;
		}
	}

	// 4B. WRITE "UNK" RELATION SLOTS
	nVerbRelations = 0;
	for (i=0; i<nSentences; i++) {
		int j;
		const PropositionSet *propSet = _docTheory->getSentenceTheory(i)->getPropositionSet();
		for (j=0; j<propSet->getNPropositions(); j++) {
			const Proposition *proposition = propSet->getProposition(j);
			RelMentionUID uid(maxRelationUID.toInt() + (++nVerbRelations));
			if(isValidProposition(proposition, i))
				appendVerbRelationSlots(stream, proposition, uid, i);
		}
	}


	// 5. WRITE EVENTS
	EventSet *events = _docTheory->getEventSet();
	int nEvents = events->getNEvents();

	// events
	for(i=0; i<nEvents; i++) {
		appendEvent(stream, events->getEvent(i));
	}

	// event mentions
	for(i=0; i<nEvents; i++) {
		Event::LinkedEventMention *lem = events->getEvent(i)->getEventMentions();
		while (lem != NULL) {
			appendEventMention(stream, lem->eventMention, events->getEvent(i));
			lem = lem->next;
		}
	}

	// event slots
	for(i=0; i<nEvents; i++) {
		appendEventSlots(stream, events->getEvent(i));
	}

	// event mention slots
	for(i=0; i<nEvents; i++) {
		Event::LinkedEventMention *lem = events->getEvent(i)->getEventMentions();
		while (lem != NULL) {
			appendEventMentionSlots(stream, lem->eventMention);
			lem = lem->next;
		}
	}



	// 6. WRITE SENTENCES
	for (i=0; i<nSentences; i++) {
		appendSentence(stream, _docTheory->getSentence(i));
	}

	appendEndDocTag(stream);
}



// the mention has been linked to an entity by serif
bool CBRNEResultCollector::isLinkedEntityMention(const Mention *mention) {
	return (_docTheory->getEntitySet()->getEntityByMention(mention->getUID()) != NULL);
}

// the mention has not been linked but has a valid entity type
bool CBRNEResultCollector::isUnlinkedTypedEntityMention(const Mention *mention) {
	return (!isLinkedEntityMention(mention) && isTypedEntityMention(mention));
}

// the mention has not been linked and has an invalid entity type
bool CBRNEResultCollector::isUnlinkedUntypedEntityMention(const Mention *mention) {
	return (!isLinkedEntityMention(mention) && !isTypedEntityMention(mention));
}

// the mention has a valid entity type
bool CBRNEResultCollector::isTypedEntityMention(const Mention *mention) {
	return mention->getEntityType().isRecognized();
}

bool CBRNEResultCollector::isValidRelationMention(const RelMention *mention) {
	//if we have not defined a map to override the standard RDC names OR a filter to
	// eliminate some names, then anything is valid
	if(!_relationTypeMap && !_relationFilter) return true;

	bool valid = false;

	// map types first if applicable, then filter if applicable
	if(_relationTypeMap) {
		Symbol type = mention->getType();
		SymArray *typeArr = _new SymArray(&type, 1);
		SymArray **searchResult = (*_relationTypeMap).get(typeArr);
		delete typeArr;

		const Mention *left, *right;
		left = fixAppositiveCase(mention->getLeftMention());
		right = fixAppositiveCase(mention->getRightMention());
		valid = (searchResult != NULL) && isValidEntityMention(left) && isValidEntityMention(right);
	}

	if(_relationFilter) {
		//todo
	}
	return valid;
}

// determines whether "mention" is part of printed output
// for unlinked mentions: this function is also used to determine whether to output a new
// entity for this mention
bool CBRNEResultCollector::isValidEntityMention(const Mention *mention) {
	if(isLinkedEntityMention(mention))
		return (isValidEntity(
				   _docTheory->getEntitySet()->getEntityByMention(mention->getUID())) &&
		        mention->getMentionType() != Mention::LIST &&
				mention->getMentionType() != Mention::NONE &&
				//mention->getMentionType() != Mention::PART &&   //CHANGE: PART's now get treated as DESC's
				mention->getMentionType() != Mention::APPO);
	else
		return (mention->getMentionType() != Mention::LIST &&
				mention->getMentionType() != Mention::NONE &&
				//mention->getMentionType() != Mention::PART &&
				mention->getMentionType() != Mention::APPO &&
				mention->getMentionType() != Mention::PRON);
}

// determines whether "entity" is valid for output: i.e., whether it has at least 1
// name or descriptor mention associated with it
bool CBRNEResultCollector::isValidEntity(const Entity *entity) {
	int nameCount = 0, descCount = 0;
	int i;
	for(i=0; i<entity->getNMentions(); i++) {
		Mention *mention = _docTheory->getEntitySet()->getMention(entity->getMention(i));
		Mention::Type mentionType = fixPartitiveCase(mention->getMentionType());
		if(mentionType == Mention::NAME) nameCount++;
		if(mentionType == Mention::DESC) descCount++;
	}
	return (nameCount || descCount);
}

Mention::Type CBRNEResultCollector::fixPartitiveCase(Mention::Type type) {
	return (type == Mention::PART) ? Mention::DESC : type;
}

// to be valid, a prop must be VERB_PRED and have both a subject and either (object or indirect-object), such that
//  NEITHER is an unknown-entity mention
bool CBRNEResultCollector::isValidProposition(const Proposition *proposition, const int sentence_num) {
	const MentionSet *mentions = _docTheory->getSentenceTheory(sentence_num)->getMentionSet();
	if(proposition->getPredType() == Proposition::VERB_PRED) {
		const Mention *sub=0, *obj=0, *iobj=0, *prep=0;
		int i;
		for (i=0; i<proposition->getNArgs(); i++) {
			const Argument *arg = proposition->getArg(i);
			if(arg->getType() == Argument::MENTION_ARG) {
				const Mention *mention = arg->getMention(mentions);
				if(isValidEntityMention(mention) && isTypedEntityMention(mention))  {
					if(arg->getRoleSym() == Argument::SUB_ROLE)
						sub = mention;
					if(arg->getRoleSym() == Argument::OBJ_ROLE)
						obj = mention;
					if(arg->getRoleSym() == Argument::IOBJ_ROLE)
						iobj = mention;
					if(CBRNEWordConstants::isCBRNEReportedPreposition(arg->getRoleSym()))
						prep =  mention;
				}
			}
		}
		if(sub && (obj || iobj))
			return true;
		else return false;
	}
	else return false;
}

void CBRNEResultCollector::appendEvent(OutputStream &stream, Event *e) {

	stream << L"<EVENT ";

	stream << L"UID=\"";
	stream << obtainEventUID(e, e->getID());
	stream << L"\" ";

	stream << L"eventType=\"";
	stream << (e->getType()).to_string();
	stream << L"\" ";

	stream << L"/>\n";

}

void CBRNEResultCollector::appendEventMention(OutputStream &stream, EventMention *em, Event *e) {

	const SynNode *anchor = em->getAnchorNode()->getHeadPreterm();

	int startTok = anchor->getStartToken();
	int endTok = anchor->getEndToken();

	TokenSequence *ts = _docTheory->getSentenceTheory(em->getSentenceNumber())->getTokenSequence();
	EDTOffset startOffset = ts->getToken(startTok)->getStartEDTOffset();
	EDTOffset endOffset = ts->getToken(endTok)->getEndEDTOffset();

	stream << L"<EVENT_MENTION ";

	stream << L"UID=\"";
	stream << obtainEventMentionUID(em, em->getUID());
	stream << L"\" ";

	stream << L"eventUID=\"";
	stream << obtainEventUID(e, e->getID());
	stream << L"\" ";

	stream << L"headWord=\"";
	stream << anchor->getHeadWord().to_string();
	stream << L"\" ";

	stream << L"startOffset=\"";
	stream << startOffset;
	stream << L"\" ";

	stream << L"stringLength=\"";
	stream << (endOffset.value()-startOffset.value());
	stream << L"\" ";

	stream << L"/>\n";

}

void CBRNEResultCollector::appendEventSlots(OutputStream &stream, Event *e) {
	int i, nSlots;

	nSlots = e->getNumConsolidatedEERelations();

	for(i=0; i<nSlots; i++) {
		EventEntityRelation *eer = e->getConsolidatedEERelation(i);

		stream << L"<EVENT_SLOT ";

		stream << L"eventUID=\"";
		stream << obtainEventUID(e, e->getID());
		stream << L"\" ";

		stream << L"entityUID=\"ent";
		stream << eer->getEntityID();
		stream << L"\" ";

		stream << L"slotType=\"";
		stream << eer->getType().to_string();
		stream << L"\" ";

		stream << L"/>\n";
	}
}

void CBRNEResultCollector::appendEventMentionSlots(OutputStream &stream, EventMention *em) {
	int i, nSlots;

	nSlots = em->getNArgs();

	for(i=0; i<nSlots; i++) {

		stream << L"<EVENT_MENTION_SLOT ";

		stream << L"eventMentionUID=\"";
		stream << obtainEventMentionUID(em, em->getUID());
		stream << L"\" ";

		stream << L"entityMentionUID=\"";
		stream << obtainEntityMentionUID(em->getNthArgMention(i), em->getNthArgMention(i)->getUID());
		stream << L"\" ";

		stream << L"slotType=\"";
		stream << em->getNthArgRole(i).to_string();
		stream << L"\" ";

		stream << L"/>\n";
	}

}


void CBRNEResultCollector::appendEntity(OutputStream &stream, const Entity *entity) {

	stream << L"<ENTITY ";

	stream << L"UID=\"ent";
	stream << entity->getID();
	stream << L"\" ";

	stream << L"entityType=\"";
	if (!entity->type.isRecognized())
		stream << "UNK";
	else
		stream << entity->type.getName().to_string();
	stream << L"\" ";

	stream << L"canonicalString=\"";
	// find canonical string
	int i;
	Mention *cMention = NULL;  //canonical mention
	wchar_t cString[1024] = L"";
	for(i=0; i<entity->getNMentions(); i++) {
		Mention *thisMention = _docTheory->getEntitySet()->getMention(entity->getMention(i));
		const TokenSequence *tokens = _docTheory->getSentenceTheory(thisMention->getSentenceNumber())->getTokenSequence();
		const size_t MAX_STRLEN = 1024;
		wchar_t thisString[MAX_STRLEN] = L"";

		int startTok = thisMention->getNode()->getStartToken();
		int endTok = thisMention->getNode()->getEndToken();

		int j;
		if(wcslen(thisString) + 1 + wcslen(tokens->getToken(startTok)->getSymbol().to_string()) <= MAX_STRLEN)
			wcscat(thisString, tokens->getToken(startTok)->getSymbol().to_string());
		for(j=startTok+1; j<=endTok; j++) {
			if(wcslen(thisString) + 1 + 1 <= MAX_STRLEN)
				wcscat(thisString, L" ");
			if(wcslen(thisString) + 1 + wcslen(tokens->getToken(j)->getSymbol().to_string()) <= MAX_STRLEN)
				wcscat(thisString, tokens->getToken(j)->getSymbol().to_string());
		}

		Mention::Type thisMentionType = fixPartitiveCase(thisMention->getMentionType());

		if(!cMention  ||
			(thisMentionType == Mention::NAME &&
				(cMention->getMentionType() != Mention::NAME || wcslen(cString) < wcslen(thisString))) ||
			(thisMentionType == Mention::DESC &&
				(cMention->getMentionType() != Mention::NAME && wcslen(cString) < wcslen(thisString))))
		{
			cMention = thisMention;
			wcscpy(cString, thisString);
		}
	}

	appendEscaped(stream, cString);
	stream << L"\" ";

	stream << L"/>\n";
}

void CBRNEResultCollector::appendExtraEntity(OutputStream &stream, int uid, Mention *mention, int sentence_num) {
	const TokenSequence *tokens = _docTheory->getSentenceTheory(sentence_num)->getTokenSequence();
	// find canonical string
	const size_t MAX_STRLEN = 1024;
	wchar_t cString[MAX_STRLEN] = L"";

	int startTok = mention->getNode()->getStartToken();
	int endTok = mention->getNode()->getEndToken();

	//get canonical string, which is simply the text of the mention
	int j;
	if(wcslen(cString) + 1 + wcslen(tokens->getToken(startTok)->getSymbol().to_string()) <= MAX_STRLEN)
		wcscat(cString, tokens->getToken(startTok)->getSymbol().to_string());
	for(j=startTok+1; j<=endTok; j++) {
		if(wcslen(cString) + 1 + 1 <= MAX_STRLEN)
			wcscat(cString, L" ");
		if(wcslen(cString) + 1 + wcslen(tokens->getToken(j)->getSymbol().to_string()) <= MAX_STRLEN)
			wcscat(cString, tokens->getToken(j)->getSymbol().to_string());
	}
	// find entity type
	Symbol type;
	if(isUnlinkedUntypedEntityMention(mention))
        type = Symbol(L"UNK");
	else
		type = mention->getEntityType().getName();

	//print XML
	stream << L"<ENTITY ";

	stream << L"UID=\"ent";
	stream << uid;
	stream << L"\" ";

	stream << L"entityType=\"";
	stream << type.to_string();
	stream << L"\" ";

	stream << L"canonicalString=\"";
	appendEscaped(stream, cString);
	stream << L"\" ";

	stream << L"/>\n";
}

int CBRNEResultCollector::obtainRelMentionUID(const RelMention *mention, RelMentionUID suggestedUID) {
	if(!suggestedUID.isValid())
		// this is an error!
		return -1;
	else return (suggestedUID.toInt() * 5) + 1;
}

int CBRNEResultCollector::obtainEntityMentionUID(const Mention *mention, MentionUID suggestedUID) {
	if(!suggestedUID.isValid())
		// this is an error!
		return -1;
	else return (suggestedUID.toInt() * 5) + 2;
}

int CBRNEResultCollector::obtainSentenceUID(int suggestedUID) {
	if(suggestedUID == -1)
		// this is an error!
		return -1;
	else return (suggestedUID * 5) + 3;
}

// NOTE: this is not currently used
int CBRNEResultCollector::obtainSlotUID(int suggestedUID) {
	if(suggestedUID == -1)
		// this is an error!
		return -1;
	// Do not worry about this "4", these UID's never get created so this slot is still available.
	else return (suggestedUID * 5) + 4;
}

int CBRNEResultCollector::obtainEventMentionUID(const EventMention *em, EventMentionUID suggestedUID) {
	if(!suggestedUID.isValid())
		// this is an error!
		return -1;
	else return (suggestedUID.toInt() * 10) + 4;

}

int CBRNEResultCollector::obtainEventUID(const Event *e, int suggestedUID) {
	if(suggestedUID == -1)
		// this is an error!
		return -1;
	else return (suggestedUID * 10) + 9;

}



void CBRNEResultCollector::appendMention(OutputStream &stream,
										const Mention *mention,
										const Entity *entity,
										int uid)
{
	const SynNode *node = mention->getNode();
    int sentence_num = mention->getSentenceNumber();
	const TokenSequence *tokens = _docTheory->getSentenceTheory(sentence_num)->getTokenSequence();
	int startTok = node->getStartToken();
	int endTok   = node->getEndToken();
	EDTOffset start    = tokens->getToken(startTok)->getStartEDTOffset();
	EDTOffset end      = tokens->getToken(endTok)->getEndEDTOffset();


	stream << L"<ENTITY_MENTION ";

	stream << L"UID=\"";
	stream << obtainEntityMentionUID(mention, mention->getUID());
	stream << L"\" ";

	stream << L"entityUID=\"ent";
	stream << (entity ? entity->getID() : uid);
	stream << L"\" ";  //getGUID()?

	stream << L"syntacticCategory=\"";
	//convert syntacticCategory value to uppercase
	Mention::Type mentionType = fixPartitiveCase(mention->getMentionType());
	char typeString[500];
	strncpy(typeString, Mention::getTypeString(mentionType), 100);
	_strupr_s(typeString, 500);
	stream << typeString;
	stream << L"\" ";

	stream << L"headWord=\"";
	appendEscaped(stream, tokens->getToken(mention->getHead()->getHeadPreterm()->getStartToken())->getSymbol().to_string());
	stream << L"\" ";

	stream << L"textString=\"";
	appendNodeText(stream, node, tokens);
	stream << L"\" ";

	stream << L"startOffset=\"";
	stream << start;
	stream << L"\" ";

	stream << L"stringLength=\"";
	stream << ((end.value() + 1) - start.value());
	stream << L"\"";

	stream << L"/>\n";
}

void CBRNEResultCollector::appendRelMention(OutputStream &stream,
											RelMention *relMention,
										   const Relation *relation)
{
	const SynNode *node = fixAppositiveCase(relMention->getLeftMention())->getNode();
    int sentence_num = fixAppositiveCase(relMention->getLeftMention())->getSentenceNumber();
	const TokenSequence *tokens = _docTheory->getSentenceTheory(sentence_num)->getTokenSequence();
	int startTok = node->getStartToken();
	int endTok = node->getEndToken();
	EDTOffset start = tokens->getToken(startTok)->getStartEDTOffset();
	EDTOffset end = tokens->getToken(endTok)->getEndEDTOffset();

	node = fixAppositiveCase(relMention->getRightMention())->getNode();
	sentence_num = fixAppositiveCase(relMention->getRightMention())->getSentenceNumber();
	tokens = _docTheory->getSentenceTheory(sentence_num)->getTokenSequence();
	if(node->getStartToken() < startTok) {
		startTok = node->getStartToken();
		start = tokens->getToken(startTok)->getStartEDTOffset();
	}

	if(node->getEndToken() > endTok) {
		endTok = node->getEndToken();
		end = tokens->getToken(endTok)->getEndEDTOffset();
	}

	//endTok = node->getEndToken();
	//if (start > tokens->getToken(startTok)->getStartEDTOffset())
	//if (end < tokens->getToken(endTok)->getEndEDTOffset())
	//	end = tokens->getToken(endTok)->getEndEDTOffset();


	stream << L"<RELATION_MENTION ";

	stream << L"UID=\"";
	stream << obtainRelMentionUID(relMention, relMention->getUID());
	stream << L"\" ";

	stream << L"relationType=\"";
	//get the relation type name: find in table
	//stream << relMention->getType().to_string());
	Symbol type = relMention->getType();

	if(_relationTypeMap) {
		SymArray *key = _new SymArray(&type, 1);
		SymArray **searchResult = (*_relationTypeMap).get(key); //MEM:BORROW searchResult (this note means: I am borrowing
																// someone else's memory, so I don't get deleted here.

		// there had better be an entry here, because _relationTypeMap exists and isValidRelationMention() succeeded
		if(!searchResult)
			throw InternalInconsistencyException("CBRNEResultCollector::appendRelMention()", "No mapped relation type found for apparently valid type.");

		SymArray *value = *searchResult;						//MEM:BORROW value
		delete key;
		type = (value->array)[0];
	}
	//we must also prepend the entity Types of the two slots

	Symbol slot1Type, slot2Type;
	slot1Type = fixAppositiveCase(relMention->getLeftMention())->getEntityType().getName();
	slot2Type = fixAppositiveCase(relMention->getRightMention())->getEntityType().getName();


	//cout << slot1Type.to_debug_string() << ",  " << slot2Type.to_debug_string() <<"\n";
	//const wchar_t *str1 = slot1Type.to_debug_string();
	//const wchar_t *str2 = slot2Type.to_string();
	stream << slot1Type.to_debug_string();
	stream << L"-";
	stream << slot2Type.to_debug_string();
	stream << L".";
	stream << type.to_string();
	stream << L"\" ";

	stream << L"startOffset=\"";
	stream << start;
	stream << L"\" ";

	stream << L"stringLength=\"";
	stream << ((end.value()+1) - start.value());
	stream << L"\" ";

	stream << L"textString=\"";
	int i;
	appendEscaped(stream, tokens->getToken(startTok)->getSymbol().to_string());
	for(i=startTok+1; i<=endTok; i++) {
		stream << L" ";
		appendEscaped(stream, tokens->getToken(i)->getSymbol().to_string());
	}
	//stream << _docTheory->getSentence(sentence_num)->getString()->substring(start,end)->toSymbol().to_string());
	stream << L"\"";

	stream << L"/>\n";
}

void CBRNEResultCollector::appendSlots(OutputStream &stream,
									   RelMention *relMention,
									  const Relation *relation)
{
	//left slot first
	stream << L"<SLOT ";

	stream << L"relationMentionUID=\"";
	stream << obtainRelMentionUID(relMention, relMention->getUID());
	stream << L"\" ";

	stream << L"slotType=\"";
	stream << L"ARG1";
	stream << L"\" ";

	const Mention *leftMention = fixAppositiveCase(relMention->getLeftMention());

	stream << L"entityMentionUID=\"";
	stream << obtainEntityMentionUID(leftMention, leftMention->getUID());
	stream << L"\" ";

	stream << L"/>\n";

	//right slot
	stream << L"<SLOT ";

	stream << L"relationMentionUID=\"";
	stream << obtainRelMentionUID(relMention, relMention->getUID());
	stream << L"\" ";

	stream << L"slotType=\"";
	stream << L"ARG2";
	stream << L"\" ";

	const Mention *rightMention = fixAppositiveCase(relMention->getRightMention());

	stream << L"entityMentionUID=\"";
	stream << obtainEntityMentionUID(rightMention, rightMention->getUID());
	stream << L"\" ";

	stream << L"/>\n";
}

const Mention *CBRNEResultCollector::fixAppositiveCase(const Mention *mention) {
	if(mention->mentionType == Mention::APPO) {
		Mention *child1, *child2;
		child1 = mention->getChild();
		child2 = child1->getNext();
		Mention::Type child1Type = fixPartitiveCase(child1->getMentionType());
		Mention::Type child2Type = fixPartitiveCase(child2->getMentionType());
		if(child1Type==Mention::NAME)
			mention = child1;
		else if(child2Type==Mention::NAME)
			mention = child2;
		else if(child1Type==Mention::DESC)
			mention = child1;
		else if(child2Type==Mention::DESC)
			mention = child2;
		else {
			SessionLogger::warn("CBRNE") 
				<< "Appositive found with no DESC or NAME children. This will cause a mistake in output.";
		}
	}
	return mention;
}

void CBRNEResultCollector::appendNodeText(OutputStream &stream,
										  const SynNode* node,
										 const TokenSequence *tokens)
{
	int startTok = node->getStartToken();
	int endTok = node->getEndToken();
	EDTOffset start = tokens->getToken(startTok)->getStartEDTOffset();
	EDTOffset end = tokens->getToken(endTok)->getEndEDTOffset();

	int i;
	for (i=startTok; i<=endTok; i++) {

		const wchar_t* symStr = tokens->getToken(i)->getSymbol().to_string();
		appendEscaped(stream, symStr);
		if(i != endTok)
			stream << L" ";
	}

}

// append a relation of type UNK
void CBRNEResultCollector::appendVerbRelation(OutputStream &stream,
											  const Proposition *proposition,
											  const RelMentionUID uid,
											  const int sentence_num)
{
	//int sentence_num = proposition->getArg(0)->getMention(;
	const TokenSequence *tokens = _docTheory->getSentenceTheory(sentence_num)->getTokenSequence();
	const MentionSet *mentions = _docTheory->getSentenceTheory(sentence_num)->getMentionSet();
	const Argument *sub=0, *obj=0, *iobj=0;
	const Symbol headWord = proposition->getPredHead()->getHeadWord();

	int i;
	int startTok = -1, endTok = -1;
	startTok = proposition->getPredHead()->getStartToken();
	endTok = proposition->getPredHead()->getEndToken();
	EDTOffset start = tokens->getToken(startTok)->getStartEDTOffset();
	EDTOffset end = tokens->getToken(endTok)->getEndEDTOffset();

	for (i=0; i<proposition->getNArgs(); i++) {
		const Argument *arg = proposition->getArg(i);
		if(arg->getType() == Argument::MENTION_ARG) {
			if(arg->getRoleSym() == Argument::SUB_ROLE)
				sub = arg;
			if(arg->getRoleSym() == Argument::OBJ_ROLE)
				obj = arg;
			if(arg->getRoleSym() == Argument::IOBJ_ROLE)
				iobj = arg;

			int arg_startTok = arg->getMention(mentions)->getNode()->getStartToken();
			int arg_endTok = arg->getMention(mentions)->getNode()->getEndToken();
			if(arg_startTok < startTok || startTok == -1) {
				startTok = arg_startTok;
				start = tokens->getToken(arg_startTok)->getStartEDTOffset();
			}
			if(arg_endTok > endTok || endTok == -1) {
				endTok = arg_endTok;
				end   = tokens->getToken(arg_endTok)->getEndEDTOffset();
			}

		}
	}

	stream << L"<RELATION_MENTION ";

	stream << L"UID=\"";
	stream << obtainRelMentionUID(NULL, uid);
	stream << L"\" ";

	stream << L"relationType=\"";
	stream << L"UNK";
	stream << L"\" ";

	stream << L"startOffset=\"";
	stream << start;
	stream << L"\" ";

	stream << L"stringLength=\"";
	stream << ((end.value()+1) - start.value());
	stream << L"\" ";

	stream << L"headWord=\"";
	appendEscaped(stream, headWord.to_string());
	stream << L"\" ";

	stream << L"textString=\"";
	appendEscaped(stream, tokens->getToken(startTok)->getSymbol().to_string());
	for(i=startTok+1; i<=endTok; i++) {
		stream << L" ";
		appendEscaped(stream, tokens->getToken(i)->getSymbol().to_string());
	}
	stream << L"\"";

	stream << L"/>\n";
}

void CBRNEResultCollector::appendVerbRelationSlots(OutputStream &stream,
												   const Proposition *proposition,
												   const RelMentionUID uid,
												   const int sentence_num)
{
	const MentionSet *mentions = _docTheory->getSentenceTheory(sentence_num)->getMentionSet();
	if(proposition->getPredType() != Proposition::VERB_PRED) return;
	int i;
	wchar_t argType[1024];
	for (i=0; i<proposition->getNArgs(); i++) {
		const Argument *arg = proposition->getArg(i);
		wcscpy(argType, L"");

		if(arg->getType() == Argument::MENTION_ARG) {
			const Mention *mention = arg->getMention(mentions);
			if(!mention)
				throw InternalInconsistencyException("CBRNEResultCollector::appendVerbRelationSlots()", "Mention argument has null mention pointer.");
			if(isValidEntityMention(mention) && isTypedEntityMention(mention)) {
				if(arg->getRoleSym() == Argument::SUB_ROLE)
					wcscpy(argType, L"SUB");
				if(arg->getRoleSym() == Argument::OBJ_ROLE)
					wcscpy(argType, L"OBJ");
				if(arg->getRoleSym() == Argument::IOBJ_ROLE)
					wcscpy(argType, L"IOBJ");
				if(CBRNEWordConstants::isCBRNEReportedPreposition(arg->getRoleSym())) {
					wcscpy(argType, arg->getRoleSym().to_string());
					_wcsupr_s(argType,1024);
				}
			}
		}

		if(wcslen(argType) > 0) {
			stream << L"<SLOT ";

			stream << L"relationMentionUID=\"";
			stream << obtainRelMentionUID(NULL, uid);
			stream << L"\" ";

			//TODO: arg names
			stream << L"slotType=\"";
			stream << argType;
			stream << L"\" ";

			stream << L"entityMentionUID=\"";
			stream << obtainEntityMentionUID(arg->getMention(mentions), arg->getMention(mentions)->getUID());
			stream << L"\" ";

			stream << L"/>\n";
		}
	}
}

void CBRNEResultCollector::appendSentence(OutputStream &stream, const Sentence *sentence) {
	//int start = sentence->getStartEDTOffset();
	//int length = sentence->getNChars();
	const LocatedString *lstr = sentence->getString();
	EDTOffset start = lstr->start<EDTOffset>(0);
	int length = lstr->end<EDTOffset>(lstr->length()-1).value() - start.value();
	int uid = sentence->getSentNumber();

	stream << L"<SENTENCE UID=\"";
	stream << obtainSentenceUID(uid);
	stream << L"\" startOffset=\"";
	stream << start;
	stream << L"\" stringLength=\"";
	stream << length;
	stream << L"\"";
	stream << L"/>\n";
}

void CBRNEResultCollector::appendBeginDocTag(OutputStream &stream, const wchar_t * file) {
	stream << L"<DOC ID=\"";
	stream << _docTheory->getDocument()->getName().to_string();
	stream << L"\">\n";
}

void CBRNEResultCollector::appendEndDocTag(OutputStream &stream) {
	stream << L"</DOC>\n";
}

void CBRNEResultCollector::appendEscaped(OutputStream &stream, const wchar_t *str) {
	wchar_t reserved[] = L"<>&\"\'";
	wchar_t * str_copy = _new wchar_t[wcslen(str)+1];
	wchar_t *next_reserved_char = NULL;
	size_t i;
	wcscpy(str_copy, str);
	wchar_t *str_copy_beginning = str_copy;
	do {
		next_reserved_char = NULL;
		for(i=0; i<wcslen(reserved); i++) {
			wchar_t *found = wcschr(str_copy, reserved[i]);
            if(found && (!next_reserved_char || found < next_reserved_char))
				next_reserved_char = found;
		}
		if(next_reserved_char) {
			wchar_t res = *next_reserved_char;
			*next_reserved_char = L'\0';
			stream << str_copy;
			switch (res) {
				case L'<' : stream << L"&lt;";
					break;
				case L'>' : stream << L"&gt;";
					break;
				case L'&' : stream << L"&amp;";
					break;
				case L'\'': stream << L"&apos;";
					break;
				case L'\"': stream << L"&quot;";
					break;
				default:
					throw InternalInconsistencyException("CBRNEResultCollector::appendEscapedToResultString()", "Unexpected reserved char encountered.");
			}
			str_copy = next_reserved_char + 1;
		}
		else {
			stream << str_copy;
			str_copy += wcslen(str_copy);
		}
	} while (wcslen(str_copy));
	delete [] str_copy_beginning;

}

