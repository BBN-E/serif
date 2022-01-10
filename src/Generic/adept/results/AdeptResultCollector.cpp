// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/results/ResultCollector.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/AdeptWordConstants.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
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
#include "Generic/adept/results/AdeptResultCollector.h"
#include <math.h>
#include <iostream>
//#include "Generic/common/StringTransliterator.h"

using namespace std;

AdeptResultCollector::AdeptResultCollector() : _docTheory(0), _resultString(0)
{
	loadRelationTypeNameMap();
	loadRelationFilter();
}

AdeptResultCollector::~AdeptResultCollector() {
	delete[] _resultString;
	delete _relationTypeMap;
	delete _relationFilter;
}

void AdeptResultCollector::loadRelationTypeNameMap() {
	_relationTypeMap = NULL;
	const MAX_LEN = 256;
	char filename[MAX_LEN];
	if(!ParamReader::getNarrowParam(filename, Symbol(L"relation_type_map"), MAX_LEN))
		return;

	Symbol key[1], value[1];
	// read in file
	//int i;
	SexpReader reader(filename);
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

void AdeptResultCollector::loadRelationFilter() {
	_relationFilter = NULL;
	//const MAX_LEN = 456;
	//char filename[MAX_LEN];
	//if(!ParamReader::getNarrowParam(filename, Symbol(L"relation_filter"), MAX_LEN))
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

void AdeptResultCollector::loadDocTheory(DocTheory *docTheory) {
	_docTheory = docTheory;
	resetResultString();
	createResultString();
}

void AdeptResultCollector::produceOutput(const wchar_t *output_dir,
                                         const wchar_t *document_filename) 
{ 	
	if(!output_dir || !document_filename)
		return;
	wstring output_file = wstring(output_dir) + LSERIF_PATH_SEP + wstring(document_filename) + L".adept.xml";

	UTF8OutputStream stream;
	stream.open(output_file.c_str());
	stream << _resultString;
	stream.close();	
}

const wchar_t * AdeptResultCollector::retrieveOutput() {
	return _resultString;
}

void AdeptResultCollector::resetResultString() {
	if(_resultString != NULL) 
		delete[] _resultString;
	_resultString = _new wchar_t [1024];
	_resultString[0] = L'\0';
	_maxResultString = 1024;
}

void AdeptResultCollector::createResultString() {
	appendBeginDocTag(L"");

	// steps:
	//1. write entities
	//2. write entity mentions
	//3. write relation mentions
	//4. write relation slots
	//5. write sentences

	const EntitySet *entitySet = _docTheory->getEntitySet();
    int i;
	int nKnownEntities = entitySet->getNEntities();
	const nSentences = _docTheory->getNSentences();

	// 1A. WRITE KNOWN ENTITIES
	for (i=0; i<nKnownEntities; i++) {
		Entity *thisEntity = entitySet->getEntity(i);
		if (isValidEntity(thisEntity)) 
			appendEntity(thisEntity);
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
				appendExtraEntity(nKnownEntities + nExtraEntities, mention, i);
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
					appendMention(mention, NULL, nKnownEntities + nExtraEntities);
					nExtraEntities++;
				}
				else {
					Entity *entity = entities->getEntityByMention(mention->getUID());
					appendMention(mention, entity);
				}
			}			
		}
	}

	// 3A. WRITE NON-UNK RELATION MENTIONS
	const RelationSet *relationSet = _docTheory->getRelationSet();
	int maxRelationUID = -1;
	for (i=0; i<relationSet->getNRelations(); i++) {
		Relation *thisRelation = relationSet->getRelation(i);
		Relation::LinkedRelMention *iterator = thisRelation->getMentions();
		while (iterator != NULL) {
			RelMention *thisMention = iterator->relMention;
			if(isValidRelationMention(thisMention)) {
				if(maxRelationUID < thisMention->getUID())
					maxRelationUID = thisMention->getUID();
				appendRelMention(thisMention, thisRelation);
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
			if(isValidProposition(proposition, i)) 
				appendVerbRelation(proposition, maxRelationUID + (++nVerbRelations), i);
		}
	}

	// 4A. WRITE NON-UNK RELATION SLOTS
	for (i=0; i<relationSet->getNRelations(); i++) {
		Relation *thisRelation = relationSet->getRelation(i);
		Relation::LinkedRelMention *iterator = thisRelation->getMentions();
		while (iterator != NULL) {
			RelMention *thisMention = iterator->relMention;
			if(isValidRelationMention(thisMention)) {
		        appendSlots(thisMention, thisRelation);
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
			if(isValidProposition(proposition, i))
				appendVerbRelationSlots(proposition, maxRelationUID + (++nVerbRelations), i);
		}
	}

	// 5. WRITE SENTENCES
	for (i=0; i<nSentences; i++) {
		appendSentence(_docTheory->getSentence(i));
	}

	appendEndDocTag();
}

// the mention has been linked to an entity by serif
bool AdeptResultCollector::isLinkedEntityMention(const Mention *mention) {
	return (_docTheory->getEntitySet()->getEntityByMention(mention->getUID()) != NULL);
}

// the mention has not been linked but has a valid entity type
bool AdeptResultCollector::isUnlinkedTypedEntityMention(const Mention *mention) {
	return (!isLinkedEntityMention(mention) && isTypedEntityMention(mention));
}

// the mention has not been linked and has an invalid entity type
bool AdeptResultCollector::isUnlinkedUntypedEntityMention(const Mention *mention) {
	return (!isLinkedEntityMention(mention) && !isTypedEntityMention(mention));
}

// the mention has a valid entity type
bool AdeptResultCollector::isTypedEntityMention(const Mention *mention) {
	return mention->getEntityType().isRecognized();
}

bool AdeptResultCollector::isValidRelationMention(const RelMention *mention) {
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
bool AdeptResultCollector::isValidEntityMention(const Mention *mention) {
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
bool AdeptResultCollector::isValidEntity(const Entity *entity) {
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

Mention::Type AdeptResultCollector::fixPartitiveCase(Mention::Type type) {
	return (type == Mention::PART) ? Mention::DESC : type;
}

// to be valid, a prop must be VERB_PRED and have both a subject and either (object or indirect-object), such that 
//  NEITHER is an unknown-entity mention
bool AdeptResultCollector::isValidProposition(const Proposition *proposition, const int sentence_num) {
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
					if(AdeptWordConstants::isADEPTReportedPreposition(arg->getRoleSym()))
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

void AdeptResultCollector::appendEntity(const Entity *entity) {

	appendToResultString(L"<ENTITY ");
	
	appendToResultString(L"UID=\"ent");
	appendToResultString(entity->getID());
	appendToResultString(L"\" ");
	
	appendToResultString(L"entityType=\"");
	if (!entity->type.isRecognized())
		appendToResultString("UNK");
	else
		appendToResultString(entity->type.getName().to_string());
	appendToResultString(L"\" ");

	appendToResultString(L"canonicalString=\"");
	// find canonical string
	int i;
	Mention *cMention = NULL;  //canonical mention
	wchar_t cString[1024] = L"";
	for(i=0; i<entity->getNMentions(); i++) {
		Mention *thisMention = _docTheory->getEntitySet()->getMention(entity->getMention(i));
		const TokenSequence *tokens = _docTheory->getSentenceTheory(thisMention->getSentenceNumber())->getTokenSequence();
		const MAX_STRLEN = 1024;
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

	appendEscapedToResultString(cString);
	appendToResultString(L"\" ");
	
	appendToResultString(L"/>\n");    
}

void AdeptResultCollector::appendExtraEntity(int uid, Mention *mention, int sentence_num) {
	const TokenSequence *tokens = _docTheory->getSentenceTheory(sentence_num)->getTokenSequence();
	// find canonical string
	const MAX_STRLEN = 1024;
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
	appendToResultString(L"<ENTITY ");
	
	appendToResultString(L"UID=\"ent");
	appendToResultString(uid);
	appendToResultString(L"\" ");
	
	appendToResultString(L"entityType=\"");
	appendToResultString(type.to_string());
	appendToResultString(L"\" ");

	appendToResultString(L"canonicalString=\"");
	appendEscapedToResultString(cString);
	appendToResultString(L"\" ");
	
	appendToResultString(L"/>\n");    
}

int AdeptResultCollector::obtainRelMentionUID(const RelMention *mention, int suggestedUID) {
	if(suggestedUID == -1)
		// this is an error!
		return -1;
	else return (suggestedUID * 5) + 1;
}

int AdeptResultCollector::obtainEntityMentionUID(const Mention *mention, int suggestedUID) {
	if(suggestedUID == -1)
		// this is an error!
		return -1;
	else return (suggestedUID * 5) + 2;
}

int AdeptResultCollector::obtainSentenceUID(int suggestedUID) {
	if(suggestedUID == -1)
		// this is an error!
		return -1;
	else return (suggestedUID * 5) + 3;
}

int AdeptResultCollector::obtainSlotUID(int suggestedUID) {
	if(suggestedUID == -1)
		// this is an error!
		return -1;
	else return (suggestedUID * 5) + 4;
}


void AdeptResultCollector::appendMention(const Mention *mention,
										const Entity *entity,
										int uid) 
{
	const SynNode *node = mention->getNode();
    int sentence_num = mention->getSentenceNumber();
	const TokenSequence *tokens = _docTheory->getSentenceTheory(sentence_num)->getTokenSequence();
	int startTok = node->getStartToken();
	int endTok   = node->getEndToken();
	int start    = tokens->getToken(startTok)->getStartOffset();
	int end      = tokens->getToken(endTok)->getEndOffset(); 


	appendToResultString(L"<ENTITY_MENTION ");
	
	appendToResultString(L"UID=\"");
	appendToResultString(obtainEntityMentionUID(mention, mention->getUID()));
	appendToResultString(L"\" ");
	
	appendToResultString(L"entityUID=\"ent");
	appendToResultString((entity ? entity->getID() : uid));
	appendToResultString(L"\" ");  //getGUID()?

	appendToResultString(L"syntacticCategory=\"");
	//convert syntacticCategory value to uppercase
	Mention::Type mentionType = fixPartitiveCase(mention->getMentionType());
	char typeString[500];
	strncpy(typeString, Mention::getTypeString(mentionType), 100);
	strupr(typeString);
	appendToResultString(typeString);
	appendToResultString(L"\" ");
	
	appendToResultString(L"headWord=\"");   
	appendEscapedToResultString(tokens->getToken(mention->getHead()->getHeadPreterm()->getStartToken())->getSymbol().to_string());
	appendToResultString(L"\" ");
	
	appendToResultString(L"textString=\"");   
	appendNodeText(node, tokens); 
	appendToResultString(L"\" ");
	
	appendToResultString(L"startOffset=\"");
	appendToResultString(start);
	appendToResultString(L"\" ");
	
	appendToResultString(L"stringLength=\"");
	appendToResultString((end + 1) - start);
	appendToResultString(L"\"");
	
	appendToResultString(L"/>\n");
}

void AdeptResultCollector::appendRelMention(RelMention *relMention,
										   const Relation *relation)
{
	const SynNode *node = fixAppositiveCase(relMention->getLeftMention())->getNode();
    int sentence_num = fixAppositiveCase(relMention->getLeftMention())->getSentenceNumber();
	const TokenSequence *tokens = _docTheory->getSentenceTheory(sentence_num)->getTokenSequence();
	int startTok = node->getStartToken();
	int endTok = node->getEndToken();
	int start = tokens->getToken(startTok)->getStartOffset();
	int end = tokens->getToken(endTok)->getEndOffset(); 

	node = fixAppositiveCase(relMention->getRightMention())->getNode();
	sentence_num = fixAppositiveCase(relMention->getRightMention())->getSentenceNumber();
	tokens = _docTheory->getSentenceTheory(sentence_num)->getTokenSequence();
	if(node->getStartToken() < startTok) {
		startTok = node->getStartToken();
		start = tokens->getToken(startTok)->getStartOffset();
	}

	if(node->getEndToken() > endTok) {
		endTok = node->getEndToken();
		end = tokens->getToken(endTok)->getEndOffset();
	}

	//endTok = node->getEndToken();
	//if (start > tokens->getToken(startTok)->getStartOffset())
	//if (end < tokens->getToken(endTok)->getEndOffset())
	//	end = tokens->getToken(endTok)->getEndOffset();


	appendToResultString(L"<RELATION_MENTION ");
	
	appendToResultString(L"UID=\"");          
	appendToResultString(obtainRelMentionUID(relMention, relMention->getUID()));
	appendToResultString(L"\" ");
	
	appendToResultString(L"relationType=\"");
	//get the relation type name: find in table
	//appendToResultString(relMention->getType().to_string());
	Symbol type = relMention->getType();
	
	if(_relationTypeMap) {
		SymArray *key = _new SymArray(&type, 1);					
		SymArray **searchResult = (*_relationTypeMap).get(key); //MEM:BORROW searchResult (this note means: I am borrowing 
																// someone else's memory, so I don't get deleted here.

		// there had better be an entry here, because _relationTypeMap exists and isValidRelationMention() succeeded
		if(!searchResult)
			throw InternalInconsistencyException("AdeptResultCollector::appendRelMention()", "No mapped relation type found for apparently valid type.");
		
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
	appendToResultString(slot1Type.to_debug_string());
	appendToResultString(L"-");
	appendToResultString(slot2Type.to_debug_string());
	appendToResultString(L".");
	appendToResultString(type.to_string());
	appendToResultString(L"\" ");
	
	appendToResultString(L"startOffset=\"");
	appendToResultString(start);
	appendToResultString(L"\" ");
	
	appendToResultString(L"stringLength=\"");
	appendToResultString((end+1) - start);
	appendToResultString(L"\" ");
	
	appendToResultString(L"textString=\"");
	int i;
	appendEscapedToResultString(tokens->getToken(startTok)->getSymbol().to_string());
	for(i=startTok+1; i<=endTok; i++) {
		appendToResultString(L" ");
		appendEscapedToResultString(tokens->getToken(i)->getSymbol().to_string());
	}
	//appendToResultString(_docTheory->getSentence(sentence_num)->getString()->substring(start,end)->toSymbol().to_string());
	appendToResultString(L"\"");
	
	appendToResultString(L"/>\n");
}

void AdeptResultCollector::appendSlots(RelMention *relMention,
									  const Relation *relation) 
{
	//left slot first
	appendToResultString(L"<SLOT ");
	
	appendToResultString(L"relationMentionUID=\"");
	appendToResultString(obtainRelMentionUID(relMention, relMention->getUID()));
	appendToResultString(L"\" ");
	
	appendToResultString(L"slotType=\"");
	appendToResultString(L"ARG1");
	appendToResultString(L"\" ");
	
	const Mention *leftMention = fixAppositiveCase(relMention->getLeftMention());
	
	appendToResultString(L"entityMentionUID=\"");
	appendToResultString(obtainEntityMentionUID(leftMention, leftMention->getUID()));
	appendToResultString(L"\" ");
	
	appendToResultString(L"/>\n");

	//right slot
	appendToResultString(L"<SLOT ");
	
	appendToResultString(L"relationMentionUID=\"");
	appendToResultString(obtainRelMentionUID(relMention, relMention->getUID()));
	appendToResultString(L"\" ");
	
	appendToResultString(L"slotType=\"");
	appendToResultString(L"ARG2");
	appendToResultString(L"\" ");
	
	const Mention *rightMention = fixAppositiveCase(relMention->getRightMention());

	appendToResultString(L"entityMentionUID=\"");
	appendToResultString(obtainEntityMentionUID(rightMention, rightMention->getUID()));
	appendToResultString(L"\" ");
	
	appendToResultString(L"/>\n");
}

const Mention *AdeptResultCollector::fixAppositiveCase(const Mention *mention) {
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
			SessionLogger *logger = SessionLogger::logger; 
			logger->beginWarning();
			(*logger) << "Appositive found with no DESC or NAME children. This will cause a mistake in output.";
		}
	}
	return mention;
}

void AdeptResultCollector::appendNodeText(const SynNode* node,
										 const TokenSequence *tokens) 
{
	int startTok = node->getStartToken();
	int endTok = node->getEndToken();
	int start = tokens->getToken(startTok)->getStartOffset();
	int end = tokens->getToken(endTok)->getEndOffset(); 

	int i;
	for (i=startTok; i<=endTok; i++) {
		
		const wchar_t* symStr = tokens->getToken(i)->getSymbol().to_string();
		appendEscapedToResultString(symStr);
		if(i != endTok)
			appendToResultString(L" ");
	}
		
}

// append a relation of type UNK
void AdeptResultCollector::appendVerbRelation(const Proposition *proposition,
											  const int uid,
											  const int sentence_num)
{
	//int sentence_num = proposition->getArg(0)->getMention(;
	const TokenSequence *tokens = _docTheory->getSentenceTheory(sentence_num)->getTokenSequence();
	const MentionSet *mentions = _docTheory->getSentenceTheory(sentence_num)->getMentionSet();
	const Argument *sub=0, *obj=0, *iobj=0;
	const Symbol headWord = proposition->getPredHead()->getHeadWord();

	int i;
	int start = -1, end = -1;
	int startTok = -1, endTok = -1;
	startTok = proposition->getPredHead()->getStartToken();
	endTok = proposition->getPredHead()->getEndToken();
	start = tokens->getToken(startTok)->getStartOffset();
	end = tokens->getToken(endTok)->getEndOffset();

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
				start = tokens->getToken(arg_startTok)->getStartOffset();
			}
			if(arg_endTok > endTok || endTok == -1) {
				endTok = arg_endTok;
				end   = tokens->getToken(arg_endTok)->getEndOffset();
			}

		}
	}

	appendToResultString(L"<RELATION_MENTION ");
	
	appendToResultString(L"UID=\"");          
	appendToResultString(obtainRelMentionUID(NULL, uid));
	appendToResultString(L"\" ");
	
	appendToResultString(L"relationType=\"");
	appendToResultString(L"UNK");
	appendToResultString(L"\" ");
	
	appendToResultString(L"startOffset=\"");
	appendToResultString(start);
	appendToResultString(L"\" ");
	
	appendToResultString(L"stringLength=\"");
	appendToResultString((end+1) - start);
	appendToResultString(L"\" ");
	
	appendToResultString(L"headWord=\"");
	appendEscapedToResultString(headWord.to_string());
	appendToResultString(L"\" ");

	appendToResultString(L"textString=\"");
	appendEscapedToResultString(tokens->getToken(startTok)->getSymbol().to_string());
	for(i=startTok+1; i<=endTok; i++) {
		appendToResultString(L" ");
		appendEscapedToResultString(tokens->getToken(i)->getSymbol().to_string());
	}
	appendToResultString(L"\"");

	appendToResultString(L"/>\n");
}

void AdeptResultCollector::appendVerbRelationSlots(const Proposition *proposition,
												   const int uid,
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
				throw InternalInconsistencyException("AdeptResultCollector::appendVerbRelationSlots()", "Mention argument has null mention pointer.");
			if(isValidEntityMention(mention) && isTypedEntityMention(mention)) {
				if(arg->getRoleSym() == Argument::SUB_ROLE)
					wcscpy(argType, L"SUB");
				if(arg->getRoleSym() == Argument::OBJ_ROLE)
					wcscpy(argType, L"OBJ");
				if(arg->getRoleSym() == Argument::IOBJ_ROLE)
					wcscpy(argType, L"IOBJ");
				if(AdeptWordConstants::isADEPTReportedPreposition(arg->getRoleSym())) {
					wcscpy(argType, arg->getRoleSym().to_string());
					wcsupr(argType);
				}
			}
		}

		if(wcslen(argType) > 0) {
			appendToResultString(L"<SLOT ");
			
			appendToResultString(L"relationMentionUID=\"");
			appendToResultString(obtainRelMentionUID(NULL, uid));
			appendToResultString(L"\" ");
			
			//TODO: arg names
			appendToResultString(L"slotType=\"");
			appendToResultString(argType);
			appendToResultString(L"\" ");
			
			appendToResultString(L"entityMentionUID=\"");
			appendToResultString(obtainEntityMentionUID(arg->getMention(mentions), arg->getMention(mentions)->getUID()));
			appendToResultString(L"\" ");
			
			appendToResultString(L"/>\n");
		}
	}
}

