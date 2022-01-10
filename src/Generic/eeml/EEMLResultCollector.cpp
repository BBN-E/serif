// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/eeml/EEMLResultCollector.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/RelationSet.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/theories/Relation.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/EventSet.h"
#include "Generic/theories/EventEntityRelation.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/eeml/GroupFnGuesser.h"
//#include "Generic/common/StringTransliterator.h"

using namespace std;

#define PRINT_OUT_GENERIC_EE_RELATIONS true

// this one trumps PRINT_OUT_GENERIC_EE_RELATIONS
#define PRINT_EVERYTHING_AS_SPECIFIC true

void EEMLResultCollector::loadDocTheory(DocTheory* theory) {
	finalize(); // get rid of old stuff

	_docTheory = theory;
	_entitySet = theory->getEntitySet();
	_relationSet = theory->getRelationSet();
	_eventSet = theory->getEventSet();

	int numSents = theory->getNSentences();
	_tokenSequence = _new const TokenSequence*[numSents];
	for (int i = 0; i < numSents; i++)
		_tokenSequence[i] = theory->getSentenceTheory(i)->getTokenSequence();
}

// WARNING: read the memory warning below before making changes to this method!
void EEMLResultCollector::produceOutput(const wchar_t *output_dir,
									    const wchar_t *doc_filename)
{
	wstring output_file = wstring(output_dir) + LSERIF_PATH_SEP + wstring(doc_filename) + L".eeml";

	UTF8OutputStream stream;
	stream.open(output_file.c_str());
	
	// memory?
	bool *isEntityPrinted = _new bool[_entitySet->getNEntities()];
	bool *isEntityGeneric = _new bool[_entitySet->getNEntities()];

	for (int i = 0; i < _entitySet->getNEntities(); i++)
		isEntityPrinted[i] = false;

	// NOTE: these two were options that were both false in the last ace run
	//       I'm assuming we don't do them anymore
	// TODO: from original code - remove unlinked descriptors
	// TODO: from original code - remove pronoun chains

	const wchar_t* docName = _docTheory->getDocument()->getName().to_string();
	
	// TODO: we need a way to figure source out!
	const wchar_t* source = L"newspaper";
	_printEEMLDocumentHeader(stream, docName, source, doc_filename);
	if (_entitySet != NULL) {
		int ents = _entitySet->getNEntities();
		for (int i = 0; i < ents; i++) {
			Entity* ent = _entitySet->getEntity(i);
			GrowableArray<MentionUID> &mens = ent->mentions;

			// MEMORY: these are array deleted after they are filled and used
			// MEMORY WARNING: if a	break or continue is thrown	after this point, 
			//				   these have to be	deleted!!!
			Mention** validMentions	= _new Mention*[mens.length()];
			int	valid_mentions_size	= 0;
			Mention** validNames = _new	Mention*[mens.length()];
			int	valid_names_size = 0;

			// the selection criteria :	determine if this mention deserves
			// to be included, and if it should	also be	in the name
			// attributes section

			int	j;
			for	(j = 0;	j <	mens.length(); j++)	{
				Mention* ment =	_entitySet->getMention(mens[j]);
				_isValidMention[ment->getUID()] = false;
				
				// we only pretty much want	solo, unnested mentions
				// an exception	is made, though, for appositive	members
				// and GPE modifiers of	GPE	entities with person role (? &&	TODO)
				if (_isSecondPartOfAppositive(ment)	|| 
					_isGPEModifierOfPersonGPE(ment)	||
					// (_isTopMention(ment) &&	!_isNestedInsideOfName(ment))) {				
					(_isTopMention(ment, ent->getType()))) 
				{				
					validMentions[valid_mentions_size++] = ment;
					// names in	the	normal situation are the only mentions added
					// to the name attributes section
					if (ment->mentionType == Mention::NAME)
						validNames[valid_names_size++] = ment;
				} else if (_isNameNotInHeadOfParent(ment, ent->getType())) {
					validMentions[valid_mentions_size++] = ment;
				} 

			}
			// is there	something to print?
			if (valid_mentions_size	< 1) {
				// MEMORY: handle the arrays created earlier
				delete [] validMentions;
				delete [] validNames;
				continue;
			}



			// we should have at least one non-pronoun in an entity
			// this	could be determined	as the array is	collected, but it's	cleaner	to check now
			// and doesn't take	considerably longer
			/* bool seen_non_pronoun =	false;
			for	(j = 0;	j <	valid_mentions_size; j++) {
				if (validMentions[j]->mentionType != Mention::PRON)	{
					seen_non_pronoun = true;
					break;
				}
			}
			if (!seen_non_pronoun) {
				// MEMORY: handle the arrays created earlier
				delete [] validMentions;
				delete [] validNames;
				continue;
			}*/


			bool is_generic = PRINT_EVERYTHING_AS_SPECIFIC ? false : ent->isGeneric();
			_printEEMLEntityHeader(stream, ent, is_generic, 
				_isPluralEntity(ent), docName);
			isEntityPrinted[ent->getID()] = true;
			if (is_generic)
				isEntityGeneric[ent->getID()] = true;
			else 
				isEntityGeneric[ent->getID()] = false;

			// now print the mentions
			for	(j = 0;	j <	valid_mentions_size; j++) {
				_printEEMLMention(stream, validMentions[j], ent);
			}
			// now print the names
			if (valid_names_size > 0) {
				_printEEMLAttributeHeader(stream);
				for	(j = 0;	j <	valid_names_size; j++) {
					_printEEMLNameAttribute(stream, validNames[j]);
				}
				_printEEMLAttributeFooter(stream);
			}
							
			_printEEMLEntityFooter(stream);

			// MEMORY: handle the arrays created earlier
			delete [] validMentions;
			delete [] validNames;
		}
	}

	// Events and Event Relations
	if (_eventSet != NULL) {
		int n_events = _eventSet->getNEvents();
		for (int i = 0; i < n_events; i++) {
			Event *event = _eventSet->getEvent(i);

			_printEEMLEventHeader(stream, event, false, COMPLETED, docName);

			Event::LinkedEventMention* linkedEventMention = event->getEventMentions();
			while (linkedEventMention != NULL ) {
				_printEEMLEventMention(stream, linkedEventMention->eventMention, event, docName);
				linkedEventMention = linkedEventMention->next;
			}
			_printEEMLEventFooter(stream);

			// for every event, print out the consolidated event entity relations
			int n_ee_relations = event->getNumConsolidatedEERelations();
			for (int j = 0; j < n_ee_relations; j++) {
				EventEntityRelation* eeRelation = event->getConsolidatedEERelation(j);
				if (!PRINT_OUT_GENERIC_EE_RELATIONS && 
					!PRINT_EVERYTHING_AS_SPECIFIC &&
					isEntityGeneric[eeRelation->getEntityID()])
					continue;

				_printEEMLEERelationHeader(stream, eeRelation, docName);
				EventEntityRelation::LinkedEERelMention* linkedEERelMention = eeRelation->getEEMentions();
				while (linkedEERelMention != NULL) {
					_printEEMLEERelationMention(stream, linkedEERelMention->eventMention, linkedEERelMention->mention, eeRelation);
					linkedEERelMention = linkedEERelMention->next;
				}
				_printEEMLEERelationFooter(stream);
			}
		}
	}

	// TODO: generics go here
	if (_relationSet != NULL) {
		int n_rels = _relationSet->getNRelations();
		for (int i = 0; i < n_rels; i++) {
			Relation *rel = _relationSet->getRelation(i);

			// figure out if mentions were valid?
			const Relation::LinkedRelMention *mentions = rel->getMentions();
			if (mentions == 0) {
				//std::cerr << "no mentions\n";
				continue;
			}
			//std::cerr << "Trying to print: ";
			//std::cerr << rel->getMentions()->relMention->toDebugString() << "\n";
		

			if (!PRINT_EVERYTHING_AS_SPECIFIC &&
				(_entitySet->getEntity(rel->getLeftEntityID())->isGeneric() ||
				 _entitySet->getEntity(rel->getRightEntityID())->isGeneric()))
				continue;

			//std::cerr << "not generic\n";

			if (!isEntityPrinted[rel->getLeftEntityID()] ||
				!isEntityPrinted[rel->getRightEntityID()])
				continue;

			//std::cerr << "printed\n";
			
			_printEEMLRelationHeader(stream, rel, docName);
			while (mentions != 0) {
				_printEEMLRelMention(stream, mentions->relMention, rel);
				mentions = mentions->next;
			}
			_printEEMLRelationFooter(stream);
		}
	}
	
	_printEEMLDocumentFooter(stream);
	delete [] isEntityPrinted;
	delete [] isEntityGeneric;

	stream.close();
}

