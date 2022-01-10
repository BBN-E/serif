// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/apf/APFResultCollector.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/OStringStream.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/EventSet.h"
#include "Generic/theories/EventEntityRelation.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/RelationSet.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/theories/Relation.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/common/SessionLogger.h"
#include <sstream>
//#include "Generic/common/StringTransliterator.h"

using namespace std;


void APFResultCollector::loadDocTheory(DocTheory* theory) {
	finalize(); // get rid of old stuff

	_docTheory = theory;
	_entitySet = theory->getEntitySet();
	_relationSet = theory->getRelationSet();
	int numSents = theory->getNSentences();
	_tokenSequence = _new const TokenSequence*[numSents];
	for (int i = 0; i < numSents; i++)
		_tokenSequence[i] = theory->getSentenceTheory(i)->getTokenSequence();
}

void APFResultCollector::produceOutput(std::wstring *results) {
	OStringStream stream(*results);
	produceOutput(stream);
}

void APFResultCollector::produceOutput(const wchar_t *output_dir,
									   const wchar_t *doc_filename)
{
	wstring output_file = wstring(output_dir) + LSERIF_PATH_SEP + wstring(doc_filename) + L".apf";
	
	UTF8OutputStream stream;
	stream.open(output_file.c_str());
	produceOutput(stream);
}
	
