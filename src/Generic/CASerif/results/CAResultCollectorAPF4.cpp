// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include <set>
#include "Generic/common/ParamReader.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/WordConstants.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/EventSet.h"
#include "Generic/theories/Value.h"
#include "Generic/theories/ValueSet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/RelationSet.h"
#include "Generic/theories/Relation.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/values/TemporalNormalizer.h"

#include "Generic/clutter/ACE2008EvalClutterFilter.h"
#include "Generic/clutter/EntityTypeClutterFilter.h"

#include "Generic/CASerif/correctanswers/CorrectAnswers.h"
#include "Generic/CASerif/correctanswers/CorrectMention.h"
#include "Generic/CASerif/correctanswers/CorrectEntity.h"
#include "Generic/CASerif/results/CAResultCollectorAPF4.h"

#include "Generic/common/version.h"


using namespace std;

#define PRINT_PRONOUN_LEVEL_ENTITIES false
#define PRINT_1P_PRONOUN_LEVEL_ENTITIES false
#define PRINT_PARTITIVES_IN_RELATIONS true
#define PRINT_PARTITIVES_IN_EVENTS true
//#ifdef ENGLISH_LANGUAGE
//	#define PRINT_ALL_PARTITIVES false
//#else
//	#define PRINT_ALL_PARTITIVES true
//#endif


static const string WEA_ClutterFilterName = EntityTypeClutterFilter::getFilterName(L"WEA");//"remove-type-WEA";
static const string VEH_ClutterFilterName = EntityTypeClutterFilter::getFilterName(L"VEH");//"remove-type-VEH";

//
// This class will also be used to create APF2005-style output.
//
// 2004/2005 differences are as follows:
//		* 2004: no subtypes for PER
//        2005: all entities have subtypes
//	    * different labels for various items in the relation structure
//		  (e.g. rel_entity_arg vs. relation_argument)
//	    * 2005 has no PRE mentions
//		* 2005 adds values (including TIMEX) and events
//		* 2005 remove mentions (and adjust relations) for bnews,cts  not in speaker tags

CAResultCollectorAPF4::CAResultCollectorAPF4(int mode_)
	: APF4GenericResultCollector(mode_), USE_CORRECT_COREF(true), USE_CORRECT_SUBTYPES(true)
{
	if (ParamReader::getParam(Symbol(L"use_correct_coref")) == Symbol(L"false"))
		USE_CORRECT_COREF = false;
	if (ParamReader::getParam(Symbol(L"use_correct_subtypes")) == Symbol(L"false"))
		USE_CORRECT_SUBTYPES = false;
}

void CAResultCollectorAPF4::loadDocTheory(DocTheory *docTheory) { 
	APF4GenericResultCollector::loadDocTheory(docTheory);
	_correctDocument = CorrectAnswers::getInstance().getCorrectDocument(docTheory->getDocument()->getName());
}

const wchar_t* CAResultCollectorAPF4::_convertMentionTypeToAPF(Mention* ment)
{
	CorrectMention *cm = _correctDocument->getCorrectMentionFromMentionID(ment->getUID());
	if (cm != NULL) {
		Symbol cmt = cm->getCorrectMentionType();
		if (cmt == CASymbolicConstants::NAME_UPPER)
			return L"NAM";
		if (cmt == CASymbolicConstants::NOMINAL_UPPER)
			return L"NOM";
		if (cmt == CASymbolicConstants::PRONOUN_UPPER)
			return L"PRO";
		if (cmt == CASymbolicConstants::PRE_UPPER) {
			if (MODE == APF2004) 
				return L"PRE";
			else //if (MODE == APF2005 || MODE == APF2007 || MODE == APF2008)
				return L"NAM";
		}
        if (cmt == CASymbolicConstants::NOM_PRE_UPPER) {
			if (MODE == APF2004) 
				return L"PRE";
			else //if (MODE == APF2005 || MODE == APF2007 || MODE == APF2008)
				return L"NOM";
		}
	}
	// else

	if (ment->mentionType == Mention::LIST) {
			// for list, the type is the type of the first child
			Mention *subMent = ment->getChild();
			return _convertMentionTypeToAPF(subMent);
	}

	return APF4GenericResultCollector::_convertMentionTypeToAPF(ment);
}