const wchar_t* EEMLResultCollector::_convertMentionTypeToEEML(Mention* ment)
{
	if (ment->getEntityType().isTemp())
		return L"Date/Time";

	Mention::Type type = ment->mentionType;
	Mention* subMent = 0;
	switch (type) {
		case Mention::NAME:
			return L"Name";
		case Mention::DESC:
		case Mention::PART:
			return L"Nominal";
		case Mention::PRON:
			return L"Pronoun";			
		case Mention::APPO:
			// for appositive, the type is the type of the first child
			subMent = ment->getChild();
			return _convertMentionTypeToEEML(subMent);
		case Mention::LIST:
			// for list, the type is the type of the first child
			subMent = ment->getChild();
			return _convertMentionTypeToEEML(subMent);
		default:
			return L"WARNING_BAD_MENTION_TYPE";
	}
}


// PRINTING NOTE: a former version used hard tabs here. for each former
//                hard tab, there are now two spaces.

void EEMLResultCollector::_printEEMLDocumentHeader(UTF8OutputStream& stream,
											  const wchar_t* doc_name,
											  const wchar_t* doc_source,
											  const wchar_t* doc_path)
{
	//stream << L"<?xml version=\"1.0\"?>\n";
	//stream << L"<!DOCTYPE source_file SYSTEM \"ace-eeml.v2.0.1.dtd\">\n";
	stream << L"<source_file SOURCE=\"" << doc_source 
		   << L"\" TYPE=\"text\" VERSION=\"2.0\" URI=\"" << doc_name
		   << L"\" PATH=\"" << doc_path << "\">\n";
	stream << L"  <document DOCID=\"" << doc_name << L"\">\n\n";
}