// WARNING: read the memory warning below before making changes to this method!
void APFResultCollector::produceOutput(OutputStream &stream) {
	bool *isEntityPrinted = NULL;
	if (_entitySet != NULL) {
		isEntityPrinted = _new bool[_entitySet->getNEntities()];
		for (int i = 0; i < _entitySet->getNEntities(); i++)
			isEntityPrinted[i] = false;
	}
	// NOTE: these two were options that were both false in the last ace run
	//       I'm assuming we don't do them anymore
	// TODO: from original code - remove unlinked descriptors
	// TODO: from original code - remove pronoun chains

	const wchar_t* docName = _docTheory->getDocument()->getName().to_string();

	// TODO: we need a way to figure source out!
	const wchar_t* source = L"newspaper";
	_printAPFDocumentHeader(stream, docName, source);
	if (_entitySet != NULL) {
		int ents = _entitySet->getNEntities();
		
		for (int i = 0; i < ents; i++) {
			Entity* ent = _entitySet->getEntity(i);
			if (!ent->getType().isRecognized())
				continue;
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
				// really odd if mentions aren't of	the	type of	their entity
				if (!ment->getEntityType().isRecognized()) {
					string err = "mention in edt entity	not	of edt type!\nWhile	creating: ";
					err.append(UnicodeUtil::toUTF8StdString(stream.getFileName()));
					err.append("\nAt node:\n");
					err.append(ment->node->toDebugString(0));
					err.append("\n");
					err.append("Entity was ");
	//				err.append(ent->getID());
					err.append(" (");
					err.append(ent->getType().getName().to_debug_string());
					err.append("), mention was ");
	//				err.append(ment->ID);
					err.append(" (");
					err.append(ment->getEntityType().getName().to_debug_string());
					err.append(" - ");
					err.append(Mention::getTypeString(ment->mentionType));
					err.append(")\n");
					throw InternalInconsistencyException("APFResultCollector::produceAPFOutput()",
														(char*)err.c_str());
				}

				// we only pretty much want	solo, unnested mentions
				// an exception	is made, though, for appositive	members
				// and GPE modifiers of	GPE	entities with person role (? &&	TODO)

				if(_isPrintableMention(ment, ent)) {
						validMentions[valid_mentions_size++] = ment;
						// names in	the	normal situation are the only mentions added
						// to the name attributes section
						// JCS 3/12/04 - All names need to have a name attribute
						if (ment->getMentionType() == Mention::NAME) //&& !_isNameNotInHeadOfParent(ment, ent->getType()))
							validNames[valid_names_size++] = ment;
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
			bool seen_non_pronoun =	false;
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
			}


			_printAPFEntityHeader(stream, ent, ent->isGeneric(), docName);
			isEntityPrinted[ent->getID()] = true;

			// now print the mentions
			for	(j = 0;	j <	valid_mentions_size; j++) {
				_printAPFMention(stream, validMentions[j], ent);
			}
			// now print the names
			if (valid_names_size > 0) {
				_printAPFAttributeHeader(stream);
				for	(j = 0;	j <	valid_names_size; j++) {
					_printAPFNameAttribute(stream, validNames[j]);
				}
				_printAPFAttributeFooter(stream);
			}
							
			_printAPFEntityFooter(stream);

			// MEMORY: handle the arrays created earlier
			delete [] validMentions;
			delete [] validNames;
		}
	}

	// print events and event relations
	EventSet *eventSet = _docTheory->getEventSet();
	if (_includeEvents && eventSet != NULL) {
		int n_events = eventSet->getNEvents();
		for (int i = 0; i < n_events; i++) {
			Event *event = eventSet->getEvent(i);

			_printExAPFEventHeader(stream, event, false, COMPLETED, docName);

			Event::LinkedEventMention* linkedEventMention = event->getEventMentions();
			while (linkedEventMention != NULL ) {
				_printExAPFEventMention(stream, linkedEventMention->eventMention, event, docName);
				linkedEventMention = linkedEventMention->next;
			}
			_printExAPFEventFooter(stream);

			// for every event, print out the consolidated event entity relations
			int n_ee_relations = event->getNumConsolidatedEERelations();
			for (int j = 0; j < n_ee_relations; j++) {
				EventEntityRelation* eeRelation = event->getConsolidatedEERelation(j);
				//if (!_includeGenericEERelations && 
				//	!_printAllEventsAsSpecific &&
				//	isEntityGeneric[eeRelation->getEntityID()])
				//	continue;

				_printExAPFEERelationHeader(stream, eeRelation, docName);
				EventEntityRelation::LinkedEERelMention* linkedEERelMention = eeRelation->getEEMentions();
				while (linkedEERelMention != NULL) {
					_printExAPFEERelationMention(stream, linkedEERelMention->eventMention, linkedEERelMention->mention, eeRelation);
					linkedEERelMention = linkedEERelMention->next;
				}
				_printExAPFEERelationFooter(stream);
			}
		}
	}


	// end print events
	// TODO: generics go here
	if (_relationSet != NULL) {
		int n_rels = _relationSet->getNRelations();
		for (int i = 0; i < n_rels; i++) {
			Relation *rel = _relationSet->getRelation(i);
			//char buffer[1000];
			//StringTransliterator::transliterateToEnglish(buffer, 
			//	rel->getMentions()->relMention->toString().c_str(),
			//						   1000);
			//std::cerr << buffer << "\n";
			
			rel->getMentions()->relMention->toString();

			if (_entitySet->getEntity(rel->getLeftEntityID())->isGeneric() ||
				_entitySet->getEntity(rel->getRightEntityID())->isGeneric())
				continue;

			//std::cerr << "not generic\n";

			if (!isEntityPrinted[rel->getLeftEntityID()] ||
				!isEntityPrinted[rel->getRightEntityID()])
				continue;

			//std::cerr << "printed\n";
			// figure out if mentions were valid?
			
			const Relation::LinkedRelMention *mentions = rel->getMentions();
			if (mentions == 0)
				continue;
			
			_printAPFRelationHeader(stream, rel, docName);
			while (mentions != 0) {
				_printAPFRelMention(stream, mentions->relMention, rel);
				mentions = mentions->next;
			}
			_printAPFRelationFooter(stream);
		}
	}
	
	_printAPFDocumentFooter(stream);
	delete [] isEntityPrinted;
	stream.close();
}

