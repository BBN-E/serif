// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "Generic/CASerif/correctanswers/CorrectAnswers.h"
#include "Generic/CASerif/results/CAResultCollectorAPF.h"
#include "theories/SentenceTheory.h"
#include "theories/DocTheory.h"
#include "common/GrowableArray.h"
#include "common/InternalInconsistencyException.h"
#include "common/UTF8OutputStream.h"
#include "common/UnicodeUtil.h"
#include "theories/Mention.h"
#include "theories/Entity.h"
#include "theories/SynNode.h"
#include "theories/Token.h"
#include "theories/EntitySet.h"
#include "theories/RelationSet.h"
#include "theories/RelationConstants.h"
#include "theories/Relation.h"
#include "theories/RelMention.h"
#include "theories/TokenSequence.h"

using namespace std;


void CAResultCollectorAPF::loadDocTheory(DocTheory* theory) {
	finalize(); // get rid of old stuff

	_docTheory = theory;
	_entitySet = theory->getEntitySet();
	_relationSet = theory->getRelationSet();
	_correctDocument = CorrectAnswers::getInstance().getCorrectDocument(theory->getDocument()->getName());
	int numSents = theory->getNSentences();
	_tokenSequence = _new const TokenSequence*[numSents];
	for (int i = 0; i < numSents; i++)
		_tokenSequence[i] = theory->getSentenceTheory(i)->getTokenSequence();
}