void EEMLResultCollector::_printEEMLEntityHeader(UTF8OutputStream& stream, 
											Entity* ent, 
											bool isGeneric, 
											bool isGroupFN,
											const wchar_t* doc_name)
{
	stream << L"    <entity ID=\"" << doc_name << L"-E" << ent->getID() << L"\">\n";
	stream << L"      <entity_type";
	if(isGeneric) 
		stream << L" GENERIC=\"TRUE\"";
	else
		stream << L" GENERIC=\"FALSE\"";

	if(isGroupFN)
		stream << L" GROUP_FN=\"TRUE\"";
	else 
		stream << L" GROUP_FN=\"FALSE\"";

	stream << L">" << ent->type.getName().to_string() << L"</entity_type>\n";
}

void EEMLResultCollector::_printEEMLAttributeHeader(UTF8OutputStream& stream) 
{
	stream << L"        <entity_attributes>\n";
}

void EEMLResultCollector::_printEEMLAttributeFooter(UTF8OutputStream& stream) 
{
	stream << L"        </entity_attributes>\n";
}


void EEMLResultCollector::_printEEMLMention(UTF8OutputStream& stream,
									   Mention* ment,
									   Entity* ent)
{
	// NOTE: in the rare case that we want to print a list as a mention,
	//       print each of its parts instead.
	if (ment->mentionType == Mention::LIST) {
		Mention* child = ment->getChild();
		while (child != 0) {
			_printEEMLMention(stream, child, ent);
			child = child->getNext();
		}
		return;
	}

	_isValidMention[ment->getUID()] = true;			

	stream << L"        <entity_mention TYPE=\"" << _convertMentionTypeToEEML(ment) << L"\"";
	stream << L" ID=\"" << ent->ID << L"-" << ment->getUID() << L"\"";
	// TODO: MENTION ROLE (once accessors have been written)
	// once there's a role, print role stuff here
	if (ment->hasRoleType())
		stream << L" ROLE=\"" << ment->getRoleType().getName().to_string() << "\"";
	//	if (!role.equals(""))
	//	stream << L" ROLE=\"NO_ROLE_INFO_YET\"";
	
	// TODO: INTENDED INFO (once accessors have been written)
	if (ment->hasIntendedType()) {
		if (ent->getType() == ment->getEntityType()) {
			stream << L" REFERENCE=\"LITERAL\"";		
		}
		else 
			stream << L" REFERENCE=\"INTENDED\"";
	}
/*	if (ment->isIntended())
	    stream << L" REFERENCE=\"INTENDED\" ";
	else if (ment->isLiteral())
		stream << L" REFERENCE=\"LITERAL\" ";
*/

	stream << L">\n";

	// print extent info for the whole extent and the head extent.
	const SynNode* node = ment->node;

	int sentNum = ment->getSentenceNumber();
	const SynNode* head = _getEDTHead(ment);
	stream << L"          <extent>\n";
	_printEEMLMentionExtent(stream, node, sentNum);
	stream << L"          </extent>\n";
	stream << L"          <head>\n";
	_printEEMLMentionExtent(stream, head, sentNum);
	stream << L"          </head>\n";
	stream << L"        </entity_mention>\n";

}