const wchar_t* APFResultCollector::_convertMentionTypeToAPF(Mention* ment)
{
	Mention::Type type = ment->mentionType;
	Mention* subMent = 0;
	switch (type) {
		case Mention::NAME:
			return L"NAME";
		case Mention::DESC:
		case Mention::PART:
			return L"NOMINAL";
		case Mention::PRON:
			return L"PRONOUN";			
		case Mention::APPO:
			// for appositive, the type is the type of the first child
			subMent = ment->getChild();
			return _convertMentionTypeToAPF(subMent);
		case Mention::LIST:
			// for list, the type is the type of the first child
			subMent = ment->getChild();
			return _convertMentionTypeToAPF(subMent);
		default:
			return L"WARNING_BAD_MENTION_TYPE";
	}
}


// PRINTING NOTE: a former version used hard tabs here. for each former
//                hard tab, there are now two spaces.

void APFResultCollector::_printAPFDocumentHeader(OutputStream& stream,
											  const wchar_t* doc_name,
											  const wchar_t* doc_source)
{
	if(_includeXMLHeaderInfo) {
		stream << L"<?xml version=\"1.0\"?>\n";
		stream << L"<!DOCTYPE source_file SYSTEM \"ace-rdc.v2.0.1.dtd\">\n";
	}
	stream << L"<source_file SOURCE=\"" << doc_source 
		<< L"\" TYPE=\"text\" VERSION=\"2.0\" URI=\"" << doc_name
		<< L"\">\n";
	stream << L"  <document DOCID=\"" << doc_name << L"\">\n\n";
}

void APFResultCollector::_printAPFEntityHeader(OutputStream& stream, 
											  Entity* ent, 
											  bool isGeneric, 
											  const wchar_t* doc_name)
{
	stream << L"    <entity ID=\"";
	if(!_useAbbreviatedIDs)	stream << doc_name << L"-";
	stream << L"E" << ent->ID << L"\">\n";
	if(isGeneric) 
		stream << L"      <entity_type GENERIC=\"TRUE\">" << ent->type.getName().to_string() << L"</entity_type>\n";
	else
		stream << L"      <entity_type GENERIC=\"FALSE\">" << ent->type.getName().to_string() << L"</entity_type>\n";
}

void APFResultCollector::_printAPFAttributeHeader(OutputStream& stream) 
{
	stream << L"        <entity_attributes>\n";
}

void APFResultCollector::_printAPFAttributeFooter(OutputStream& stream) 
{
	stream << L"        </entity_attributes>\n";
}


void APFResultCollector::_printAPFMention(OutputStream& stream,
									   Mention* ment,
									   Entity* ent)
{
	// NOTE: in the rare case that we want to print a list as a mention,
	//       print each of its parts instead.
	if (ment->getMentionType() == Mention::LIST) {
		Mention* child = ment->getChild();
		while (child != 0) {
			_printAPFMention(stream, child, ent);
			child = child->getNext();
		}
		return;
	}


	stream << L"        <entity_mention TYPE=\"" << _convertMentionTypeToAPF(ment) << L"\"";
	stream << L" ID=\"";
	if(!_useAbbreviatedIDs)
		stream << ent->ID << L"-";
	stream << ment->getUID() << L"\"";
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
	const SynNode* head = ment->getEDTHead();
	stream << L"          <extent>\n";
	_printAPFMentionExtent(stream, node, sentNum);
	stream << L"          </extent>\n";
	stream << L"          <head>\n";
	_printAPFMentionExtent(stream, head, sentNum);
	stream << L"          </head>\n";
	stream << L"        </entity_mention>\n";

}

void APFResultCollector::_printAPFNameAttribute(OutputStream& stream,
											 Mention* ment)
{
	stream << L"          <name>\n";
	_printAPFMentionExtent(stream, ment->getEDTHead(), ment->getSentenceNumber());
	stream << L"          </name>\n";
}