// WARNING: read the memory warning below before making changes to this method!
void CAResultCollectorAPF::produceOutput(const wchar_t *output_dir,
                                         const wchar_t *doc_filename)
{
	wstring output_file = wstring(output_dir) + LSERIF_PATH_SEP + wstring(doc_filename) + L".apf";

	UTF8OutputStream stream;
	stream.open(output_file.c_str());
	
	// memory?
	bool *isEntityPrinted = _new bool[_entitySet->getNEntities()];
	for (int i = 0; i < _entitySet->getNEntities(); i++)
		isEntityPrinted[i] = false;

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
			GrowableArray<Mention::UID> &mens = ent->mentions;

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
					throw InternalInconsistencyException("CAResultCollectorAPF::produceAPFOutput()",
														(char*)err.c_str());
				}

				// we only pretty much want	solo, unnested mentions
				// an exception	is made, though, for appositive	members
				// and GPE modifiers of	GPE	entities with person role (? &&	TODO)
				if (ment->isPopulated() && 
					(_isSecondPartOfAppositive(ment)	|| 
					_isGPEModifierOfPersonGPE(ment)	||
//					(_isTopMention(ment) &&	!_isNestedInsideOfName(ment))) {				
					(_isTopMention(ment, ent->getType())))) {				
						validMentions[valid_mentions_size++] = ment;
						// names in	the	normal situation are the only mentions added
						// to the name attributes section
						if (ment->mentionType == Mention::NAME)
							validNames[valid_names_size++] = ment;
					}
				else if	(_isNameNotInHeadOfParent(ment, ent->getType())) {
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
			
			// EMB: should still print out just pronouns for CA,
			//  so, since this is a CA file, let's comment out this next part
			
			/*bool seen_non_pronoun =	false;
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


			CorrectEntity *ce = _correctDocument->getCorrectEntityFromEntityID(ent->getID());
			bool is_generic;
			if (ce && ce->isGeneric()) 
				is_generic = true;
			else 
				is_generic = false;

			_printAPFEntityHeader(stream, ent, is_generic, docName);
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

	if (_relationSet != NULL) {
		int n_rels = _relationSet->getNRelations();
		for (int i = 0; i < n_rels; i++) {
			Relation *rel = _relationSet->getRelation(i);
			
			CorrectEntity *leftCE = _correctDocument->getCorrectEntityFromEntityID(rel->getLeftEntityID());
			CorrectEntity *rightCE = _correctDocument->getCorrectEntityFromEntityID(rel->getRightEntityID());

			if (((leftCE && leftCE->isGeneric()) ||
				 (rightCE && rightCE->isGeneric())))
				 continue;

			if (!isEntityPrinted[rel->getLeftEntityID()] ||
				!isEntityPrinted[rel->getRightEntityID()])
				continue;

			// figure out if mentions were valid?
			const Relation::LinkedRelMention *mentions = rel->getMentions();
			if (mentions == 0)
				continue;
			
			_printAPFRelationHeader(stream, rel, docName);
			while (mentions != 0) {
				_printCorrectRelMention(stream, mentions->relMention, rel);
				mentions = mentions->next;
			}
			_printAPFRelationFooter(stream);
		}
	}
	
	_printAPFDocumentFooter(stream);
	delete [] isEntityPrinted;
	stream.close();
}

const wchar_t* CAResultCollectorAPF::_convertMentionTypeToAPF(Mention* ment)
{

	// use the type from the CorrectMention if available
	CorrectMention *cm = _correctDocument->getCorrectMentionFromMentionID(ment->getUID());
	if (cm != NULL) {
		if (cm->isName())
			return L"NAME";
		else if (cm->isNominal())
			return L"NOMINAL";
		else if (cm->isPronoun())
			return L"PRONOUN";
	}	

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
			return L"NOMINAL";
		default:
			return L"WARNING_BAD_MENTION_TYPE";
	}
}


// PRINTING NOTE: a former version used hard tabs here. for each former
//                hard tab, there are now two spaces.

void CAResultCollectorAPF::_printAPFDocumentHeader(UTF8OutputStream& stream,
											  const wchar_t* doc_name,
											  const wchar_t* doc_source)
{
	stream << L"<?xml version=\"1.0\"?>\n";
	stream << L"<!DOCTYPE source_file SYSTEM \"ace-rdc.v2.0.1.dtd\">\n";
	stream << L"<source_file SOURCE=\"" << doc_source 
		   << L"\" TYPE=\"text\" VERSION=\"2.0\" URI=\"" << doc_name
		   << L"\">\n";
	stream << L"  <document DOCID=\"" << doc_name << L"\">\n\n";
}

void CAResultCollectorAPF::_printAPFEntityHeader(UTF8OutputStream& stream, 
											  Entity* ent, 
											  bool isGeneric, 
											  const wchar_t* doc_name)
{
	stream << L"    <entity ID=\"" << doc_name << L"-E" << ent->ID << L"\">\n";
	if(isGeneric) 
		stream << L"      <entity_type GENERIC=\"TRUE\">" << ent->type.getName().to_string() << L"</entity_type>\n";
	else
		stream << L"      <entity_type GENERIC=\"FALSE\">" << ent->type.getName().to_string() << L"</entity_type>\n";
}

void CAResultCollectorAPF::_printAPFAttributeHeader(UTF8OutputStream& stream) 
{
	stream << L"        <entity_attributes>\n";
}

void CAResultCollectorAPF::_printAPFAttributeFooter(UTF8OutputStream& stream) 
{
	stream << L"        </entity_attributes>\n";
}

void CAResultCollectorAPF::_printCorrectMention(UTF8OutputStream& stream,
									   CorrectMention* cm,
									   Entity* ent)

{
	const wchar_t * mentionType = cm->getCorrectMentionType().to_string();
	
	stream << L"        <entity_mention TYPE=\"" << mentionType << L"\"";
	stream << L" ID=\"CM" << ent->ID << L"-" << cm->getAnnotationID().to_string() << L"\"";
	stream << L">\n";
	int sentNum = cm->getSentenceNumber();
	if (sentNum != -1) {
		cm->setIsPrintedFlag();
		stream << L"          <extent>\n";
		_printCorrectMentionExtent(stream, cm, sentNum);
		stream << L"          </extent>\n";
		stream << L"          <head>\n";
		_printCorrectMentionHeadExtent(stream, cm, sentNum);
		stream << L"          </head>\n";
	}
	stream << L"        </entity_mention>\n";
}

void CAResultCollectorAPF::_printAPFMention(UTF8OutputStream& stream,
									   Mention* ment,
									   Entity* ent)
{
	// JCS - 4/8/04 - This seems to be messing up some of the cases where the CorrectMention
	// is classified as a LIST, so I'm taking it out.
	// NOTE: in the rare case that we want to print a list as a mention,
	//       print each of its parts instead.
	/*if (ment->mentionType == Mention::LIST) {
		Mention* child = ment->getChild();
		while (child != 0) {
			_printAPFMention(stream, child, ent);
			child = child->getNext();
		}
		return;
	}*/


	stream << L"        <entity_mention TYPE=\"" << _convertMentionTypeToAPF(ment) << L"\"";
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
	CorrectMention *cm = _correctDocument->getCorrectMentionFromMentionID(ment->getUID());

	if (cm != NULL && !cm->isPrinted()) {
		int sentNum = ment->getSentenceNumber();
		cm->setIsPrintedFlag();
		stream << L"          <extent>\n";
		_printCorrectMentionExtent(stream, cm, sentNum);
		stream << L"          </extent>\n";
		stream << L"          <head>\n";
		_printCorrectMentionHeadExtent(stream, cm, sentNum);
		stream << L"          </head>\n";
		stream << L"        </entity_mention>\n";
	}
	else {
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
}

void CAResultCollectorAPF::_printAPFNameAttribute(UTF8OutputStream& stream,
											 Mention* ment)
{
	stream << L"          <name>\n";
	_printAPFMentionExtent(stream, ment->getEDTHead(), ment->getSentenceNumber());
	stream << L"          </name>\n";
}



void CAResultCollectorAPF::_printAPFMentionExtent(UTF8OutputStream& stream,
											 const SynNode* node,
											 int sentNum)
{
	stream << L"              <charseq>\n";
	_printAPFNodeText(stream, node, sentNum);

	int startTok = node->getStartToken();
	int endTok = node->getEndToken();
	EDTOffset start = _tokenSequence[sentNum]->getToken(startTok)->getStartEDTOffset();
	EDTOffset end = _tokenSequence[sentNum]->getToken(endTok)->getEndEDTOffset(); 
	stream << L"                <start>" << start << L"</start><end>" << end <<  L"</end></charseq>\n";
}

void CAResultCollectorAPF::_printCorrectMentionExtent(UTF8OutputStream& stream,
												   CorrectMention *cm,
												   int sentNum)
{
	stream << L"              <charseq>\n";
	_printCorrectMentionText(stream, cm, sentNum);

	int startTok = cm->getStartToken();
	int endTok = cm->getEndToken();
	EDTOffset start = _tokenSequence[sentNum]->getToken(startTok)->getStartEDTOffset();
	EDTOffset end = _tokenSequence[sentNum]->getToken(endTok)->getEndEDTOffset(); 
	stream << L"                <start>" << start << L"</start><end>" << end <<  L"</end></charseq>\n";
}

void CAResultCollectorAPF::_printCorrectMentionHeadExtent(UTF8OutputStream& stream,
												   CorrectMention *cm,
												   int sentNum)
{
	stream << L"              <charseq>\n";
	_printCorrectMentionHeadText(stream, cm, sentNum);

	int startTok = cm->getHeadStartToken();
	int endTok = cm->getHeadEndToken();
	EDTOffset start = _tokenSequence[sentNum]->getToken(startTok)->getStartEDTOffset();
	EDTOffset end = _tokenSequence[sentNum]->getToken(endTok)->getEndEDTOffset(); 
	stream << L"                <start>" << start << L"</start><end>" << end <<  L"</end></charseq>\n";
}

void CAResultCollectorAPF::_printAPFNodeText(UTF8OutputStream& stream, 
		const SynNode* node,
		int sentNum) 
{

	int startTok = static_cast<int>(node->getStartToken());
	int endTok = static_cast<int>(node->getEndToken());
	EDTOffset start = _tokenSequence[sentNum]->getToken(startTok)->getStartEDTOffset();
	EDTOffset end = _tokenSequence[sentNum]->getToken(endTok)->getEndEDTOffset(); 

	_printAPFNodeText(stream, sentNum, startTok, endTok, start, end);
}
	

void CAResultCollectorAPF::_printAPFNodeText(UTF8OutputStream& stream, 
		int sentNum,
		int startTok,
		int endTok,
		EDTOffset start,
		EDTOffset end) 
{
	
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
		
	stream << L"\"-->\n";
}

void CAResultCollectorAPF::_printCorrectMentionText(UTF8OutputStream& stream, 
		CorrectMention *cm,
		int sentNum) 
{
	int startTok = cm->getStartToken();
	int endTok = cm->getEndToken();
	EDTOffset start = _tokenSequence[sentNum]->getToken(startTok)->getStartEDTOffset();
	EDTOffset end = _tokenSequence[sentNum]->getToken(endTok)->getEndEDTOffset(); 

	_printAPFNodeText(stream, sentNum, startTok, endTok, start, end);
}

void CAResultCollectorAPF::_printCorrectMentionHeadText(UTF8OutputStream& stream, 
		CorrectMention *cm,
		int sentNum) 
{
	int startTok = cm->getHeadStartToken();
	int endTok = cm->getHeadEndToken();
	EDTOffset start = _tokenSequence[sentNum]->getToken(startTok)->getStartEDTOffset();
	EDTOffset end = _tokenSequence[sentNum]->getToken(endTok)->getEndEDTOffset();

	_printAPFNodeText(stream, sentNum, startTok, endTok, start, end);
}

void CAResultCollectorAPF::_printAPFRelationHeader(UTF8OutputStream& stream, 
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

void CAResultCollectorAPF::_printAPFRelMention(UTF8OutputStream& stream,
									   RelMention* ment,
									   Relation* rel)
{

	stream << L"        <relation_mention ID=\"";
	stream << rel->getID() << L"-" << ment->getUID().toInt() << L"\">\n";

	stream << L"          <rel_mention_arg MENTIONID=\"";
	stream << rel->getLeftEntityID() << L"-" << ment->getLeftMention()->getUID();
	stream << L"\" ARGNUM=\"1\">\n";
	_printAPFNodeText(stream, ment->getLeftMention()->getNode(), 
		ment->getLeftMention()->getSentenceNumber());
	stream << L"          </rel_mention_arg>\n";
	
	stream << L"          <rel_mention_arg MENTIONID=\"";
	stream << rel->getRightEntityID() << L"-" << ment->getRightMention()->getUID();
	stream << L"\" ARGNUM=\"2\">\n";
	_printAPFNodeText(stream, ment->getRightMention()->getNode(),
		ment->getRightMention()->getSentenceNumber());
	stream << L"          </rel_mention_arg>\n";
		
	// TODO: add time

	stream << L"        </relation_mention>\n";

}

void CAResultCollectorAPF::_printCorrectRelMention(UTF8OutputStream& stream,
													RelMention* ment,
													Relation* rel)
{
	stream << L"        <relation_mention ID=\"";
	stream << rel->getID() << L"-" << ment->getUID().toInt() << L"\">\n";

	stream << L"          <rel_mention_arg MENTIONID=\"";
	stream << rel->getLeftEntityID() << L"-" << ment->getLeftMention()->getUID();
	stream << L"\" ARGNUM=\"1\">\n";
	if (_correctDocument->getCorrectMentionFromMentionID(ment->getLeftMention()->getUID()) != NULL) {
		_printCorrectMentionText(stream, 
				_correctDocument->getCorrectMentionFromMentionID(ment->getLeftMention()->getUID()),
				ment->getLeftMention()->getSentenceNumber());
	}
	else {
		_printAPFNodeText(stream, ment->getLeftMention()->getNode(), 
			ment->getLeftMention()->getSentenceNumber());
	}
	stream << L"          </rel_mention_arg>\n";
	
	stream << L"          <rel_mention_arg MENTIONID=\"";
	stream << rel->getRightEntityID() << L"-" << ment->getRightMention()->getUID();
	stream << L"\" ARGNUM=\"2\">\n";
	if (_correctDocument->getCorrectMentionFromMentionID(ment->getRightMention()->getUID()) != NULL) {
		_printCorrectMentionText(stream,
				_correctDocument->getCorrectMentionFromMentionID(ment->getRightMention()->getUID()),
				ment->getRightMention()->getSentenceNumber());
	}
	else {
		_printAPFNodeText(stream, ment->getRightMention()->getNode(), 
			ment->getRightMention()->getSentenceNumber());
	}
	stream << L"          </rel_mention_arg>\n";
		
	// TODO: add time

	stream << L"        </relation_mention>\n";
}


void CAResultCollectorAPF::_printAPFEntityFooter(UTF8OutputStream& stream)
{
	stream << L"    </entity>\n\n";
}

void CAResultCollectorAPF::_printAPFRelationFooter(UTF8OutputStream& stream)
{
	stream << L"      </relation_mentions>\n";
	stream << L"    </relation>\n\n";
}

void CAResultCollectorAPF::_printAPFDocumentFooter(UTF8OutputStream& stream)
{
	stream << L"  </document>\n";
	stream << L"</source_file>\n";
}

bool CAResultCollectorAPF::_isTopMention(Mention* ment, EntityType type)
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
		throw InternalInconsistencyException("CAResultCollectorAPF::_isTopMention()",
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
bool CAResultCollectorAPF::_isNestedInsideOfName(Mention* ment)
{
	// TODO: is this mention hierarchy what I really want?
	if (ment->mentionType == Mention::NAME)
		return false;
	return _isNestedDriver(ment->node, ment->getSentenceNumber());

}

bool CAResultCollectorAPF::_isNestedDriver(const SynNode* node, int sentNum)
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

bool CAResultCollectorAPF::_isSecondPartOfAppositive(Mention* ment)
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
bool CAResultCollectorAPF::_isGPEModifierOfPersonGPE(Mention* ment)
{
	return false;
}

// mention must be a name, must have a parent whose edt head is different
// and must have a parent whose entity is different
bool CAResultCollectorAPF::_isNameNotInHeadOfParent(Mention* ment, EntityType type)
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
				throw InternalInconsistencyException("CAResultCollectorAPF::_isNameNotInHeadOfParent()",
													 "current mention has no entity!");
			if (parEnt->getID() == ent->getID())
				return false;
		}
	}
	return true;
}