void EEMLResultCollector::_printEEMLNameAttribute(UTF8OutputStream& stream,
											 Mention* ment)
{
	stream << L"          <name>\n";
	_printEEMLMentionExtent(stream, _getEDTHead(ment), ment->getSentenceNumber());
	stream << L"          </name>\n";
}



void EEMLResultCollector::_printEEMLMentionExtent(UTF8OutputStream& stream,
											 const SynNode* node,
											 int sentNum)
{
	stream << L"              <charseq>\n";
	_printEEMLNodeText(stream, node, sentNum);

	int startTok = node->getStartToken();
	int endTok = node->getEndToken();
	int start = _tokenSequence[sentNum]->getToken(startTok)->getStartEDTOffset().value();
	int end = _tokenSequence[sentNum]->getToken(endTok)->getEndEDTOffset().value(); 
	stream << L"                <start>" << start << L"</start><end>" << end <<  L"</end></charseq>\n";
}

void EEMLResultCollector::_printEEMLNodeText(UTF8OutputStream& stream, 
		const SynNode* node,
		int sentNum) 
{
	int startTok = node->getStartToken();
	int endTok = node->getEndToken();
	int start = _tokenSequence[sentNum]->getToken(startTok)->getStartEDTOffset().value();
	int end = _tokenSequence[sentNum]->getToken(endTok)->getEndEDTOffset().value(); 
	
	stream << L"              <!-- string = \"";
	int i;
	for (i=startTok; i<=endTok; i++) {
		// check for double dash, which can't occur inside a comment
		// substitute it 
		const wchar_t* symStr = _tokenSequence[sentNum]->getToken(i)->getSymbol().to_string();
		const wchar_t* match = wcsstr(symStr, L"--");
		// normal case. no double dash
		if (match == 0)
			stream <<  symStr;
		else {
			size_t orig_len = wcslen(symStr);
			// worst case, it's all dashes
			wchar_t* safeStr = _new wchar_t[2*orig_len];
			unsigned int oldIdx = 0;
			unsigned int newIdx = 0;
			// copy, replacing -- with - - where appropriate
			for (; oldIdx < orig_len; oldIdx++) {
				safeStr[newIdx++] = symStr[oldIdx];
				if (symStr[oldIdx] == L'-' && 
					oldIdx+1 < orig_len && 
					symStr[oldIdx+1] == L'-')
					safeStr[newIdx++] = L' ';
			}
			safeStr[newIdx] = L'\0';
			stream << safeStr;
			delete [] safeStr;
		}		 
		stream << L" ";
	}
		
	stream << L"\"-->\n";
}