void APFResultCollector::_printAPFMentionExtent(OutputStream& stream,
											 const SynNode* node,
											 int sentNum)
{
	stream << L"              <charseq>\n";
	_printAPFNodeText(stream, node, sentNum);

	int startTok = node->getStartToken();
	int endTok = node->getEndToken();
	int start = _tokenSequence[sentNum]->getToken(startTok)->getStartEDTOffset().value();
	int end = _tokenSequence[sentNum]->getToken(endTok)->getEndEDTOffset().value(); 
	stream << L"                <start>" << start << L"</start><end>" << end <<  L"</end></charseq>\n";
}

void APFResultCollector::_printAPFNodeText(OutputStream& stream, 
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
			stream << symStr;
		else {
			size_t orig_len = wcslen(symStr);
			// worst case, it's all dashes
			wchar_t* safeStr = _new wchar_t[2*orig_len + 1];
			size_t oldIdx = 0;
			size_t newIdx = 0;
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
		
	stream << L"\" -->\n";
}

void APFResultCollector::_printAPFRelationHeader(OutputStream& stream, 
											Relation* rel,  
											const wchar_t* doc_name)
{
	Symbol type = rel->getType();

	
	stream << L"    <relation ID=\"" << doc_name << L"-R" << rel->getID() << L"\"";
	stream << L" TYPE=\"" << RelationConstants::getBaseTypeSymbol(type).to_string();
	stream << L"\" SUBTYPE=\"" << RelationConstants::getSubtypeSymbol(type).to_string();
	stream << L"\" CLASS=\"EXPLICIT\">\n";

	stream << L"      <rel_entity_arg ENTITYID=\"";
	if(!_useAbbreviatedIDs)
		stream << doc_name << L"-";
	stream << L"E" << rel->getLeftEntityID();
	stream << L"\" ARGNUM=\"1\">\n";
	stream << L"      </rel_entity_arg>\n";
	
	stream << L"      <rel_entity_arg ENTITYID=\"";
	if(!_useAbbreviatedIDs)
		stream << doc_name << L"-";
	stream << L"E" << rel->getRightEntityID();
	stream << L"\" ARGNUM=\"2\">\n";
	stream << L"      </rel_entity_arg>\n";
	
	stream << L"      <relation_mentions>\n";
					
}

void APFResultCollector::_printAPFRelMention(OutputStream& stream,
									   RelMention* ment,
									   Relation* rel)
{

	stream << L"        <relation_mention ID=\"";
	stream << rel->getID() << L"-" << ment->getUID().toInt() << L"\">\n";

	stream << L"          <rel_mention_arg MENTIONID=\"";
	if(!_useAbbreviatedIDs)
		stream << rel->getLeftEntityID() << L"-";
	stream << ment->getLeftMention()->getUID();
	stream << L"\" ARGNUM=\"1\">\n";
	_printAPFNodeText(stream, ment->getLeftMention()->getNode(), 
		ment->getLeftMention()->getSentenceNumber());
	stream << L"          </rel_mention_arg>\n";
	
	stream << L"          <rel_mention_arg MENTIONID=\"";
	if(!_useAbbreviatedIDs)
		stream << rel->getRightEntityID() << L"-";
	stream << ment->getRightMention()->getUID();
	stream << L"\" ARGNUM=\"2\">\n";
	_printAPFNodeText(stream, ment->getRightMention()->getNode(),
		ment->getRightMention()->getSentenceNumber());
	stream << L"          </rel_mention_arg>\n";
		
	// TODO: add time

	stream << L"        </relation_mention>\n";

}


void APFResultCollector::_printAPFEntityFooter(OutputStream& stream)
{
	stream << L"    </entity>\n\n";
}

void APFResultCollector::_printAPFRelationFooter(OutputStream& stream)
{
	stream << L"      </relation_mentions>\n";
	stream << L"    </relation>\n\n";
}

void APFResultCollector::_printAPFDocumentFooter(OutputStream& stream)
{
	stream << L"  </document>\n";
	stream << L"</source_file>\n";
}

bool APFResultCollector::_isTopMention(const Mention* ment, EntityType type)
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
		throw InternalInconsistencyException("APFResultCollector::_isTopMention()",
			(char*)err.c_str());

	}
	if (parentEnt != 0 && parentEnt->getID() == ent->getID())
		return false;
	return true;
}