void CAResultCollectorAPF4::_printAPFMention(OutputStream& stream,
											 Mention* ment,
											 Entity* ent, 
											 const wchar_t* doc_name,
											 std::set<Mention::UID>& printed_mentions_for_this_entity)
{
	CorrectMention *cm = _correctDocument->getCorrectMentionFromMentionID(ment->getUID());

	if (ment->mentionType == Mention::LIST && cm == NULL) {
		// NOTE: If the LIST has a correct mention associated with it, it must have 
		//       been a mistake to tag it as a list, so just treat it as a normal mention.
		_printAPFListMention(stream, ment, ent, doc_name, printed_mentions_for_this_entity);
		return;
	}

	// I think this will probably only happen when we unwisely pass in a child of a list
	// 
	// Occasionally, due to an unknown bug in Serif, a mention will be part of 
	// multiple entities. Leaving this if statement in will sometimes result in an
	// entity with no mentions, which breaks the ACE Scorer -AZ 1/28/2007
//	if (_isPrintedMention(ment))
//		return;
	// RPB: Here is my fix for the above issues.  Duplicate mentions were breaking things downstream.
	if (printed_mentions_for_this_entity.find(ment->getUID()) != printed_mentions_for_this_entity.end()) {
		return;
	}
	printed_mentions_for_this_entity.insert(ment->getUID());

	_printAPFMentionHeader(stream, ment, ent, doc_name);

	// print extent info for the whole extent and the head extent.
	const SynNode* node = ment->node;
	int sentNum = ment->getSentenceNumber();

	if (cm != NULL && !cm->isPrinted()) {
		
		cm->setIsPrintedFlag();
		stream << L"          <extent>\n";
		_printCorrectMentionExtent(stream, cm, sentNum);
		stream << L"          </extent>\n";
		stream << L"          <head>\n";
		_printCorrectMentionHeadExtent(stream, cm, sentNum);
		stream << L"          </head>\n";
		stream << L"        </entity_mention>\n";
	} else {
		const SynNode* head = ment->getEDTHead();
		stream << L"          <extent>\n";
		_printAPFMentionExtent(stream, node, sentNum);
		stream << L"          </extent>\n";
		stream << L"          <head>\n";
		_printAPFMentionExtent(stream, head, sentNum);
		stream << L"          </head>\n";
		stream << L"        </entity_mention>\n";
	}
	
	_printedMentions.insert(ment->getUID());
}

void CAResultCollectorAPF4::_printCorrectMentionExtent(OutputStream& stream,
												   CorrectMention *cm,
												   int sentNum)
{
	int startTok = cm->getStartToken();
	int endTok = cm->getEndToken();
	if (startTok == -1)
		startTok = 0;
	if (endTok == -1)
		endTok = 0;
	EDTOffset start = _tokenSequence[sentNum]->getToken(startTok)->getStartEDTOffset();
	EDTOffset end = _tokenSequence[sentNum]->getToken(endTok)->getEndEDTOffset(); 
	stream << L"              <charseq START=\"" << start << "\" END=\"" << end << "\">";
	_printCorrectMentionText(stream, cm, sentNum);
	stream << L"</charseq>\n";
}

void CAResultCollectorAPF4::_printCorrectMentionHeadExtent(OutputStream& stream,
												   CorrectMention *cm,
												   int sentNum)
{
	int startTok = cm->getHeadStartToken();
	int endTok = cm->getHeadEndToken();
	if (startTok == -1)
		startTok = 0;
	if (endTok == -1)
		endTok = 0;	
	EDTOffset start = _tokenSequence[sentNum]->getToken(startTok)->getStartEDTOffset();
	EDTOffset end = _tokenSequence[sentNum]->getToken(endTok)->getEndEDTOffset();
	
	bool makechange = false;
	if(start != cm->getHeadStartOffset()){
		std::cout<<"adjusting start: "<<start<<" -> "<<cm->getHeadStartOffset()<<std::endl;
		makechange = true;
		start = cm->getHeadStartOffset();
	}
	if(end != cm->getHeadEndOffset()){
		std::cout<<"adjusting end: "<<end<<" -> "<<cm->getHeadEndOffset()<<std::endl;
		makechange = true;
		end = cm->getHeadEndOffset();
	}
	

	stream << L"              <charseq START=\"" << start << "\" END=\"" << end << "\">";
	_printCorrectMentionHeadText(stream, cm, sentNum);
	stream << L"</charseq>\n";
}