void EEMLResultCollector::_printEEMLRelationHeader(UTF8OutputStream& stream, 
											Relation* rel,  
											const wchar_t* doc_name)
{
	Symbol type = rel->getType();

	
	stream << L"    <relation ID=\"" << doc_name << L"-R" << rel->getID() << L"\"";
	stream << L" TYPE=\"" << RelationConstants::getBaseTypeSymbol(type).to_string();
	stream << L"\" SUBTYPE=\"" << RelationConstants::getSubtypeSymbol(type).to_string();
	stream << L"\" CLASS=\"EXPLICIT\">\n";

	stream << L"      <rel_entity_arg ENTITYID=\"";
	stream << doc_name << L"-E" << rel->getLeftEntityID();
	stream << L"\" ARGNUM=\"1\">\n";
	stream << L"      </rel_entity_arg>\n";
	
	stream << L"      <rel_entity_arg ENTITYID=\"";
	stream << doc_name << L"-E" << rel->getRightEntityID();
	stream << L"\" ARGNUM=\"2\">\n";
	stream << L"      </rel_entity_arg>\n";
	
	stream << L"      <relation_mentions>\n";
					
}

void EEMLResultCollector::_printEEMLEventHeader(UTF8OutputStream& stream, 
											Event* event,  
											bool isGeneric,
											CompletedFlag completeFlag,
											const wchar_t* doc_name)
{
	stream << L"    <entity ID=\"" << doc_name << L"-EV" << event->getID() << L"\">\n";
	stream << L"      <entity_type";
	if (isGeneric) 
		stream << L" GENERIC=\"TRUE\"";
	else
		stream << L" GENERIC=\"FALSE\"";

	if (completeFlag == COMPLETED) 
		stream << L" COMPLETE=\"Completed\"";
	else if (completeFlag == ATTEMPTED)
		stream << L" COMPLETE=\"Attempted\"";
	else 
		stream << L" COMPLETE=\"Failed\"";

	stream << L">" << event->getType().to_string() << L"</entity_type>\n";	
}




void EEMLResultCollector::_printEEMLEventMention(UTF8OutputStream& stream, 
											EventMention* eventMention,  
											Event* event,
											const wchar_t* doc_name)
{
	const SynNode* node = eventMention->getAnchorNode();
	const SynNode* head = node;
	int sentNum = eventMention->getSentenceNumber();

	stream << L"        <entity_mention TYPE=\"EVENT\" ID=\"EV" << 
		event->getID() << "-" << eventMention->getUID().toInt() << "\">\n";
	stream << L"          <extent>\n";
	_printEEMLMentionExtent(stream, node, sentNum);
	stream << L"          </extent>\n";
	stream << L"          <head>\n";
	_printEEMLMentionExtent(stream, head, sentNum);
	stream << L"          </head>\n";
	stream << L"        </entity_mention>\n";
}

void EEMLResultCollector::_printEEMLEERelationHeader(UTF8OutputStream& stream, 
													 EventEntityRelation* eeRelation, 
													 const wchar_t* doc_name) 
{
	stream << L"    <relation ID=\"" << doc_name << L"-EER" << eeRelation->getUID() << L"\"";
	stream << L" TYPE=\"" << eeRelation->getType().to_string() << "\">\n";
	
	stream << L"      <rel_entity_arg ENTITYID=\"" << doc_name << "-EV" << eeRelation->getEventID() << "\" ARGNUM=\"1\"/>\n";
	stream << L"      <rel_entity_arg ENTITYID=\"" << doc_name << "-E" << eeRelation->getEntityID() << "\" ARGNUM=\"2\"/>\n";
	stream << L"      <relation_mentions>\n";
}