void AdeptResultCollector::appendSentence(const Sentence *sentence) {
	//int start = sentence->getStartCharNumber();
	//int length = sentence->getNChars(); 
	const LocatedString *lstr = sentence->getString();
	int start = lstr->edtBeginOffsetAt(0);
	int length = lstr->edtEndOffsetAt(lstr->length()-1) - start;
	int uid = sentence->getSentNumber(); 

	appendToResultString(L"<SENTENCE UID=\"");
	appendToResultString(obtainSentenceUID(uid));
	appendToResultString(L"\" startOffset=\"");
	appendToResultString(start);
	appendToResultString(L"\" stringLength=\"");
	appendToResultString(length);
	appendToResultString(L"\"");
	appendToResultString(L"/>\n");
}	

void AdeptResultCollector::appendBeginDocTag(const wchar_t * file) {
	appendToResultString(L"<DOC ID=\"");
	appendToResultString(_docTheory->getDocument()->getName().to_string());
	appendToResultString(L"\">\n");
}

void AdeptResultCollector::appendEndDocTag() {
	appendToResultString(L"</DOC>\n");
}

// This function no longer manipulates str inline.
void AdeptResultCollector::appendEscapedToResultString(const wchar_t *str) {
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
			appendToResultString(str_copy);
			switch (res) {
				case L'<' : appendToResultString(L"&lt;");
					break;
				case L'>' : appendToResultString(L"&gt;");
					break;
				case L'&' : appendToResultString(L"&amp;");
					break;
				case L'\'': appendToResultString(L"&apos;");
					break;
				case L'\"': appendToResultString(L"&quot;");
					break;
				default:
					throw InternalInconsistencyException("AdeptResultCollector::appendEscapedToResultString()", "Unexpected reserved char encountered.");
			}
			str_copy = next_reserved_char + 1;			
		}
		else {
			appendToResultString(str_copy);
			str_copy += wcslen(str_copy);
		}
	} while (wcslen(str_copy));
	delete [] str_copy_beginning;

}

void AdeptResultCollector::appendToResultString(int num) { 
	int numDigits = (int) log10((double) num);
	wchar_t *str = _new wchar_t[numDigits+20];
	swprintf(str, L"%d", num);
	appendToResultString(str);
	delete[] str;
}

void AdeptResultCollector::appendToResultString(const wchar_t *str) { 
	if(str==NULL)
		throw InternalInconsistencyException("AdeptResultCollector::appendToResultString()", "Null string selected for output.");
	while(wcslen(_resultString) + wcslen(str) + 1  > _maxResultString) {
		//double size
		wchar_t *newString = _new wchar_t[_maxResultString * 2];
		wcscpy(newString, _resultString);
		delete[] _resultString;
		_resultString = newString;
		_maxResultString *= 2;
	}
	//append
	wcscat(_resultString, str);
}

void AdeptResultCollector::appendToResultString(const char *str) { 
	wchar_t *wstr = _new wchar_t [strlen(str) + 1];
	swprintf(wstr, L"%hs", str);
	appendToResultString(wstr);
	delete [] wstr;
}
