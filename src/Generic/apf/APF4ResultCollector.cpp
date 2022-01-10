// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include <set>
#include "Generic/common/ParamReader.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/InternalInconsistencyException.h"
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
#include "Generic/theories/ValueType.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/RelationSet.h"
#include "Generic/theories/Relation.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/apf/APF4ResultCollector.h"
#include "Generic/clutter/ACE2008EvalClutterFilter.h"
#include "Generic/clutter/EntityTypeClutterFilter.h"
#include "Generic/common/version.h"


using namespace std;


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


APF4ResultCollector::APF4ResultCollector(int mode_): APF4GenericResultCollector(mode_) {
	_print_pronoun_level_entities = ParamReader::getOptionalTrueFalseParamWithDefaultVal("apf_print_pronoun_level_entities", false);
	_print_1p_pronoun_level_entities= ParamReader::getOptionalTrueFalseParamWithDefaultVal("apf_print_1p_pronoun_level_entities", false);
	_print_partitives_in_relations = ParamReader::getOptionalTrueFalseParamWithDefaultVal("apf_print_partitives_in_relations", true);
	_print_partitives_in_events = ParamReader::getOptionalTrueFalseParamWithDefaultVal("apf_print_partitives_in_events", true);
//#ifdef ENGLISH_LANGUAGE
	if (SerifVersion::isEnglish()) {
		_print_all_partitives = ParamReader::getOptionalTrueFalseParamWithDefaultVal("apf_print_all_partitives", false);
	} else {
//#else
		_print_all_partitives = ParamReader::getOptionalTrueFalseParamWithDefaultVal("apf_print_all_partitives", true);
	}
//#endif
}

void APF4ResultCollector::_printAPFMention(OutputStream& stream,
										   Mention* ment,
										   Entity* ent, 
										   const wchar_t* doc_name,
										   std::set<MentionUID>& printed_mentions_for_this_entity)
{
	if (ment->mentionType == Mention::LIST) {
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
	const SynNode* head = ment->getEDTHead();
	stream << L"          <extent>\n";
	APF4GenericResultCollector::_printAPFMentionExtent(stream, node, sentNum);
	stream << L"          </extent>\n";
	stream << L"          <head>\n";
	APF4GenericResultCollector::_printAPFMentionExtent(stream, head, sentNum);
	stream << L"          </head>\n";
	stream << L"        </entity_mention>\n";

	_printedMentions.insert(ment->getUID());
}

// WARNING: read the memory warning below before making changes to this method!
void APF4ResultCollector::_printAPFEntitySet(OutputStream& stream,  std::set<int>& printed_entities) {
	const wchar_t* doc_name = _docTheory->getDocument()->getName().to_string();

	if (_entitySet != NULL) {
		int ents = _entitySet->getNEntities();

		for (int i = 0; i < ents; i++) {
			Entity* ent = _entitySet->getEntity(i);
			if (!ent->getType().isRecognized())
				continue;
			GrowableArray<MentionUID> &mens = ent->mentions;

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
					string err = "Mention in edt entity	not	of edt type!\nWhile	creating: ";
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
					SessionLogger::warn("apf4") << "APF4ResultCollector::_printAPFEntitySet(): " << err.c_str();
					//throw InternalInconsistencyException("APF4ResultCollector::produceAPFOutput()",
					//	(char*)err.c_str());
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
					// TB  3/13/08  - metonymic mentions also should be excluded
					if (!wcscmp(_convertMentionTypeToAPF(ment), L"NAM") //&&
						//!ResultCollectionUtilities::isPREmention(_entitySet, ment)
						&& !ment->isMetonymyMention())
						validNames[valid_names_size++] = ment;
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

			if (!_print_1p_pronoun_level_entities) {
				// if _print_1p_pronoun_level_entities is false weed out entities with only 
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

			if (!_print_pronoun_level_entities) {

				// If _print_pronoun_level_entities is false, we should have at least 
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

			if (!_print_all_partitives) {
				bool seen_non_partitive =	false;
				for	(int j = 0;	j <	valid_mentions_size; j++) {
					if (validMentions[j]->mentionType != Mention::PART)	{
						seen_non_partitive = true;
						break;
					}
				}
				if (!seen_non_partitive) {
					bool still_print_it = false;
					if (_print_partitives_in_relations && _relationSet != NULL) {
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
					if (!still_print_it && _print_partitives_in_events && _eventSet != NULL) {
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
			std::set<MentionUID> printed_mentions_for_this_entity;
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

void APF4ResultCollector::_printEntitySubtype(OutputStream& stream, Entity* ent) {

	if (MODE == APF2004 && ent->type.matchesPER()) return;

	EntitySubtype subtype = _entitySet->guessEntitySubtype(ent);
	if (subtype.getParentEntityType() != ent->type ||
		subtype == EntitySubtype::getUndetType())
		subtype = EntitySubtype::getDefaultSubtype(ent->getType());
	stream << L" SUBTYPE=\"";
	stream << subtype.getName().to_string();
	stream << L"\"";
	// I think we should not allow for this in APF2004 or APF2005 -- type-specific
	//   data like this does not belong in Serif, it should be in a parameter file.
	/*else {
		if (ent->type.matchesLOC())
			stream << L" SUBTYPE=\"Address\"";
		else if (!ent->type.matchesPER())
			stream << L" SUBTYPE=\"Other\"";
	} */

}

void APF4ResultCollector::_printEntityClass(OutputStream& stream, Entity* ent) {
	stream << L" CLASS=\"";
	// in 2005, we never want to call anything generic, even if we store system-internal
	//  information about whether we think it's generic or not
	if (ent->isGeneric() && MODE == APF2004) 
		stream << L"GEN";
	else
		stream << L"SPC";

	stream << L"\"";
}

void APF4ResultCollector::_printEntityMentionRole(OutputStream& stream, Mention *ment) {
	if (ment->getEntityType() == EntityType::getGPEType() && ment->hasRoleType())
		stream << L" ROLE=\"" << ment->getRoleType().getName().to_string() << "\"";
}

bool APF4ResultCollector::isValidRelationParticipant(int entity_id) {
	if ((_entitySet->getEntity(entity_id))->isGeneric())
		return false;
	return true;
}