void EEMLResultCollector::_printEEMLEERelationMention(UTF8OutputStream& stream, 
													  EventMention* eventMention, 
													  const Mention* mention,
													  EventEntityRelation* eeRelation)
{
	// if the mention is not valid for output, first try for parent, but
	// then give up and pick ANY mention of this entity
	if (!_isValidMention[mention->getUID()]) {
		SessionLogger::warn("eeml")
			<< "EEMLResultCollector::_printEEMLEERelationMention: Invalid mention referenced "
			<< "in output:\n"
			<< mention->getNode()->toDebugTextString().c_str() << "\n";

		const Mention *parent = mention->getParent();
		if (parent != 0 &&
			_isValidMention[parent->getUID()] &&
			_entitySet->getEntityByMention(mention->getUID()) ==
			_entitySet->getEntityByMention(parent->getUID()))
		{
			mention = parent;
		} else {
			Entity *ent = _entitySet->getEntityByMention(mention->getUID());
			GrowableArray<MentionUID> &mens = ent->mentions;
			for	(int j = 0;	j <	mens.length(); j++)	{
				Mention* ment =	_entitySet->getMention(mens[j]);
				if (_isValidMention[ment->getUID()]) {
					mention = ment;
					break;
				}
			}
		}
	}

	stream << L"        <relation_mention ID=\"EER" << eeRelation->getUID() << "-" 
		<< eventMention->getUID().toInt() << "-" << mention->getUID().toInt() << "\">\n";
	stream << L"          <rel_mention_arg MENTIONID=\"" << "EV" << eeRelation->getEventID() 
		<< "-" << eventMention->getUID().toInt() << "\" ARGNUM=\"1\">\n";
	_printEEMLNodeText(stream, eventMention->getAnchorNode(), eventMention->getSentenceNumber());
	stream << L"          </rel_mention_arg>\n";
	stream << L"          <rel_mention_arg MENTIONID=\"" << eeRelation->getEntityID() 
		<< "-" << mention->getUID() << "\" ARGNUM=\"2\">\n";
	stream << L"          </rel_mention_arg>\n";
	_printEEMLNodeText(stream, mention->getNode(), mention->getSentenceNumber());
	stream << L"        </relation_mention>\n";

}



void EEMLResultCollector::_printEEMLRelMention(UTF8OutputStream& stream,
									   RelMention* ment,
									   Relation* rel)
{

	stream << L"        <relation_mention ID=\"";
	stream << rel->getID() << L"-" << ment->getUID().toInt() << L"\">\n";

	const Mention *left = ment->getLeftMention();
	const Mention *right = ment->getRightMention();

	// if the mention is not valid for output, first try for parent, but
	// then give up and pick ANY mention of this entity
	if (!_isValidMention[left->getUID()]) {
        ostringstream ostr;
		ostr << "EEMLResultCollector::_printEEMLRelMention: Invalid mention referenced "
			<< "in output:\n"
			<< left->getUID() << " -- "
			<< left->getNode()->toDebugTextString().c_str() << "\n";

		const Mention *parent = left->getParent();
		if (parent != 0 &&
			_isValidMention[parent->getUID()] &&
			_entitySet->getEntityByMention(left->getUID()) ==
			_entitySet->getEntityByMention(parent->getUID()))
		{
			left = parent;
		} else {
			Entity *ent = _entitySet->getEntityByMention(left->getUID());
			GrowableArray<MentionUID> &mens = ent->mentions;
			for	(int j = 0;	j <	mens.length(); j++)	{
				Mention* new_ment =	_entitySet->getMention(mens[j]);
				if (_isValidMention[new_ment->getUID()]) {
					left = new_ment;
					break;
				}
			}
		}		
		ostr << "replaced with: " << left->getUID() << "\n";
		SessionLogger::warn("eeml") << ostr.str();
	}
	if (!_isValidMention[right->getUID()]) {
        ostringstream ostr;
		ostr
			<< "EEMLResultCollector::_printEEMLRelMention: Invalid mention referenced "
			<< "in output:\n"
			<< right->getUID() << " -- "
			<< right->getNode()->toDebugTextString().c_str() << "\n";

		const Mention *parent = right->getParent();
		if (parent != 0 &&
			_isValidMention[parent->getUID()] &&
			_entitySet->getEntityByMention(right->getUID()) ==
			_entitySet->getEntityByMention(parent->getUID()))
		{
			right = parent;
		} else {
			Entity *ent = _entitySet->getEntityByMention(right->getUID());
			GrowableArray<MentionUID> &mens = ent->mentions;
			for	(int j = 0;	j <	mens.length(); j++)	{
				Mention* new_ment =	_entitySet->getMention(mens[j]);
				if (_isValidMention[new_ment->getUID()]) {
					right = new_ment;
					break;
				}
			}
		}
		ostr << "replaced with: " << right->getUID() << "\n";
		SessionLogger::warn("eeml") << ostr.str();
	}

	stream << L"          <rel_mention_arg MENTIONID=\"";
	stream << rel->getLeftEntityID() << L"-" << left->getUID();
	stream << L"\" ARGNUM=\"1\">\n";
	_printEEMLNodeText(stream, left->getNode(), left->getSentenceNumber());
	stream << L"          </rel_mention_arg>\n";
	
	stream << L"          <rel_mention_arg MENTIONID=\"";
	stream << rel->getRightEntityID() << L"-" << right->getUID();
	stream << L"\" ARGNUM=\"2\">\n";
	_printEEMLNodeText(stream, right->getNode(), right->getSentenceNumber());
	stream << L"          </rel_mention_arg>\n";
		
	// TODO: add time

	stream << L"        </relation_mention>\n";

}