void CAResultCollectorAPF4::_printCorrectMentionText(OutputStream& stream, 
		CorrectMention *cm,
		int sentNum) 
{
	int startTok = cm->getStartToken();
	int endTok = cm->getEndToken();
	if (startTok == -1)
		startTok = 0;
	if (endTok == -1)
		endTok = 0;	
	
	_printAPFTokenSpanText(stream, startTok, endTok, sentNum);
}

void CAResultCollectorAPF4::_printCorrectMentionHeadText(OutputStream& stream, 
		CorrectMention *cm,
		int sentNum) 
{
	const LocatedString* sentString =_docTheory->getSentence(sentNum)->getString();

	EDTOffset start_edt = cm->getHeadStartOffset();
	EDTOffset end_edt = cm->getHeadEndOffset();

	int start = sentString->positionOfStartOffset(start_edt);
	int end = sentString->positionOfEndOffset(end_edt);

	if ((start != -1) && (end != -1)) {
		LocatedString* head_str = sentString->substring(start, end+1);
		const wchar_t* str =	head_str->toString();
		_printSafeString(stream, str);
		delete head_str;
	}
	else {
		int startTok = cm->getHeadStartToken();
		int endTok = cm->getHeadEndToken();
		if (startTok == -1)
			startTok = 0;
		if (endTok == -1)
			endTok = 0;	

		_printAPFTokenSpanText(stream, startTok, endTok, sentNum);
	}
}