bool APFResultCollector::_isItemOfUnprintedList(const Mention* ment)
{
	if (ment->getParent() == 0)
		return false;

	// only applies to DESCs and NAMEs
	if (ment->getMentionType() != Mention::DESC && ment->getMentionType() != Mention::NAME)
		return false;
	
	if (ment->getParent()->getMentionType() == Mention::LIST) {
		Entity* parentEnt = _entitySet->getEntityByMention(ment->getParent()->getUID());
		if (parentEnt != 0 && _isPrintableMention(ment->getParent(), parentEnt))
			return false;
		return true;
	}
	return false;
}

// is the mention structurally nested inside of a mention that is not a gpe?
// NOTE: the old comments say "return true if the mention is a name nested inside
// of a name that isn't a GPE." But the old code actually returns false if the first
// mention is a name - so it looks like it wants to avoid a desc/pron nested inside of
// a name!
bool APFResultCollector::_isNestedInsideOfName(const Mention* ment)
{
	// TODO: is this mention hierarchy what I really want?
	if (ment->getMentionType() == Mention::NAME)
		return false;
	return _isNestedDriver(ment->node, ment->getSentenceNumber());

}

bool APFResultCollector::_isNestedDriver(const SynNode* node, int sentNum)
{
	const SynNode* parent = node->getParent();
	if (parent == 0)
		return false;
	if (parent->hasMention()) {
		Mention* parMent = _entitySet->getMentionSet(sentNum)->getMentionByNode(parent);
		if (parMent->isPopulated() && 
			parMent->getMentionType() == Mention::NAME && 
			!parMent->getEntityType().matchesGPE())
			return true;
	}
	return _isNestedDriver(parent, sentNum);
}

bool APFResultCollector::_isSecondPartOfAppositive(const Mention* ment)
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
bool APFResultCollector::_isGPEModifierOfPersonGPE(const Mention* ment)
{
	return false;
}

// mention must be a name, must have a parent whose edt head is different
// and must have a parent whose entity is different
bool APFResultCollector::_isNameNotInHeadOfParent(const Mention* ment, EntityType type)
{
	if (ment->mentionType != Mention::NAME)
		return false;
	const SynNode* node = ment->node;
	const SynNode* parent = node->getParent();
	if (parent != 0 && parent->hasMention()) {		
		Mention* parMent = _entitySet->getMentionSet(ment->getSentenceNumber())->getMentionByNode(parent);
		// check head
		if (parMent->getEDTHead() == node)
			return false;
		// if possible, check entities
		if (parMent->isPopulated()) {
			Entity* parEnt = _entitySet->getEntityByMention(parMent->getUID(), type);
			if (parEnt == 0) // parent is not an entity
				return true;
			Entity* ent = _entitySet->getEntityByMention(ment->getUID(), type);
			if (ent == 0)
				throw InternalInconsistencyException("APFResultCollector::_isNameNotInHeadOfParent()",
													 "current mention has no entity!");
			if (parEnt->getID() == ent->getID())
				return false;
		}
	}
	return true;
}