void EEMLResultCollector::_printEEMLEERelationFooter(UTF8OutputStream& stream)
{
	stream << L"      </relation_mentions>\n";
	stream << L"    </relation>\n\n";
}
void EEMLResultCollector::_printEEMLEntityFooter(UTF8OutputStream& stream)
{
	stream << L"    </entity>\n\n";
}

void EEMLResultCollector::_printEEMLRelationFooter(UTF8OutputStream& stream)
{
	stream << L"      </relation_mentions>\n";
	stream << L"    </relation>\n\n";
}

void EEMLResultCollector::_printEEMLDocumentFooter(UTF8OutputStream& stream)
{
	stream << L"  </document>\n";
	stream << L"</source_file>\n";
}

void EEMLResultCollector::_printEEMLEventFooter(UTF8OutputStream& stream)
{
	stream << L"    </entity>\n\n";
}
bool EEMLResultCollector::_isTopMention(Mention* ment, EntityType type)
{
	if (ment->getParent() == 0)
		return true;
	Entity* ent = _entitySet->getEntityByMention(ment->getUID(), type);
	Entity* parentEnt = _entitySet->getEntityByMention(ment->getParent()->getUID(), type);
	if (ent == 0) {
		string err = "current mention has no entity!\nAt node:\n";
		err.append(ment->node->toDebugString(0));
		err.append("\n");
		err.append("Mention was ");
		err.append("(");
		err.append(ment->getEntityType().getName().to_debug_string());
		err.append(" - ");
		err.append(Mention::getTypeString(ment->mentionType));
		err.append(")\n");
		throw InternalInconsistencyException("EEMLResultCollector::_isTopMention()",
			(char*)err.c_str());

	}
	if (parentEnt != 0 && parentEnt->getID() == ent->getID())
		return false;
	return true;
}

// is the mention structurally nested inside of a mention that is not a gpe?
// NOTE: the old comments say "return true if the mention is a name nested inside
// of a name that isn't a GPE." But the old code actually returns false if the first
// mention is a name - so it looks like it wants to avoid a desc/pron nested inside of
// a name!
bool EEMLResultCollector::_isNestedInsideOfName(Mention* ment)
{
	// TODO: is this mention hierarchy what I really want?
	if (ment->mentionType == Mention::NAME)
		return false;
	return _isNestedDriver(ment->node, ment->getSentenceNumber());

}