// WARNING: read the memory warning below before making changes to this method!
void CAResultCollectorAPF4::_printAPFEntitySet(OutputStream& stream, std::set<int>& printed_entities) {
	const wchar_t* doc_name = _docTheory->getDocument()->getName().to_string();

	if (_entitySet != NULL) {
		int ents = _entitySet->getNEntities();

		for (int i = 0; i < ents; i++) {
			Entity* ent = _entitySet->getEntity(i);
			if (!ent->getType().isRecognized())
				continue;
			GrowableArray<Mention::UID> &mens = ent->mentions;

			// filter out Entity types VEH and WEA (for the ACE2008 eval)
			// this also results in eliminating the event s and relations involved  with those entities
			if((ent->isFiltered(WEA_ClutterFilterName) || ent->isFiltered(VEH_ClutterFilterName)
					|| ent->isFiltered(ACE2008EvalClutterFilter::filterName)) 
					&& !_in_post_xdoc_print_mode) {
				continue;
			}

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

			for	(int j = 0;	j <	mens.length(); j++)	{
				Mention* ment =	_entitySet->getMention(mens[j]);
				// really odd if mentions aren't of	the	type of	their entity
				// JSM 6/10/08: Not necessarily.  We may want to allow this in some cases, 
				// so let's just print a warning.
				if (!ment->getEntityType().isRecognized()) {
					string err = "mention in edt entity	not	of edt type!\nWhile	creating: ";
					err.append(UnicodeUtil::toUTF8StdString(stream.getFileName()));
					err.append("\nAt node:\n");
					err.append(ment->node->toDebugString(0));
					err.append("\n");
					err.append("Entity was ");
					err.append(" (");
					err.append(ent->getType().getName().to_debug_string());
					err.append("), mention was ");
					err.append(" (");
					err.append(ment->getEntityType().getName().to_debug_string());
					err.append(" - ");
					err.append(Mention::getTypeString(ment->mentionType));
					err.append(")\n");
					SessionLogger::logger->beginWarning();
					*SessionLogger::logger << "CAResultCollectorAPF4::_printAPFEntitySet(): " << err.c_str();
					//throw InternalInconsistencyException("CAResultCollectorAPF4::_printAPFEntitySet()",
					//	(char*)err.c_str());
					continue;
				}

				// we only pretty much want	solo, unnested mentions
				// an exception	is made, though, for appositive	members
				// and GPE modifiers of	GPE	entities with person role (? &&	TODO)
				if(_isPrintableMention(ment, ent)) {
					validMentions[valid_mentions_size++] = ment;
					// names in	the	normal situation are the only mentions added
					// to the name attributes section
					// JCS 3/12/04 - All names need to have a name attribute
					// EMB 6/29/04 - except, of course, PRE mentions
					// AHZ 11/4/05 - nah, lets do it for PRE mentions too
					// TB  3/13/08  - Metonymic mentions also should be excluded
					//    We should probably check for it in CorrectMention but this info is not available
					if(!ment->isMetonymyMention()) {
						CorrectMention *cm = _correctDocument->getCorrectMentionFromMentionID(ment->getUID());
						if (cm && cm->getCorrectMentionType() == CASymbolicConstants::NAME_UPPER) 
							validNames[valid_names_size++] = ment;
						if (!cm && ment->mentionType == Mention::NAME) //&&
							//!ResultCollectionUtilities::isPREmention(_entitySet, ment))
							validNames[valid_names_size++] = ment;
					}
				}
			}

			// is there	something to print?
			if (valid_mentions_size	< 1)
			{
				// MEMORY: handle the arrays created earlier
				delete [] validMentions;
				delete [] validNames;
				continue;
			}

			if (!PRINT_1P_PRONOUN_LEVEL_ENTITIES) {
				// if PRINT_1P_PRONOUN_LEVEL_ENTITIES is false weed out entities with only 
				// 1st person pronouns in them
				bool seen_non_1p_pronoun =	false;
				for	(int j = 0;	j <	valid_mentions_size; j++) {
					if (validMentions[j]->mentionType != Mention::PRON ||
						!WordConstants::is1pPronoun(validMentions[j]->getHead()->getHeadWord()))
					{
						seen_non_1p_pronoun = true;
						break;
					}
				}
				if (!seen_non_1p_pronoun) {
					// MEMORY: handle the arrays created earlier
					delete [] validMentions;
					delete [] validNames;
					continue;
				}

			}

			if (!PRINT_PRONOUN_LEVEL_ENTITIES) {

				// If PRINT_PRONOUN_LEVEL_ENTITIES is false, we should have at least 
				// one non-pronoun in an entity.
				// (This could be determined as the array is collected, but it's cleaner to check now
				// and doesn't take	considerably longer.)
				bool seen_non_pronoun =	false;
				for	(int j = 0;	j <	valid_mentions_size; j++) {
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
			}

//			if (!PRINT_ALL_PARTITIVES) {
			if (SerifVersion::isEnglish()) {
				bool seen_non_partitive =	false;
				for	(int j = 0;	j <	valid_mentions_size; j++) {
					if (validMentions[j]->mentionType != Mention::PART)	{
						seen_non_partitive = true;
						break;
					}
				}
				if (!seen_non_partitive) {
					bool still_print_it = false;
					if (PRINT_PARTITIVES_IN_RELATIONS && _relationSet != NULL) {
						int n_rels = _relationSet->getNRelations();
						for (int i = 0; i < n_rels; i++) {
							Relation *rel = _relationSet->getRelation(i);
							if (rel->getLeftEntityID() == ent->getID() ||
								rel->getRightEntityID() == ent->getID()) 
							{
								still_print_it = true;
								break;
							}
						}
					}
					// this is SOOOO inefficient
					if (!still_print_it && PRINT_PARTITIVES_IN_EVENTS && _eventSet != NULL) {
						int n_events = _eventSet->getNEvents();
						for (int i = 0; i < n_events; i++) {
							Event *event = _eventSet->getEvent(i);
							Event::LinkedEventMention *mentions = event->getEventMentions();
							while (mentions != 0) {
								int n_args = mentions->eventMention->getNArgs();
								for (int j = 0; j < n_args; j++) {
									const Mention *ment = mentions->eventMention->getNthArgMention(j);
									if (ent == _entitySet->getEntityByMention(ment->getUID(), ent->getType()))
									{
										still_print_it = true;
										break;
									}
								}
								mentions = mentions->next;
							}
						}
					}
					if (!still_print_it) {
						// MEMORY: handle the arrays created earlier
						delete [] validMentions;
						delete [] validNames;
						continue;
					}
				}
			}
			
			_printAPFEntityHeader(stream, ent, doc_name);
			printed_entities.insert(ent->getID());

			// now print the mentions
			std::set<Mention::UID> printed_mentions_for_this_entity;
			for	(int j = 0;	j <	valid_mentions_size; j++) {
				_printAPFMention(stream, validMentions[j], ent, doc_name, printed_mentions_for_this_entity);
			}
			// now print the names
			if (valid_names_size > 0) {
				_printAPFAttributeHeader(stream);
				for	(int j = 0;	j <	valid_names_size; j++) {
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
}

void CAResultCollectorAPF4::_printEntitySubtype(OutputStream& stream, Entity* ent) {
	Symbol symbolicSubtype = CASymbolicConstants::NONE_SYM;

	if (USE_CORRECT_COREF) {

		CorrectEntity *ce = _correctDocument->getCorrectEntityFromEntityID(ent->getID());

		symbolicSubtype = CASymbolicConstants::NONE_SYM;
		if (ce) 
			symbolicSubtype = ce->getSymbolicSubtype().to_string();
		
	} else {
		if (USE_CORRECT_SUBTYPES) {

			// go through mentions and find class and subtype
			for (int i = 0; i < ent->getNMentions(); i++) {
				Mention *ment = _entitySet->getMention(ent->getMention(i));
				CorrectMention *cm = _correctDocument->getCorrectMentionFromMentionID(ment->getUID());
				if (!cm) continue;

				CorrectEntity *ce = _correctDocument->getCorrectEntityFromCorrectMention(cm);
				if (!ce) continue;

				// find first non-NONE subtype
				if (symbolicSubtype == CASymbolicConstants::NONE_SYM &&
					ce->getSymbolicSubtype() != CASymbolicConstants::NONE_SYM)
				{
					// make sure entity types match?
					if (ce->getEntityType()->getName() == ent->getType().getName())
						symbolicSubtype = ce->getSymbolicSubtype();
				}
			}

			// if we didn't find a non-NONE subtype and we need one, get the default
			if (symbolicSubtype == CASymbolicConstants::NONE_SYM &&
				(MODE != APF2004 || !ent->getType().matchesPER()))
			{
				symbolicSubtype = (EntitySubtype::getDefaultSubtype(ent->getType())).getName();
			}	

		} else {
			if (MODE == APF2004 && ent->getType().matchesPER())
				symbolicSubtype = CASymbolicConstants::NONE_SYM;
			else {
				EntitySubtype subtype = _entitySet->guessEntitySubtype(ent);
				if (subtype.getParentEntityType() != ent->type ||
					subtype == EntitySubtype::getUndetType())
					subtype = EntitySubtype::getDefaultSubtype(ent->getType());	
				symbolicSubtype = subtype.getName();
			}
		}
	}

	if (symbolicSubtype != CASymbolicConstants::NONE_SYM) 
		stream << L" SUBTYPE=\"" << symbolicSubtype.to_string() << L"\"";
}

void CAResultCollectorAPF4::_printEntityClass(OutputStream& stream, Entity *ent) {

	Symbol symbolicClass = CASymbolicConstants::NONE_SYM;
		
	if (USE_CORRECT_COREF) {

		CorrectEntity *ce = _correctDocument->getCorrectEntityFromEntityID(ent->getID());

		symbolicClass = CASymbolicConstants::SPC_SYM;
		if (ce && ce->getSymbolicClass() != CASymbolicConstants::NONE_SYM) 
			symbolicClass = ce->getSymbolicClass();

	} else {

		if (USE_CORRECT_SUBTYPES) {

			// go through mentions and find class and subtype
			for (int i = 0; i < ent->getNMentions(); i++) {
				Mention *ment = _entitySet->getMention(ent->getMention(i));
				CorrectMention *cm = _correctDocument->getCorrectMentionFromMentionID(ment->getUID());
				if (!cm) continue;

				CorrectEntity *ce = _correctDocument->getCorrectEntityFromCorrectMention(cm);
				if (!ce) continue;

				// find first non-NONE class
				if (symbolicClass == CASymbolicConstants::NONE_SYM &&
					ce->getSymbolicClass() != CASymbolicConstants::NONE_SYM) 
				{	
					symbolicClass = ce->getSymbolicClass();
				}
			}	

		} 

		// if we didn't find a non-NONE class, just use SPC
		if (symbolicClass == CASymbolicConstants::NONE_SYM)
			symbolicClass = CASymbolicConstants::SPC_SYM;
		
	}

	stream << L" CLASS=\"" << symbolicClass.to_string() << L"\"";
}

void CAResultCollectorAPF4::_printEntityMentionRole(OutputStream& stream, Mention *ment) {
	CorrectMention *cm = _correctDocument->getCorrectMentionFromMentionID(ment->getUID());
	if (cm != NULL && cm->getRole() != CASymbolicConstants::NONE_SYM)
		stream << L" ROLE=\"" << cm->getRole().to_string() << "\"";
}

bool CAResultCollectorAPF4::isValidRelationParticipant(int entity_id) {
	CorrectEntity *entity = _correctDocument->getCorrectEntityFromEntityID(entity_id);

	// It turns out that LDC does mark relations between generic entities (EMB 7/12/04)
	// but probably not between negative ones
	if (entity && entity->getSymbolicClass() == Symbol(L"NEG"))
		return false;

	return true;
}