void APFResultCollector::_printExAPFEventHeader(OutputStream& stream, 
											Event* event,  
											bool isGeneric,
											CompletedFlag completeFlag,
											const wchar_t* doc_name)
{
	stream << L"    <entity ID=\"";
	if(!_useAbbreviatedIDs)
		stream << doc_name << L"-";
	stream << L"EV" << event->getID() << L"\">\n";
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




void APFResultCollector::_printExAPFEventMention(OutputStream& stream, 
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
	_printAPFMentionExtent(stream, node, sentNum);
	stream << L"          </extent>\n";
	stream << L"          <head>\n";
	_printAPFMentionExtent(stream, head, sentNum);
	stream << L"          </head>\n";
	stream << L"        </entity_mention>\n";
}

void APFResultCollector::_printExAPFEERelationHeader(OutputStream& stream, 
													 EventEntityRelation* eeRelation, 
													 const wchar_t* doc_name) 
{
	stream << L"    <relation ID=\"";
	if(!_useAbbreviatedIDs)
		stream << doc_name << L"-";
	stream << L"EER" << eeRelation->getUID() << L"\"";
	stream << L" TYPE=\"" << eeRelation->getType().to_string() << "\">\n";
	
	stream << L"      <rel_entity_arg ENTITYID=\"";
	if(!_useAbbreviatedIDs)
		stream << doc_name << "-";
	stream << L"EV" << eeRelation->getEventID() << "\" ARGNUM=\"1\"/>\n";
	stream << L"      <rel_entity_arg ENTITYID=\""; 
	if(!_useAbbreviatedIDs)
		stream << doc_name << L"-";
	stream << L"E" << eeRelation->getEntityID() << "\" ARGNUM=\"2\"/>\n";
	stream << L"      <relation_mentions>\n";
}

void APFResultCollector::_printExAPFEERelationMention(OutputStream& stream, 
													  EventMention* eventMention, 
													  const Mention* mention,
													  EventEntityRelation* eeRelation)
{
	// if the mention is not valid for output, first try for parent, but
	// then give up and pick ANY mention of this entity
	if (!_isPrintableMention(mention, _entitySet->getEntityByMention(mention->getUID()))) {
		SessionLogger::warn("apf")
			<< "APFResultCollector::_printExAPFEERelationMention: Invalid mention referenced "
			<< "in output:\n"
			<< mention->getNode()->toDebugTextString().c_str() << "\n";

		const Mention *parent = mention->getParent();
		if (parent != 0 &&
			_isPrintableMention(parent, _entitySet->getEntityByMention(parent->getUID())) &&
			_entitySet->getEntityByMention(mention->getUID()) ==
			_entitySet->getEntityByMention(parent->getUID()))
		{
			mention = parent;
		} else {
			Entity *ent = _entitySet->getEntityByMention(mention->getUID());
			GrowableArray<MentionUID> &mens = ent->mentions;
			for	(int j = 0;	j <	mens.length(); j++)	{
				Mention* ment =	_entitySet->getMention(mens[j]);
				if (_isPrintableMention(ment, ent)) {
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
	_printAPFNodeText(stream, eventMention->getAnchorNode(), eventMention->getSentenceNumber());
	stream << L"          </rel_mention_arg>\n";
	stream << L"          <rel_mention_arg MENTIONID=\"" << eeRelation->getEntityID() 
		<< "-" << mention->getUID() << "\" ARGNUM=\"2\">\n";
	stream << L"          </rel_mention_arg>\n";
	_printAPFNodeText(stream, mention->getNode(), mention->getSentenceNumber());
	stream << L"        </relation_mention>\n";

}

void APFResultCollector::_printExAPFEERelationFooter(OutputStream& stream)
{
	stream << L"      </relation_mentions>\n";
	stream << L"    </relation>\n\n";
}

void APFResultCollector::_printExAPFEventFooter(OutputStream& stream)
{
	stream << L"    </entity>\n\n";
}

bool APFResultCollector::_isPrintableMention(const Mention *ment, Entity *ent) {
	return (_isSecondPartOfAppositive(ment)	|| 
			_isGPEModifierOfPersonGPE(ment)	||
			_isTopMention(ment, ent->getType()) ||
			_isNameNotInHeadOfParent(ment, ent->getType()) ||
			_isItemOfUnprintedList(ment));
}