bool EEMLResultCollector::_isNestedDriver(const SynNode* node, int sentNum)
{
	const SynNode* parent = node->getParent();
	if (parent == 0)
		return false;
	if (parent->hasMention()) {
		Mention* parMent = _entitySet->getMentionSet(sentNum)->getMentionByNode(parent);
		if (parMent->isPopulated() && 
			parMent->mentionType == Mention::NAME && 
			!parMent->getEntityType().matchesGPE())
			return true;
	}
	return _isNestedDriver(parent, sentNum);
}

bool EEMLResultCollector::_isSecondPartOfAppositive(Mention* ment)
{
	Mention* parent = ment->getParent();
	if (parent == 0)
		return false;
	if (parent->mentionType != Mention::APPO)
		return false;
	// second element is appositive's child's next
	Mention* c1 = parent->getChild();
	if (c1 == 0)
		return false;
	Mention* c2 = c1->getNext();
	if (c2 == 0)
		return false;
	if (c2 == ment)
		return true;
	return false;
}

// TODO: need role information before this is useful
bool EEMLResultCollector::_isGPEModifierOfPersonGPE(Mention* ment)
{
	return false;
}

// mention must be a name, must have a parent whose edt head is different
// and must have a parent whose entity is different
bool EEMLResultCollector::_isNameNotInHeadOfParent(Mention* ment, EntityType type)
{
	if (ment->mentionType != Mention::NAME)
		return false;
	const SynNode* node = ment->node;
	const SynNode* parent = node->getParent();
	if (parent != 0 && parent->hasMention()) {		
		Mention* parMent = _entitySet->getMentionSet(ment->getSentenceNumber())->getMentionByNode(parent);
		// check head
		if (_getEDTHead(parMent) == node)
			return false;
		// if possible, check entities
		if (parMent->isPopulated()) {
			Entity* parEnt = _entitySet->getEntityByMention(parMent->getUID(), type);
			if (parEnt == 0) // parent is not an entity
				return true;
			Entity* ent = _entitySet->getEntityByMention(ment->getUID(), type);
			if (ent == 0)
				throw InternalInconsistencyException("EEMLResultCollector::_isNameNotInHeadOfParent()",
													 "current mention has no entity!");
			if (parEnt->getID() == ent->getID())
				return false;
		}
	}
	return true;
}

const SynNode* EEMLResultCollector::_getEDTHead(Mention* ment)
{
	if (ment == 0)
		throw InternalInconsistencyException("EEMLResultCollector::_getEDTHead()",
												 "mention does not exist");

	// DATE/TIMEs don't have separate heads!
	if (ment->getEntityType().isTemp())
		return ment->node;

	const SynNode* node = ment->node;
	if (ment->mentionType == Mention::NAME) {
		Mention* menChild = ment;
		const SynNode* head = menChild->node;
		while((menChild = menChild->getChild()) != 0)
			head = menChild->node;			
		return head;
	}
	if (ment->mentionType == Mention::APPO) {
		Mention* menChild = ment->getChild();
		if (menChild == 0)
			throw InternalInconsistencyException("EEMLResultCollector::_getEDTHead()",
												 "appositive has no children");
		return _getEDTHead(menChild);
	}
	// TODO: city/state hack : return the city, not the state

	// descend until we either come upon another mention or a preterminal
	if (!node->isPreterminal())
		do {
			node = node->getHead();
		} while (!node->isPreterminal() && !node->hasMention());
	if (node->isPreterminal())
		return node;
	return _getEDTHead(_entitySet->getMentionSet(ment->getSentenceNumber())->getMentionByNode(node));
}

bool EEMLResultCollector::_isPluralEntity(Entity *ent) {
	if (ent->getType().isTemp())
		return false;
	int nments = ent->getNMentions();	
	for (int i = 0; i < nments; i++) {
		Mention *ment = _entitySet->getMention(ent->getMention(i));
		int number = GroupFnGuesser::isPlural(ment);
		if (number == GroupFnGuesser::PLURAL)
			return true;
		else if (number == GroupFnGuesser::SINGULAR)
			return false;
	}
	return false;	
}
