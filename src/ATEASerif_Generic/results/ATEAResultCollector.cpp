// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// Copyright (c) 2006 by BBN Technologies, Inc.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "ATEASerif_generic/results/ATEAResultCollector.h"
#include "theories/SentenceTheory.h"
#include "theories/Parse.h"
#include "theories/DocTheory.h"
#include "common/GrowableArray.h"
#include "common/InternalInconsistencyException.h"
#include "common/UTF8OutputStream.h"
#include "common/OStringStream.h"
#include "common/ParamReader.h"
#include "common/SymbolHash.h"
#include "common/UnicodeUtil.h"
#include "theories/Mention.h"
#include "theories/Entity.h"
#include "theories/Event.h"
#include "theories/EventMention.h"
#include "theories/EventSet.h"
#include "theories/EventEntityRelation.h"
#include "theories/SynNode.h"
#include "theories/Token.h"
#include "theories/EntitySet.h"
#include "theories/Value.h"
#include "theories/ValueSet.h"
#include "theories/ValueType.h"
#include "theories/RelationSet.h"
#include "theories/RelationConstants.h"
#include "theories/Relation.h"
#include "theories/RelMention.h"
#include "theories/TokenSequence.h"
#include "theories/NodeInfo.h"
#include "common/SessionLogger.h"
#include "results/ResultCollectionUtilities.h"
#include "relations/RelationUtilities.h"
#include "common/WordConstants.h"
#include "ATEASerif_generic/titles/TitleMap.h"
#include "ATEASerif_generic/titles/TitleListNode.h"
#include "Generic/common/version.h"

#include "clutter/ClutterFilter.h"
#include <boost/scoped_ptr.hpp>

using namespace std;

#define PRINT_PRONOUN_LEVEL_ENTITIES true
#define PRINT_PARTITIVES_IN_RELATIONS true
#define PRINT_PARTITIVES_IN_EVENTS true
// #ifdef ENGLISH_LANGUAGE
	// #define PRINT_ALL_PARTITIVES false
// #else
	// #define PRINT_ALL_PARTITIVES true
// #endif

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

const Symbol ATEAResultCollector::SUB = Symbol(L"SUB");
const Symbol ATEAResultCollector::OBJ = Symbol(L"OBJ");
const Symbol ATEAResultCollector::IOBJ = Symbol(L"IOBJ");

ATEAResultCollector::ATEAResultCollector(int mode_) 
	: _docTheory(0), _entitySet(0), _valueSet(0), _relationSet(0), _tokenSequence(0), 
	_outputStream(0), _includeXMLHeaderInfo(true), _is_downcased_doc(false),
	_useAbbreviatedIDs(false), MODE(mode_), _clutterFilter(0)
{
	Symbol input = ParamReader::getParam(L"input_type");
	if (input == Symbol())
		_inputType = Symbol(L"text");
	else
		_inputType = input;

	_print_sentence_boundaries = false;
	_print_original_text = false;
	char buffer[501];
	if (ParamReader::getParam("output_sentence_boundaries", buffer, 500) &&
		!strcmp(buffer, "true"))
		_print_sentence_boundaries = true;
	if (ParamReader::getParam("output_original_text", buffer, 500) &&
		!strcmp(buffer, "true"))
		_print_original_text = true;
	if (ParamReader::getParam("output_pronoun_words", buffer, 500))
		_outputPronounWords = _new SymbolHash(buffer);
	else 
		_outputPronounWords = 0;
	if (ParamReader::getParam("output_name_words", buffer, 500))
		_outputNameWords = _new SymbolHash(buffer);
	else 
		_outputNameWords = 0;
	if (ParamReader::getParam("output_nominal_words", buffer, 500))
		_outputNominalWords = _new SymbolHash(buffer);
	else 
		_outputNominalWords = 0;

	if (ParamReader::isParamTrue("filter_out_overlapping_apf_mentions"))
		_filterOverlappingMentions = true;
	else
		_filterOverlappingMentions = false;

	char titles_file[501];
	if (ParamReader::getParam("titles_file", titles_file, 500))
		_create_title_mentions = true;
	else
		_create_title_mentions = false;

	if (_create_title_mentions) {
		boost::scoped_ptr<UTF8InputStream> inStream_scoped_ptr(UTF8InputStream::build(titles_file));
		UTF8InputStream& inStream(*inStream_scoped_ptr);

		_titles = new TitleMap(inStream);
	}

	_print_dump_info = ParamReader::isParamTrue("print_dump_info");
}

ATEAResultCollector::~ATEAResultCollector()
{	
	finalize(); 
	if (_outputPronounWords) {
		delete _outputPronounWords;
	}
	if (_outputNameWords) {
		delete _outputNameWords;
	}
	if (_outputNominalWords) {
		delete _outputNominalWords;
	}
}

void ATEAResultCollector::loadDocTheory(DocTheory* theory) {
	finalize(); // get rid of old stuff

	_docTheory = theory;
	_entitySet = theory->getEntitySet();
	_valueSet = theory->getValueSet();
	_relationSet = theory->getRelationSet();
	_eventSet = theory->getEventSet();
	_is_downcased_doc = theory->getDocument()->isDowncased();
	
	setClutterFilter(new ATEAInternalClutterFilter(theory));

	//if (_is_downcased_doc) {
	//	cout << "is downcased\n";
	//} else {
	//	cout << "is not downcased\n";
	//}
	int numSents = theory->getNSentences();
	_tokenSequence = _new const TokenSequence*[numSents];
	for (int i = 0; i < numSents; i++)
		_tokenSequence[i] = theory->getSentenceTheory(i)->getTokenSequence();


}

void ATEAResultCollector::produceOutput(std::wstring *results) {
	OStringStream stream(*results);
	produceOutput(stream);
}

void ATEAResultCollector::produceOutput(const char *output_dir,
										const char *doc_filename)
{
	char output_file[210];
	strncpy_s(output_file, output_dir, 100);
	strcat_s(output_file, "/");
	strncat_s(output_file, doc_filename, 100);
	strcat_s(output_file, ".apf");

	UTF8OutputStream stream;
	stream.open(output_file);
	produceOutput(stream);
}

void ATEAResultCollector::produceOutput(const wchar_t *output_dir,
										const wchar_t *doc_filename)
{
	wchar_t output_file[210];
	wcsncpy_s(output_file, output_dir, 100);
	wcscat_s(output_file, L"/");
	wcsncat_s(output_file, doc_filename, 100);
	wcscat_s(output_file, L".apf");

	UTF8OutputStream stream;
	stream.open(output_file);
	produceOutput(stream);
}

// WARNING: read the memory warning below before making changes to this method!
void ATEAResultCollector::produceOutput(OutputStream &stream) {

	_printed_mentions_count = 0;
	bool *isEntityPrinted = NULL;
	if (_entitySet != NULL) {
		isEntityPrinted = _new bool[_entitySet->getNEntities()];
		for (int i = 0; i < _entitySet->getNEntities(); i++)
			isEntityPrinted[i] = false;
	}

	// unknown relations
	_warning_printed = false;
	_unknown_relation_count = 0;

	const wchar_t* docName = _docTheory->getDocument()->getName().to_string();

	// NOTE: source here is faked. if we ever care, we need to fix this.
//	const wchar_t* source = L"newspaper";
//	_printAPFDocumentHeader(stream, docName, source);
	_printAPFDocumentHeader(stream, docName, _docTheory->getDocument()->getSourceType().to_string());
	if (_entitySet != NULL) {
		int ents = _entitySet->getNEntities();

		for (int i = 0; i < ents; i++) {
			Entity* ent = _entitySet->getEntity(i);
			if (!ent->getType().isRecognized())
				continue;
			GrowableArray<MentionUID> mens = ent->mentions;

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
				if (!ment->entityType.isRecognized()) {
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
					err.append(ment->entityType.getName().to_debug_string());
					err.append(" - ");
					err.append(Mention::getTypeString(ment->mentionType));
					err.append(")\n");
					throw InternalInconsistencyException("ATEAResultCollector::produceAPFOutput()",
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
					// EMB 6/29/04 - except, of course, PRE mentions
					// AHZ 11/4/05 - nah, lets do it for PRE mentions too
					if (!wcscmp(_convertMentionTypeToAPF(ment), L"NAM")) //&&
						//!ResultCollectionUtilities::isPREmention(_entitySet, ment))
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

			if (!PRINT_PRONOUN_LEVEL_ENTITIES) {

				// If PRINT_PRONOUN_LEVEL_ENTITIES is false, we should have at least 
				// one non-pronoun in an entity.
				// (This could be determined as the array is collected, but it's cleaner to check now
				// and doesn't take	considerably longer.)
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
			}

//			if (!PRINT_ALL_PARTITIVES) {
			if (SerifVersion::isEnglish()) {
				bool seen_non_partitive =	false;
				for	(j = 0;	j <	valid_mentions_size; j++) {
					if (validMentions[j]->mentionType != Mention::PART)	{
						seen_non_partitive = true;
						break;
					}
				}
				if (!seen_non_partitive) {
					bool still_print_it = false;
					if (PRINT_PARTITIVES_IN_RELATIONS) {
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
					if (!still_print_it && PRINT_PARTITIVES_IN_EVENTS) {
						int n_events = _eventSet->getNEvents();
						for (int i = 0; i < n_events; i++) {
							Event *event = _eventSet->getEvent(i);
							Event::LinkedEventMention *mentions = event->getEventMentions();
							while (mentions != 0) {
								int n_args = mentions->eventMention->getNArgs();
								for (int j = 0; j < n_args; j++) {
									const Mention *ment = mentions->eventMention->getNthArgMention(j);
									if (ent == _entitySet->getEntityByMention(ment->getUID(), ment->getEntityType()))
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

			_printAPFEntityHeader(stream, ent, ent->isGeneric(), docName);
			isEntityPrinted[ent->getID()] = true;

			// now print the mentions
			for	(j = 0;	j <	valid_mentions_size; j++) {
				_printAPFMention(stream, validMentions[j], ent, docName);
			}
			if (_create_title_mentions && ent->getType().matchesPER()) {
				_n_printed_titles = 0;

				for (j = 0; j < valid_mentions_size; j++) {
					Mention *ment = validMentions[j];
					if (ment->getMentionType() == Mention::NAME ||
						ment->getMentionType() == Mention::DESC)
						_printAPFTitleMentions(stream, docName, ment, ent);

					if (ment->getMentionType() == Mention::APPO) {
						Mention *child = ment->getChild();
						if (child->getMentionType() == Mention::NAME ||
							child->getMentionType() == Mention::DESC)
							_printAPFTitleMentions(stream, docName, child, ent);

						Mention *nextChild = child->getNext();
						if (!nextChild) continue;
						if (nextChild->getMentionType() == Mention::NAME ||
							nextChild->getMentionType() == Mention::DESC)
							_printAPFTitleMentions(stream, docName, nextChild, ent);
					}
				}
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

	if (_valueSet != NULL && MODE == APF2005) {
		int n_values = _valueSet->getNValues();
		for (int i = 0; i < n_values; i++) {
			Value *val = _valueSet->getValue(i);
			if (val->isTimexValue()) {
				_printAPFTimexHeader(stream, val, docName);
				_printAPFTimexMention(stream, val, docName);
				_printAPFTimexFooter(stream);
			}
			else {
				_printAPFValueHeader(stream, val, docName);
				_printAPFValueMention(stream, val, docName);
				_printAPFValueFooter(stream);
			}
		}
		// document date time field
		const LocatedString *dtString = _docTheory->getDocument()->getDateTimeField();
		if (dtString != 0) {
			// we are faking the UID of the value mention as well as of the value!
			// this needs to be updated if the way we print out TIMEX values ever changes
			stream << L"    <timex2 ID=\"";
			if (!_useAbbreviatedIDs)
				stream << docName << L"-";
			stream << L"T" << MAX_DOCUMENT_VALUES << L"\">\n";
			stream << L"        <timex2_mention ID=\"";
			if(!_useAbbreviatedIDs)
				stream << docName << L"-" << L"T" << MAX_DOCUMENT_VALUES << L"-M";
			stream << L"1\">\n";
			stream << L"            <extent>\n";
			int start = dtString->start<EDTOffset>(0).value();
			int end = dtString->end<EDTOffset>(dtString->length() - 1).value();
			stream << L"              <charseq START=\"" << start << "\" END=\"" << end << "\">";
			_printSafeString(stream, dtString->toString());
			stream << L"</charseq>\n";
			stream << L"            </extent>\n";
			stream << L"        </timex2_mention>\n";
			stream << L"    </timex2>\n\n";
		}
	}

	int max_relation_id = 0;
	if (_relationSet != NULL) {
		int n_rels = _relationSet->getNRelations();
		_printedRelationTimeRole = _new Symbol[_valueSet->getNValues()];
		for (int i = 0; i < n_rels; i++) {
			Relation *rel = _relationSet->getRelation(i);
			//skip identity relations since the scorer doesn't know about them...
			if(rel->getType() == Symbol(L"IDENT")){
				continue;
			}
			
			rel->getMentions()->relMention->toString();

			if (_entitySet->getEntity(rel->getLeftEntityID())->isGeneric() ||
				_entitySet->getEntity(rel->getRightEntityID())->isGeneric())
				continue;

			if (!isEntityPrinted[rel->getLeftEntityID()] ||
				!isEntityPrinted[rel->getRightEntityID()]) 
			{
				SessionLogger::logger->beginWarning();
				*SessionLogger::logger << "ATEAResultCollector::produceOutput(): Relation won't be printed because the component Entities weren't printed!!!!\n";
				*SessionLogger::logger << " Relation ID: " << rel->getID() << "\n";
				*SessionLogger::logger << " Left Entity ID: " << rel->getLeftEntityID() << "\n";
				*SessionLogger::logger << " Right Entity ID: " << rel->getRightEntityID() << "\n";
				*SessionLogger::logger << "Mentions:\n";
				const Relation::LinkedRelMention *mentions = rel->getMentions();
				if (mentions == 0)
					continue;
				while (mentions != 0) {
					RelMention *ment = mentions->relMention;
					*SessionLogger::logger << ment->getLeftMention()->getNode()->toDebugString(0) << "\n";
					*SessionLogger::logger << ment->getRightMention()->getNode()->toDebugString(0) << "\n\n";
					mentions = mentions->next;
				}	
	
				continue;
			}

			// figure out if mentions were valid?
			const Relation::LinkedRelMention *mentions = rel->getMentions();
			if (mentions == 0)
				continue;
			const Relation::LinkedRelMention *mentionscopy = mentions;
			int nvalid =0;
			while(mentionscopy !=0 ){
				if(_isPrintedMention(mentionscopy->relMention->getLeftMention()) &&
					_isPrintedMention(mentionscopy->relMention->getRightMention())){
						nvalid++;
					}				
					mentionscopy = mentionscopy->next;
			}
			if(nvalid == 0){
				continue;
			}



			_printAPFRelationHeader(stream, rel, docName);
			while (mentions != 0) {
				_printAPFRelMention(stream, mentions->relMention, rel, docName);
				mentions = mentions->next;
			}
			_printAPFRelationFooter(stream);
			if (rel->getID() > max_relation_id)
				max_relation_id = rel->getID();
		}
		delete [] _printedRelationTimeRole;
	}

	if (_eventSet != NULL) {
		if(_entitySet == 0){
			//std::cerr<<"ATEAResultCollector::produceOutput(): Warning collecting events with a non existant entity set"<<std::endl;
			SessionLogger::logger->beginWarning();
			*SessionLogger::logger << "ATEAResultCollector::produceOutput(): Warning collecting events with a non existant entity set\n";
			
			_printedEventEntityRole = _new Symbol[0];
		}
		else{
			_printedEventEntityRole = _new Symbol[_entitySet->getNEntities()];
		}
		if(_valueSet == 0){
			//std::cerr<<"ATEAResultCollector::produceOutput: Warning collecting events with a non existant value set"<<std::endl;
			SessionLogger::logger->beginWarning();
			*SessionLogger::logger << "ATEAResultCollector::produceOutput(): Warning collecting events with a non existant value set\n";

			_printedEventValueRole = _new Symbol[0];
		}
		else{
			_printedEventValueRole = _new Symbol[_valueSet->getNValues()];
		}
		int n_events = _eventSet->getNEvents();
		for (int i = 0; i < n_events; i++) {
			Event *event = _eventSet->getEvent(i);

			Event::LinkedEventMention *mentions = event->getEventMentions();
			if (mentions == 0)
				continue;

			_printAPFEventHeader(stream, event, docName);
			while (mentions != 0) {
				_printAPFEventMention(stream, mentions->eventMention, event, docName);
				mentions = mentions->next;
			}
			_printAPFEventFooter(stream);
		}
		delete [] _printedEventEntityRole;
		delete [] _printedEventValueRole;
	}


	// WRITE "UNK" RELATION MENTIONS
	for (int i = 0; i < _docTheory->getNSentences(); i++) {
		int j;
		const PropositionSet *propSet = _docTheory->getSentenceTheory(i)->getPropositionSet();
		for (j=0; j<propSet->getNPropositions(); j++) {
			const Proposition *proposition = propSet->getProposition(j);
			_printValidVerbRelations(stream, proposition, max_relation_id, i, docName);
		}
	}

	if (_print_sentence_boundaries)
		_printSentenceBoundaries(stream, docName);
	if (_print_original_text)
		_printOriginalText(stream);
	if (_print_dump_info)
		_printDumpInfo(stream);

	_printAPFDocumentFooter(stream);
	delete [] isEntityPrinted;
	stream.close();
}

const wchar_t* ATEAResultCollector::_convertMentionTypeToAPF(Mention* ment)
{
	if (MODE == APF2004 && ResultCollectionUtilities::isPREmention(_entitySet, ment))
		return L"PRE";

	Mention::Type type = ment->mentionType;
	Mention* subMent = 0;
	switch (type) {
		case Mention::NAME:
			if (_outputNominalWords && _outputNominalWords->lookup(ment->getHead()->getHeadWord())) 
				return L"NOM";
			else
				return L"NAM";
		case Mention::DESC:
			if (_outputPronounWords && 	_outputPronounWords->lookup(ment->getHead()->getHeadWord())) 
				return L"PRO";
			else if (_outputNameWords && _outputNameWords->lookup(ment->getHead()->getHeadWord()))
				return L"NAM";
			else
				return L"NOM";

		case Mention::PART:
			return L"NOM";
		case Mention::PRON:
			return L"PRO";			
		case Mention::APPO:
			// in ACE 2004, we shouldn't be printing these
			return L"WARNING_BAD_MENTION_TYPE";
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

void ATEAResultCollector::_printAPFDocumentHeader(OutputStream& stream,
												  const wchar_t* doc_name,
												  const wchar_t* doc_source)
{
	if(_includeXMLHeaderInfo) {
		stream << L"<?xml version=\"1.0\"?>\n";
		stream << L"<!DOCTYPE source_file SYSTEM \"ateaSERIF.v1.4.dtd\">\n";
	}
	stream << L"<source_file SOURCE=\"" << doc_source 
		<< L"\" TYPE=\"text\" VERSION=\"2.0\" URI=\"" << doc_name
		<< L"\">\n";
	stream << L"  <document DOCID=\"" << doc_name << L"\">\n\n";
}

void ATEAResultCollector::_printAPFEntityHeader(OutputStream& stream, 
												Entity* ent, 
												bool isGeneric, 
												const wchar_t* doc_name)
{
	stream << L"    <entity ID=\"";
	_printEntityID(stream, doc_name, ent);
	stream << L"\" TYPE=\"" << ent->type.getName().to_string() <<  L"\"";

	if (ent->isFiltered("ATEAEnglishRuledBasedClutterFilter")) {
		stream << L" CLUTTER=\"" << ent->getFilterScore("ATEAEnglishRuledBasedClutterFilter") << "\"";
	}
	if (MODE == APF2005 || !ent->type.matchesPER()) {
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
	stream << L" CLASS=\"";
	// in 2005, we never want to call anything generic, even if we store system-internal
	//  information about whether we think it's generic or not
	if(isGeneric) 
		stream << L"GEN";
	else
		stream << L"SPC";
	
	if (ent->getGUID() != -1) {
		stream << L"\" CDOCID=\"";
		stream << ent->getGUID();
	}
	// Print out canonical name, if exists
	Symbol can_name_symbols[500];
	int n_symbols = -1;
	ent->getCanonicalName(n_symbols, can_name_symbols);
	if (n_symbols > 0) {
		stream << L"\" CANONICAL_NAME=\"";
		for (int i = 0; i < n_symbols; i++) {
			if (i > 0)
				stream << L" ";
			stream << can_name_symbols[i].to_string();
		}
	}

	stream << L"\">\n";

}

void ATEAResultCollector::_printAPFAttributeHeader(OutputStream& stream) 
{
	stream << L"        <entity_attributes>\n";
}

void ATEAResultCollector::_printAPFAttributeFooter(OutputStream& stream) 
{
	stream << L"        </entity_attributes>\n";
}


void ATEAResultCollector::_printAPFMention(OutputStream& stream,
										   Mention* ment,
										   Entity* ent, 
										   const wchar_t* doc_name)
{
	// NOTE: in the rare case that we want to print a list as a mention,
	//       print each of its parts instead.
	if (ment->mentionType == Mention::LIST) {
		Mention* child = ment->getChild();
		while (child != 0) {
			if (child->getMentionType() != Mention::APPO) 
				_printAPFMention(stream, child, ent, doc_name);
			child = child->getNext();
		}
		return;
	}

	stream << L"        <entity_mention TYPE=\"" << _convertMentionTypeToAPF(ment) << L"\"";
	stream << L" ID=\"";
	_printEntityMentionID(stream, doc_name, ment, ent);
	stream << L"\"";

	if (ment->entityType == EntityType::getGPEType() && ment->hasRoleType())
		stream << L" ROLE=\"" << ment->getRoleType().getName().to_string() << "\"";

	if (ment->isMetonymyMention())
		stream << L" METONYMY_MENTION=\"TRUE\"";

	stream << L">\n";

	// print extent info for the whole extent and the head extent.
	const SynNode* node = ment->node;

	int sentNum = ment->getSentenceNumber();
	const SynNode* head = _getEDTHead(ment);
	stream << L"          <extent>\n";
	_printAPFMentionExtent(stream, node, sentNum);
	stream << L"          </extent>\n";
	stream << L"          <head>\n";
	_printAPFMentionExtent(stream, head, sentNum);
	stream << L"          </head>\n";
	stream << L"        </entity_mention>\n";

	if (_printed_mentions_count < MAX_MENTIONS_PER_DOCUMENT) 
		_printedMentions[_printed_mentions_count++] = ment->getUID();
	else throw InternalInconsistencyException("ATEAResultCollector::_printAPFMention()",
			"MAX_MENTIONS_PER_DOCUMENT exceeded");
}

void ATEAResultCollector::_printAPFNameAttribute(OutputStream& stream,
												 Mention* ment)
{
	// If PER ends in number, strip off the number
	int characters_to_remove = 0;
	if (ment->getEntityType() == EntityType::getPERType())
		characters_to_remove = countTrailingDigits(ment);

	stream << L"          <name>\n";
	if (characters_to_remove > 0) {
		_printAPFMentionExtentAfterRemoving(stream, _getEDTHead(ment), ment->getSentenceNumber(), characters_to_remove);
	} else {
		_printAPFMentionExtent(stream, _getEDTHead(ment), ment->getSentenceNumber());
	}
	stream << L"          </name>\n";
}


int ATEAResultCollector::countTrailingDigits(Mention *ment) {
	const SynNode *node = _getEDTHead(ment);
	int end_tok = node->getEndToken();
	int sent_num = ment->getSentenceNumber();

	const Token *token = _tokenSequence[sent_num]->getToken(end_tok);
	std::wstring wstr = token->getSymbol().to_string();

	size_t num_digits = 0;
	for (int i = wstr.length() - 1; i >= 0; i--) {
		if (!iswdigit(wstr.at(i)))
			break;
		num_digits++;
	}
	
	if (num_digits < wstr.length())
		return num_digits;

	return 0;
}


void ATEAResultCollector::_printAPFMentionExtent(OutputStream& stream,
												 const SynNode* node,
												 int sentNum)
{
	int startTok = node->getStartToken();
	int endTok = node->getEndToken();

	_printAPFMentionExtent(stream, startTok, endTok, sentNum);
}

void ATEAResultCollector::_printAPFMentionExtentAfterRemoving(OutputStream& stream,
															  const SynNode* node,
															  int sentNum, 
														      int characters_to_remove)
{
	int startTok = node->getStartToken();
	int endTok = node->getEndToken();

	_printAPFMentionExtentAfterRemoving(stream, startTok, endTok, sentNum, characters_to_remove);
}


void ATEAResultCollector::_printAPFMentionExtent(OutputStream& stream,
												 int startTok,
												 int endTok,
												 int sentNum)
{


	int start = _tokenSequence[sentNum]->getToken(startTok)->getStartEDTOffset().value();
	int end = _tokenSequence[sentNum]->getToken(endTok)->getEndEDTOffset().value(); 

	stream << L"              <charseq START=\"" << start << "\" END=\"" << end << "\">";
	_printAPFTokenSpanText(stream, startTok, endTok, sentNum);
	stream << L"</charseq>\n";
}


void ATEAResultCollector::_printAPFMentionExtentAfterRemoving(OutputStream& stream,
													          int startTok,
												              int endTok,
												              int sentNum, 
															  int characters_to_remove)
{
	int start = _tokenSequence[sentNum]->getToken(startTok)->getStartEDTOffset().value();
	int end = _tokenSequence[sentNum]->getToken(endTok)->getEndEDTOffset().value(); 

	stream << L"              <charseq START=\"" << start << "\" END=\"" << end - characters_to_remove << "\">";
	_printAPFTokenSpanTextAfterRemoving(stream, startTok, endTok, sentNum, characters_to_remove);
	stream << L"</charseq>\n";
}

void ATEAResultCollector::_printAPFTokenSpanText(OutputStream& stream, 
												 int startTok,
												 int endTok,
												 int sentNum) 
{
	int start = _tokenSequence[sentNum]->getToken(startTok)->getStartEDTOffset().value();
	int end = _tokenSequence[sentNum]->getToken(endTok)->getEndEDTOffset().value(); 

	int i;
	for (i=startTok; i<=endTok; i++) {
		// check for ampersands, which can't occur in XML
		// substitute it 
		const wchar_t* symStr = _tokenSequence[sentNum]->getToken(i)->getSymbol().to_string();
		_printSafeString(stream, symStr);		
		if (i != endTok)
			stream << L" ";
	}
}

void ATEAResultCollector::_printAPFTokenSpanTextAfterRemoving(OutputStream& stream, 
												              int startTok,
												              int endTok,
												              int sentNum, 
															  int characters_to_remove) 
{
	int start = _tokenSequence[sentNum]->getToken(startTok)->getStartEDTOffset().value();
	int end = _tokenSequence[sentNum]->getToken(endTok)->getEndEDTOffset().value(); 

	int i;
	for (i=startTok; i<=endTok; i++) {
		// check for ampersands, which can't occur in XML
		// substitute it 
		const wchar_t* symStr = _tokenSequence[sentNum]->getToken(i)->getSymbol().to_string();
		
		if (i == endTok) {
			std::wstring wstr = symStr;
			wstr = wstr.substr(0, wstr.length() - characters_to_remove);
			_printSafeString(stream, wstr.c_str());		
		} else 
			_printSafeString(stream, symStr);		

		if (i != endTok)
			stream << L" ";
	}
}


void ATEAResultCollector::_printSafeString(OutputStream& stream,
										   const wchar_t* origStr)
{
	bool match = wcsstr(origStr, L"&") || wcsstr(origStr, L"<") || wcsstr(origStr, L">");
	// normal case, no ampersand
	if (match == false) {
		if (_is_downcased_doc) {
			// document was originally an all upcase doc, so upcase the output
			size_t j;
			wchar_t* upcaseStr = _new wchar_t[wcslen(origStr) + 1]; 
			for (j = 0; j < wcslen(origStr); j++) 
				upcaseStr[j] = towupper(origStr[j]);
			upcaseStr[j] = L'\0';
			stream << upcaseStr;
			delete [] upcaseStr;
		} else {
			stream << origStr;
		}
	} else {
		size_t orig_len = wcslen(origStr);
		// worst case, it's all ampersands
		wchar_t* safeStr = _new wchar_t[5*orig_len + 1];
		size_t oldIdx = 0;
		size_t newIdx = 0;
		// copy, replacing & with &amp; where appropriate
		for (; oldIdx < orig_len; oldIdx++) {
			if (origStr[oldIdx] == L'&') {
				safeStr[newIdx++] = L'&';
				safeStr[newIdx++] = L'a';
				safeStr[newIdx++] = L'm';
				safeStr[newIdx++] = L'p';
				safeStr[newIdx++] = L';';
			} else if (origStr[oldIdx] == L'<') {
				safeStr[newIdx++] = L'&';
				safeStr[newIdx++] = L'l';
				safeStr[newIdx++] = L't';
				safeStr[newIdx++] = L';';
			} else if (origStr[oldIdx] == L'>') {
				safeStr[newIdx++] = L'&';
				safeStr[newIdx++] = L'g';
				safeStr[newIdx++] = L't';
				safeStr[newIdx++] = L';';
			} else {
				safeStr[newIdx++] = origStr[oldIdx];
			}
		}
		safeStr[newIdx] = L'\0';
		if (_is_downcased_doc) {
			// document was originally an all upcase doc, so upcase the output
			size_t j;
			for (j = 0; j < newIdx; j++) 
				safeStr[j] = towupper(safeStr[j]);
		}
		stream << safeStr;
		delete [] safeStr;
	}		 

}


void ATEAResultCollector::_printAPFValueHeader(OutputStream& stream, 
											   Value* val,  
											   const wchar_t* doc_name)
{
	//<value ID="CNN_CF_20030304.1900.01-V1" TYPE="Numeric" SUBTYPE="Money">
	stream << L"    <value ID=\"";
	_printValueID(stream, doc_name, val);
	stream << L"\"";
	stream << L" TYPE=\"" << val->getType().to_string();
	if (!ValueType::isNullSubtype(val->getSubtype()))
		stream << L"\" SUBTYPE=\"" << val->getSubtype().to_string();
	stream << L"\">\n";
}

void ATEAResultCollector::_printAPFTimexHeader(OutputStream& stream, 
											   Value* val,  
											   const wchar_t* doc_name)
{
	//<timex2 ID="CNN_CF_20030304.1900.01-T1">
	stream << L"    <timex2 ID=\"";
	_printTimexID(stream, doc_name, val);
	stream << L"\"";
	if (val->getTimexVal() != Symbol()) 
		stream << L" VAL=\"" << val->getTimexVal().to_string() << L"\"";
	if (val->getTimexMod() != Symbol())
		stream << L" MOD=\"" << val->getTimexMod().to_string() << L"\"";
	if (val->getTimexSet() != Symbol())
		stream << L" SET=\"" << val->getTimexSet().to_string() << L"\"";
	if (val->getTimexAnchorVal() != Symbol())
		stream << L" ANCHOR_VAL=\"" << val->getTimexAnchorVal().to_string() << L"\"";
	if (val->getTimexAnchorDir() != Symbol())
		stream << L" ANCHOR_DIR=\"" << val->getTimexAnchorDir().to_string() << L"\"";
	if (val->getTimexNonSpecific() != Symbol())
		stream << L" NON_SPECIFIC=\"" << val->getTimexNonSpecific().to_string() << L"\"";
	stream << ">\n";
}

void ATEAResultCollector::_printAPFValueMention(OutputStream& stream,
											    Value* val, 
											    const wchar_t* doc_name)
{
	//  <value_mention ID="CNN_CF_20030304.1900.01-V1-1">
	stream << L"        <value_mention ID=\"";
	_printValueMentionID(stream, doc_name, val);
	stream << L"\">\n";

	//<extent>
    //  <charseq START="253" END="260">$250,000</charseq>
    //</extent>
	stream << L"            <extent>\n";
	_printAPFMentionExtent(stream, val->getStartToken(), val->getEndToken(), 
						 val->getSentenceNumber());
	stream << L"            </extent>\n";

	//  </value_mention>
	stream << L"        </value_mention>\n";
}

void ATEAResultCollector::_printAPFTimexMention(OutputStream& stream,
											    Value* val, 
											    const wchar_t* doc_name)
{
	//  <timex2_mention ID="CNN_CF_20030304.1900.01-T1-1">
	stream << L"        <timex2_mention ID=\"";
	_printTimexMentionID(stream, doc_name, val);
	stream << L"\">\n";

	//<extent>
    //  <charseq START="253" END="260">$250,000</charseq>
    //</extent>
	stream << L"            <extent>\n";
	_printAPFMentionExtent(stream, val->getStartToken(), val->getEndToken(), 
						 val->getSentenceNumber());
	stream << L"            </extent>\n";

	//  </timex2_mention>
	stream << L"        </timex2_mention>\n";
}

void ATEAResultCollector::_printAPFRelationHeader(OutputStream& stream, 
												  Relation* rel,  
												  const wchar_t* doc_name)
{
	Symbol type = rel->getType();

	for (int value = 0; value < _valueSet->getNValues(); value++) {
		_printedRelationTimeRole[value] = Symbol();
	}

	// <relation ID="APW20001002.0615.0146-R1" TYPE="PHYS" SUBTYPE="Part-Whole" 
	//    TENSE="Unspecified" MODALITY="Asserted">
	stream << L"    <relation ID=\"";
	_printRelationID(stream, doc_name, rel);
	stream << L"\"";

	if (rel->isFiltered("ATEAEnglishRuledBasedClutterFilter")) {
		stream << L" CLUTTER=\"" << rel->getFilterScore("ATEAEnglishRuledBasedClutterFilter") 
			<< L"\"";
	}

	stream << L" TYPE=\"" << RelationConstants::getBaseTypeSymbol(type).to_string();
	if (RelationConstants::getSubtypeSymbol(type) != RelationConstants::NONE)
		stream << L"\" SUBTYPE=\"" << RelationConstants::getSubtypeSymbol(type).to_string();

	if (MODE != APF2004) {
		stream << L"\" MODALITY=\"" << rel->getModality().toString();
		stream << L"\" TENSE=\"" << rel->getTense().toString();
	}

	stream << L"\">\n";

	std::wstring arg_name;
	std::wstring id_name;
	std::wstring role_prefix;

	if (MODE == APF2004) {
		arg_name = L"rel_entity_arg";
		id_name = L"ENTITYID";
		role_prefix = L"ARGNUM=\"";
	} else if (MODE == APF2005) {
		arg_name = L"relation_argument";
		id_name = L"REFID";
		role_prefix = L"ROLE=\"Arg-";
	}

	// 2004: <rel_entity_arg ENTITYID="APW20001002.0615.0146-E12" ARGNUM="1" />
  	// 2005: <relation_argument REFID="SAMPLE-E52" ROLE="Arg-1"/>
	stream << L"      <" << arg_name << L" " << id_name << L"=\"";
	_printEntityID(stream, doc_name, rel->getLeftEntityID());
	stream << L"\" " << role_prefix << "1\"/>\n";

	// REPEAT FOR ARG 2
	stream << L"      <" << arg_name << L" " << id_name << L"=\"";
	_printEntityID(stream, doc_name, rel->getRightEntityID());
	stream << L"\" " << role_prefix << "2\"/>\n";

	// REPEAT FOR RELATION TIME
	const Relation::LinkedRelMention *mentions = rel->getMentions();
	while (mentions != 0) {
		if (mentions->relMention->getTimeArgument() != 0) {
			const Value *value = mentions->relMention->getTimeArgument()->getDocValue();
			Symbol role = mentions->relMention->getTimeRole();
			if (value == 0) {
				SessionLogger::logger->beginWarning();
				*SessionLogger::logger << "ATEAResultCollector::_printAPFRelationHeader():";
				*SessionLogger::logger << "No value for value mention ";
				*SessionLogger::logger << mentions->relMention->getTimeArgument()->getUID().toInt() << "\n";
			}
			else if (_printedRelationTimeRole[value->getID()] == Symbol()) {
				_printedRelationTimeRole[value->getID()] = role;
				stream << L"      <" << arg_name << L" " << id_name << L"=\"";
				_printTimexID(stream, doc_name, value);
				stream << L"\" ROLE=\"" << role.to_string() << L"\"/>\n";
			}
		}
		mentions = mentions->next;
	}

}

void ATEAResultCollector::_printAPFRelMention(OutputStream& stream,
											  RelMention* ment,
											  Relation* rel, 
											  const wchar_t* doc_name)
{
	if (!_isPrintedMention(ment->getLeftMention()) ||
		!_isPrintedMention(ment->getRightMention()))
	{
		SessionLogger::logger->beginWarning();
		*SessionLogger::logger << "ATEAResultCollector::_printAPFRelMention(): Relation won't be printed because the component Mentions weren't printed!!!!\n";
		*SessionLogger::logger << "Relation ID: " << rel->getID() << "\n";
		return;
	}

	std::wstring arg_name;
	std::wstring id_name;
	std::wstring role_prefix;

	if (MODE == APF2004) {
		arg_name = L"rel_mention_arg";
		id_name = L"ENTITYMENTIONID";
		role_prefix = L"ARGNUM=\"";
	} else if (MODE == APF2005) {
		arg_name = L"relation_mention_argument";
		id_name = L"REFID";
		role_prefix = L"ROLE=\"Arg-";
	}

	// <relation_mention ID="SAMPLE-R1-1" LEXICALCONDITION="Verbal">
	stream << L"        <relation_mention ID=\"";
	_printRelationMentionID(stream, doc_name, ment, rel);
	stream << "\"";

	if (MODE == APF2004)
		stream << L" LDCLEXICALCONDITION=\"Preposition\"";

	stream << ">\n";

	// 2005 -- FAKED!! Currently this just prints the extent of the left mention...
	if (MODE == APF2005) {
		stream << L"          <extent>\n";
		_printAPFMentionExtent(stream, ment->getLeftMention()->getNode(), 
			ment->getLeftMention()->getSentenceNumber());
		stream << L"          </extent>\n";
	}

	// 2004: <rel_mention_arg ENTITYMENTIONID="12-1" ARGNUM="1">
	// 2005: <relation_mention_argument REFID="SAMPLE-E50-83" ROLE="Arg-1">
	stream << L"          <" << arg_name << L" " << id_name << L"=\"";
	_printEntityMentionID(stream, doc_name, ment->getLeftMention()->getUID(), rel->getLeftEntityID());
    stream << L"\" " << role_prefix << "1\">\n";

	// <extent>
	//   <charseq START="132" END="137">Police</charseq>
    // </extent>
	stream << L"            <extent>\n";
	_printAPFMentionExtent(stream, ment->getLeftMention()->getNode(), 
		ment->getLeftMention()->getSentenceNumber());
	stream << L"            </extent>\n";

	// 2004: </rel_mention_arg>
	// 2005: </relation_mention_argument>
	stream << L"          </" << arg_name << L">\n";

	// REPEAT FOR ARG 2
	stream << L"          <" << arg_name << L" " << id_name << L"=\"";
	_printEntityMentionID(stream, doc_name, ment->getRightMention()->getUID(), rel->getRightEntityID());
    stream << L"\" " << role_prefix << "2\">\n";
	stream << L"            <extent>\n";
	_printAPFMentionExtent(stream, ment->getRightMention()->getNode(), 
		ment->getRightMention()->getSentenceNumber());
	stream << L"            </extent>\n";
	stream << L"          </" << arg_name << L">\n";
	

	// REPEAT FOR RELATION TIME
	if (ment->getTimeArgument() != 0) {
		const Value *value = ment->getTimeArgument()->getDocValue();
		Symbol role = ment->getTimeRole();
		// we will only print times with one role per relation
		if (value == 0) {
			SessionLogger::logger->beginWarning();
			*SessionLogger::logger << "ATEAResultCollector::_printAPFRelMention():";
			*SessionLogger::logger << "No value for value mention ";
			*SessionLogger::logger << ment->getTimeArgument()->getUID().toInt() << "\n";
		}
		else if (_printedRelationTimeRole[value->getID()] == role) {
			stream << L"          <" << arg_name << L" " << id_name << L"=\"";
			_printTimexMentionID(stream, doc_name, value);
			stream << L"\" ROLE=\"" << role.to_string() << L"\">\n";
			stream << L"            <extent>\n";
			_printAPFMentionExtent(stream, value->getStartToken(), value->getEndToken(), 
							   value->getSentenceNumber());
			stream << L"            </extent>\n";
			stream << L"          </" << arg_name << L">\n";
		}
	}
	
	stream << L"        </relation_mention>\n";
}

void ATEAResultCollector::_printAPFEventHeader(OutputStream& stream, 
												  Event* event,  
												  const wchar_t* doc_name)
{
	std::wstring str = event->getType().to_string();
	size_t index = str.find(L".");
	Symbol type = Symbol(str.substr(0, index).c_str());
	Symbol subtype = Symbol(str.substr(index + 1).c_str());

	// <event ID="SAMPLE-EV1" TYPE="Justice" SUBTYPE="Arrest-Jail" MODALITY="Asserted" 
	//		POLARITY="Positive" GENERICITY="Specific" TENSE="Past">
	stream << L"    <event ID=\"";
	_printEventID(stream, doc_name, event);
	stream << L"\"";
	stream << L" TYPE=\"" << type.to_string();
	stream << L"\" SUBTYPE=\"" << subtype.to_string();

	Symbol genericity;
	Symbol polarity;
	Symbol modality;
	Symbol tense;
	stream << L"\" MODALITY=\"" << event->getModality().toString();
	stream << L"\" POLARITY=\"" << event->getPolarity().toString();
	stream << L"\" GENERICITY=\"" << event->getGenericity().toString();
	stream << L"\" TENSE=\"" << event->getTense().toString();
	stream << L"\">\n";

	Event::LinkedEventMention *mentions = event->getEventMentions();
	for (int ent = 0; ent < _entitySet->getNEntities(); ent++) {
		_printedEventEntityRole[ent] = Symbol();
	}
	for (int value = 0; value < _valueSet->getNValues(); value++) {
		_printedEventValueRole[value] = Symbol();
	}

	// N.B.: right now this just prints an entity argument with the first ROLE it comes across
	while (mentions != 0) {
		EventMention *em = mentions->eventMention;
		int i;
		for (i = 0; i < em->getNArgs(); i++) {
			Symbol role = em->getNthArgRole(i);
			const Mention *mention = em->getNthArgMention(i);
			if (!_isPrintedMention(mention)) {
				/*std::cerr << "Argument doesn't exist as an entity: ";
				std::cerr << mention->getNode()->toDebugTextString() << "\n";
				const SynNode* root = mention->getNode();
				while (root->getParent() != 0)
					root = root->getParent();
				std::cerr << "Sentence: " << root->toDebugTextString() << "\n";*/
				continue;
			}
			const Entity *ent = _entitySet->getEntityByMention(mention->getUID());
			if (ent->isGeneric())
				continue;
			int ent_id = ent->getID();
			if (_printedEventEntityRole[ent_id] == Symbol()) {
				_printedEventEntityRole[ent_id] = role;

				// <event_argument REFID="SAMPLE-E5" ROLE="Person"/>
				stream << L"      <event_argument REFID=\"";
				_printEntityID(stream, doc_name, ent_id);
				stream << L"\" ROLE=\"";
				stream << role.to_string() << L"\"/>\n";
			}
		}
		for (i = 0; i < em->getNValueArgs(); i++) {
			Symbol role = em->getNthArgValueRole(i);
			const ValueMention *vmention = em->getNthArgValueMention(i);
			Value *value = _valueSet->getValueByValueMention(vmention->getUID());
			if (value == 0) {
				SessionLogger::logger->beginWarning();
				*SessionLogger::logger << "ATEAResultCollector::_printAPFEventHeader():";
				*SessionLogger::logger << "No value for value mention ";
				*SessionLogger::logger << vmention->getUID().toInt() << "\n";
				continue;
			}
			if (_printedEventValueRole[value->getID()] == Symbol()) {
				_printedEventValueRole[value->getID()] = role;

				// <event_argument REFID="SAMPLE-V5" ROLE="Time-Within"/>
				stream << L"      <event_argument REFID=\"";
				if (value->isTimexValue())
					_printTimexID(stream, doc_name, value);
				else _printValueID(stream, doc_name, value);
				stream << L"\" ROLE=\"" << role.to_string() << L"\"/>\n";
			}
		}
		mentions = mentions->next;
	}

}

void ATEAResultCollector::_printAPFEventMention(OutputStream& stream,
												EventMention* em,
												Event* event,  
												const wchar_t* doc_name)
{
	// <event_mention ID="SAMPLE-EV1-1">
	stream << L"      <event_mention ID=\""; 
	_printEventMentionID(stream, doc_name, em, event);
	stream << L"\">\n";

	// print extent -- FOR NOW WE FAKE THIS USING THE ANCHOR!
	stream << L"        <extent>\n";
	_printAPFMentionExtent(stream, em->getAnchorNode(), em->getSentenceNumber());
	stream << L"        </extent>\n";

	// <anchor>
    //  <charseq START="671" END="678">arrested</charseq>
    // </anchor>
	stream << L"        <anchor>\n";
	_printAPFMentionExtent(stream, em->getAnchorNode(), em->getSentenceNumber());
	stream << L"        </anchor>\n";

	// <event_mention_argument REFID="SAMPLE-E5-7" ROLE="Person">
    //  <extent>
    //    <charseq START="704" END="706">one</charseq>
    //  </extent>
    // </event_mention_argument>
	int arg;
	for (arg = 0; arg < em->getNArgs(); arg++) {
		Symbol role = em->getNthArgRole(arg).to_string();
		const Mention *argMention = em->getNthArgMention(arg);
		if (!_isPrintedMention(argMention))
			continue;
		const Entity *ent = _entitySet->getEntityByMention(argMention->getUID());
		if (ent->isGeneric())
			continue;
		int ent_id = ent->getID();
		// we will only print entities with one role per event
		if (_printedEventEntityRole[ent_id] != role) {
			continue;
		}
		
		stream << L"        <event_mention_argument REFID=\"";
		_printEntityMentionID(stream, doc_name, argMention);
		stream << L"\" ROLE=\"" << role.to_string() << L"\">\n";
		stream << L"          <extent>\n";
		_printAPFMentionExtent(stream, argMention->getNode(), argMention->getSentenceNumber());
		stream << L"          </extent>\n";
		stream << L"        </event_mention_argument>\n";
	}
	
	for (arg = 0; arg < em->getNValueArgs(); arg++) {
		Symbol role = em->getNthArgValueRole(arg).to_string();
		const ValueMention *vmention = em->getNthArgValueMention(arg);
		Value *value = _valueSet->getValueByValueMention(vmention->getUID());	
		
		if (value == 0) {
			SessionLogger::logger->beginWarning();
			*SessionLogger::logger << "ATEAResultCollector::_printAPFEventHeader():";
			*SessionLogger::logger << "No value for value mention ";
			*SessionLogger::logger << vmention->getUID().toInt() << "\n";
			continue;
		}
			
		// we will only print values with one role per event
		if (_printedEventValueRole[value->getID()] != role) {
			continue;
		}
		
		stream << L"        <event_mention_argument REFID=\"";		
		if (value->isTimexValue())
			_printTimexMentionID(stream, doc_name, value);
		else _printValueMentionID(stream, doc_name, value);
		stream << L"\" ROLE=\"" << role.to_string() << L"\">\n";
		stream << L"          <extent>\n";
		_printAPFMentionExtent(stream, value->getStartToken(), value->getEndToken(), 
						 value->getSentenceNumber());
		stream << L"          </extent>\n";
		stream << L"        </event_mention_argument>\n";
	}

	stream << L"      </event_mention>\n"; 

}


void ATEAResultCollector::_printAPFEntityFooter(OutputStream& stream)
{
	stream << L"    </entity>\n\n";
}

void ATEAResultCollector::_printAPFValueFooter(OutputStream& stream)
{
	stream << L"    </value>\n\n";
}

void ATEAResultCollector::_printAPFTimexFooter(OutputStream& stream)
{
	stream << L"    </timex2>\n\n";
}

void ATEAResultCollector::_printAPFRelationFooter(OutputStream& stream)
{
	stream << L"    </relation>\n\n";
}

void ATEAResultCollector::_printAPFEventFooter(OutputStream& stream)
{
	stream << L"    </event>\n\n";
}

void ATEAResultCollector::_printAPFDocumentFooter(OutputStream& stream)
{
	stream << L"  </document>\n";
	stream << L"</source_file>\n";
}

bool ATEAResultCollector::_isTopMention(const Mention* ment, EntityType type)
{
	if (ment->getParent() == 0)
		return true;
	if (ment->getParent()->getMentionType() == Mention::PART)
		return true;

	Entity* ent = _entitySet->getEntityByMention(ment->getUID(), type);
	Entity* parentEnt = _entitySet->getEntityByMention(ment->getParent()->getUID(), type);
	if (ent == 0) {
		string err = "current mention has no entity!\nAt node:\n";
		err.append(ment->node->toDebugString(0));
		err.append("\n");
		err.append("Mention was ");
		err.append("(");
		err.append(ment->entityType.getName().to_debug_string());
		err.append(" - ");
		err.append(Mention::getTypeString(ment->mentionType));
		err.append(")\n");
		throw InternalInconsistencyException("ATEAResultCollector::_isTopMention()",
			(char*)err.c_str());

	}
	if (parentEnt != 0 && parentEnt->getID() == ent->getID())
		return false;

	

	return true;
}

bool ATEAResultCollector::_isItemOfUnprintedList(const Mention* ment)
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

bool ATEAResultCollector::_isPartOfAppositive(const Mention* ment)
{
	// Top of appositive doesn't count
	if (ment->mentionType == Mention::APPO)
		return false;
	if (ment->mentionType == Mention::NONE)
		return false;

	Mention* parent = ment->getParent();
	if (parent == 0)
		return false;
	if (parent->mentionType != Mention::APPO)
		return false;
	// parent --> child (part 1) --> next (part 2)
	Mention* c1 = parent->getChild();
	if (c1 == 0)
		return false;
	if (c1 == ment)
		return true;
	Mention* c2 = c1->getNext();
	if (c2 == 0)
		return false;
	if (c2 == ment)
		return true;
	return false;
}

// TODO: need role information before this is useful
bool ATEAResultCollector::_isGPEModifierOfPersonGPE(const Mention* ment)
{
	return false;
}

// mention must be a name, must have a parent whose edt head is different
// and must have a parent whose entity is different
bool ATEAResultCollector::_isNameNotInHeadOfParent(const Mention* ment, EntityType type)
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
				throw InternalInconsistencyException("ATEAResultCollector::_isNameNotInHeadOfParent()",
				"current mention has no entity!");
			if (parEnt->getID() == ent->getID())
				return false;
		}
	}
	return true;
}

bool ATEAResultCollector::_sharesHeadWithAlreadyPrintedMention(const Mention* ment)
{
	int sentNum = ment->getSentenceNumber();
	const MentionSet *mentionSet = _entitySet->getMentionSet(sentNum);
	const SynNode *head = _getEDTHead(ment);
	
	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		const Mention *other = mentionSet->getMention(i);
		const SynNode *otherHead = _getEDTHead(other);
		if ((other != ment) && _isPrintedMention(other) && 
			_doesOverlap(head, otherHead, ment->getSentenceNumber(), other->getSentenceNumber()))
		{
			return true;
		}
	}
	return false;
}

bool ATEAResultCollector::_sharesHeadWithEarlierMentionOfSameEntity(const Mention* ment,
																	Entity *ent)
{
	const SynNode *head = _getEDTHead(ment);

	GrowableArray<MentionUID> mens = ent->mentions;
	for	(int j = 0;	j <	mens.length(); j++)	{
		const Mention *other = _entitySet->getMention(mens[j]);
		const SynNode *otherHead = _getEDTHead(other);

		if (ment == other)
			return false;
		
		if (_doesOverlap(head, otherHead, ment->getSentenceNumber(), other->getSentenceNumber()) 
			&& _isPrintableMention(other, ent)) 
		{
			return true;
		}
	}
	return false;
}

bool ATEAResultCollector::_doesOverlap(const SynNode *node1, const SynNode *node2, int sent_num1, int sent_num2)
{
	int startToken1 = node1->getStartToken();
	int endToken1 = node1->getEndToken();

	int startToken2 = node2->getStartToken();
	int endToken2 = node2->getEndToken();

	EDTOffset start1 = _tokenSequence[sent_num1]->getToken(startToken1)->getStartEDTOffset();
	EDTOffset end1 = _tokenSequence[sent_num1]->getToken(endToken1)->getEndEDTOffset();

	EDTOffset start2 = _tokenSequence[sent_num2]->getToken(startToken2)->getStartEDTOffset();
	EDTOffset end2 = _tokenSequence[sent_num2]->getToken(endToken2)->getEndEDTOffset();

	if ((start1 >= start2 && start1 <= end2) ||
		(end1 >= start2 && end1 <= end2)) 
	{
		return true;
	}

	if ((start2 >= start1 && start2 <= end1) ||
		(end2 >= start1 && end2 <= end1)) 
	{
		return true;
	}

	return false;
}

const SynNode* ATEAResultCollector::_getEDTHead(const Mention* ment)
{
	if (ment == 0)
		throw InternalInconsistencyException("ATEAResultCollector::_getEDTHead()",
		"mention does not exist");

	const SynNode* node = ment->node;
	if (ment->mentionType == Mention::NAME) {
		const Mention* menChild = ment;
		const SynNode* head = menChild->node;
		while((menChild = menChild->getChild()) != 0)
			head = menChild->node;			
		return head;
	}
	if (ment->mentionType == Mention::APPO) {
		Mention* menChild = ment->getChild();
		if (menChild == 0)
			throw InternalInconsistencyException("ATEAResultCollector::_getEDTHead()",
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

bool ATEAResultCollector::_isPrintableMention(const Mention *ment, Entity *ent) {
	if(MODE == APF2005){
		
		//Note: this is a fairly english specific hack to avoid printing 
		//mentions in bn/cts that are not in the non-speaker turn regions of the document.
		//If it causes problems, we could do language specific utilities for the APFCollector

		//Simple rule for non-CA Serif would be, if your most immediate covering span is turn, 
		//otherwise don't print, but in CASerif, enamex spans are added, so these need to be ignored
		/*if((_docTheory->getSourceType() == Symbol(L"broadcast conversation"))  ||
			_docTheory->getSourceType() == Symbol(L"telephone")){
				int sentnum = ment->getSentenceNumber();
				int sent_offset = _docTheory->getSentence(sentnum)->getStartCharNumber();
				Metadata* metadata = _docTheory->getMetadata();
				if(metadata != 0){
					Span* turnspan = metadata->getCoveringSpan(sent_offset, Symbol(L"TURN"));
					Span* speakerspan = metadata->getCoveringSpan(sent_offset, Symbol(L"SPEAKER"));
					if((turnspan == 0) || (speakerspan != 0)){
						return false;
					}
				}
			}*/
	}
	//if(ment->getMentionType() == Mention::NONE){
	//	return false;
	//}	
	if (_filterOverlappingMentions &&
		(_sharesHeadWithAlreadyPrintedMention(ment) ||
		_sharesHeadWithEarlierMentionOfSameEntity(ment, ent)))
	{
		return false;
	}

	return (_isPartOfAppositive(ment)	 || 
			_isGPEModifierOfPersonGPE(ment) ||
		   (_isTopMention(ment, ent->getType()) && ment->getMentionType() != Mention::APPO) ||
			_isNameNotInHeadOfParent(ment, ent->getType()) ||
		    _isItemOfUnprintedList(ment));
}

bool ATEAResultCollector::_isPrintedMention(const Mention *ment) {
	int i;
	for (i = 0; i < _printed_mentions_count; i++) 
		if (_printedMentions[i] == ment->getUID()) 
			return true;

	return false;
}

// ENTITY ID: docname-E0 or E0
// ENTITY MENTION ID: docname-E0-M0 or 0
// VALUE ID: docname-V0 or V0
// VALUE MENTION ID: docname-V0-M0 or 0
// RELATION ID: docname-R0 or R0
// RELATION MENTION ID: docname-R0-M0 or 0
// EVENT ID: docname-V0 or V0
// EVENT MENTION ID: docname-V0-M0 or 0

void ATEAResultCollector::_printEntityID(OutputStream& stream, const wchar_t* doc_name, Entity *ent) {
	_printEntityID(stream, doc_name, ent->ID);
}
void ATEAResultCollector::_printEntityID(OutputStream& stream, const wchar_t* doc_name, int id) {
	if (!_useAbbreviatedIDs)
		stream << doc_name << L"-";
	stream << L"E" << id;
}
void ATEAResultCollector::_printEntityMentionID(OutputStream& stream, const wchar_t* doc_name, 
												const Mention *ment) 
{
	_printEntityMentionID(stream, doc_name, ment, _entitySet->getEntityByMention(ment->getUID()));
}
void ATEAResultCollector::_printEntityMentionID(OutputStream& stream, const wchar_t* doc_name, 
												const Mention *ment, Entity *ent) 
{
	_printEntityMentionID(stream, doc_name, ment->getUID(), ent->ID);
}
void ATEAResultCollector::_printEntityMentionID(OutputStream& stream, const wchar_t* doc_name, 
												MentionUID mid, int eid) 
{
	if (!_useAbbreviatedIDs) {
		_printEntityID(stream, doc_name, eid);
		stream << L"-M";
	}
	stream << mid;
}
void ATEAResultCollector::_printValueID(OutputStream& stream, 
										const wchar_t* doc_name, 
										const Value *val) 
{
	if (!_useAbbreviatedIDs)
		stream << doc_name << L"-";
	stream << L"V" << val->getID();
}
void ATEAResultCollector::_printValueMentionID(OutputStream& stream, 
											   const wchar_t* doc_name, 
											   const Value *val) 
{
	if(!_useAbbreviatedIDs) {
		_printValueID(stream, doc_name, val);
		stream << L"-M";
	}
	stream << L"1";
}
void ATEAResultCollector::_printTimexID(OutputStream& stream, 
										const wchar_t* doc_name, 
										const Value *val) 
{
	if (!_useAbbreviatedIDs)
		stream << doc_name << L"-";
	stream << L"T" << val->getID();
}
void ATEAResultCollector::_printTimexMentionID(OutputStream& stream, 
											   const wchar_t* doc_name, 
											   const Value *val) 
{
	if(!_useAbbreviatedIDs) {
		_printTimexID(stream, doc_name, val);
		stream << L"-M";
	}
	stream << L"1";
}
void ATEAResultCollector::_printRelationID(OutputStream& stream, const wchar_t* doc_name, Relation *rel) {
	if (!_useAbbreviatedIDs)
		stream << doc_name << L"-";
	stream << L"R" << rel->getID();
}
void ATEAResultCollector::_printRelationMentionID(OutputStream& stream, 
												  const wchar_t* doc_name, 
												  RelMention *rment, Relation *rel) 
{
	if(!_useAbbreviatedIDs) {
		_printRelationID(stream, doc_name, rel);
		stream << L"-M";
	}
	stream << rment->getUID().toInt();
}
void ATEAResultCollector::_printEventID(OutputStream& stream, const wchar_t* doc_name, Event *event) {
	if (!_useAbbreviatedIDs)
		stream << doc_name << L"-";
	stream << L"EV" << event->getID();
}
void ATEAResultCollector::_printEventMentionID(OutputStream& stream, const wchar_t* doc_name, 
											   EventMention *evment, Event *event) 
{
	if(!_useAbbreviatedIDs) {
		_printEventID(stream, doc_name, event);
		stream << L"-M";
	}
	stream << evment->getUID().toInt();
}

void ATEAResultCollector::_printSentenceBoundaries(OutputStream &stream, const wchar_t *doc_name)
{
	for (int i = 0; i < _docTheory->getNSentences(); i++) {
		SentenceTheory *st = _docTheory->getSentenceTheory(i);
		TokenSequence *ts = st->getTokenSequence();
		if (ts->getNTokens() < 1) continue;
		stream << L"<sentence ID=\"";
		stream << doc_name << L"-S" << i << "\">\n";
		stream << L"  <charseq START=\"" << ts->getToken(0)->getStartEDTOffset() 
			   << "\" END=\"" << ts->getToken(ts->getNTokens() - 1)->getEndEDTOffset() << "\"/>\n";
		
		stream << L"</sentence>\n";
	}
}

void ATEAResultCollector::_printOriginalText(OutputStream &stream)
{

	Document *doc = _docTheory->getDocument();
	const LocatedString *text = doc->getOriginalText();

	if (!text) return;

	LocatedString textCopy(*text);
/*	bool in_tag = false;
	int start = 0;
	int i = 0;
	while (i < textCopy.length()) {
		if (textCopy.charAt(i) == L'\n') {
			in_tag = false;
		}
		if (textCopy.charAt(i) == L'<') {
			in_tag = true;
			start = i;
		}
		if (textCopy.charAt(i) == L'>' && in_tag) {
			textCopy.remove(start, i + 1);
			i -= (i - start + 1);
			in_tag = false;
		}	
		i++;
	}*/

	stream << L"    <source>\n";
	stream << L"    <!--";
	
	textCopy.replace(L"--", L"  ");
	stream << textCopy.toString();

	stream << "-->\n    </source>\n";
}


// to be valid, a prop must be VERB_PRED and have both a subject and either (object or indirect-object), such that
//  NEITHER is an unknown-entity mention
void ATEAResultCollector::_printValidVerbRelations(OutputStream &stream,
													const Proposition *proposition,
													const int max_relation_id,
													const int sentence_num,
													const wchar_t* doc_name)
{
	const MentionSet *mentions = _docTheory->getSentenceTheory(sentence_num)->getMentionSet();
	if(proposition->getPredType() == Proposition::VERB_PRED) {
		Symbol verb = proposition->getPredSymbol();
		const Mention *sub=0, *anyObj = 0;
		int i;
		Symbol prepSym = Symbol();

		// find a subject
		for (i=0; i<proposition->getNArgs(); i++) {
			const Argument *arg = proposition->getArg(i);
			if(arg->getType() == Argument::MENTION_ARG) {
				const Mention *mention = arg->getMention(mentions);
				Symbol role = arg->getRoleSym();
				if(_isPrintedMention(mention))
					if(role == Argument::SUB_ROLE)
						sub = mention;
			}
		}

		if (!sub) return;

		// if there's a subject, then print out relations between that subject
		// and any object or indirect object
		for (i=0; i<proposition->getNArgs(); i++) {
			const Argument *arg = proposition->getArg(i);
			if(arg->getType() == Argument::MENTION_ARG) {
				Symbol role = arg->getRoleSym();
				if (role != Argument::OBJ_ROLE && role != Argument::IOBJ_ROLE) continue; // for optimization

				const Mention *mention = arg->getMention(mentions);
				anyObj = mention;

				if(_isPrintedMention(mention) &&
				   !_relationBetween(sub, mention))
				{
					if (role == Argument::OBJ_ROLE) {
						_printVerbRelation(sub, mention, SUB, OBJ, stream,
							doc_name, verb, max_relation_id);
					}
					if (role == Argument::IOBJ_ROLE) {
						_printVerbRelation(sub, mention, SUB, IOBJ, stream,
							doc_name, verb, max_relation_id);
					}
				}
			}
		}

		if (anyObj) return;

		// if there isn't an object or direct object, print out any preposition
		// relations
		for (i=0; i<proposition->getNArgs(); i++) {
			const Argument *arg = proposition->getArg(i);
			if(arg->getType() == Argument::MENTION_ARG) {
				Symbol role = arg->getRoleSym();
				if(!WordConstants::isUnknownRelationReportedPreposition(role)) continue; // for optimization
				const Mention *mention = arg->getMention(mentions);

				if(WordConstants::isUnknownRelationReportedPreposition(role) &&
					_isPrintedMention(mention) &&
					!_relationBetween(sub, mention) &&
					!_eventBetween(sub, mention))
				{
					_printVerbRelation(sub, mention, SUB, role,
						stream, doc_name, verb, max_relation_id);
				}
			}
		}
	}
}

bool ATEAResultCollector::_relationBetween(const Mention *arg1, const Mention *arg2)
{
	int n_rels = _relationSet->getNRelations();
	for (int i = 0; i < n_rels; i++) {
		Relation *rel = _relationSet->getRelation(i);
		const Relation::LinkedRelMention *mentions = rel->getMentions();
		if (mentions == 0)
			continue;

		while (mentions != 0) {
			RelMention *relMention = mentions->relMention;
			const Mention *ment1 = relMention->getLeftMention();
			const Mention *ment2 = relMention->getRightMention();

			if (ment1->getUID() == arg1->getUID() && ment2->getUID() == arg2->getUID() ||
				ment2->getUID() == arg1->getUID() && ment1->getUID() == arg2->getUID())
				return true;

			mentions = mentions->next;
		}
	}
	return false;
}

bool ATEAResultCollector::_eventBetween(const Mention *arg1, const Mention *arg2)
{
	int n_events = _eventSet->getNEvents();
	
	bool found_arg1, found_arg2;

	for (int i = 0; i < n_events; i++) {
		
		Event *event = _eventSet->getEvent(i);
		Event::LinkedEventMention *mentions = event->getEventMentions();
		while (mentions != 0) {
			found_arg1 = false;
			found_arg2 = false;

			int n_args = mentions->eventMention->getNArgs();
			for (int j = 0; j < n_args; j++) {

				const Mention *ment = mentions->eventMention->getNthArgMention(j);
				if (ment->getUID() == arg1->getUID()) 
					found_arg1 = true;
				
				if (ment->getUID() == arg2->getUID())		
					found_arg2 = true;
			
			}
			if (found_arg1 && found_arg2) return true;
			mentions = mentions->next;
		}
	}

	return false;
}
		


void ATEAResultCollector::_printVerbRelation(const Mention *arg1, const Mention *arg2, Symbol role1, Symbol role2,
											 OutputStream &stream, const wchar_t* doc_name,
											 Symbol verb, int max_relation_id)
{

	Entity *ent1 = _entitySet->getEntityByMention(arg1->getUID());
	Entity *ent2 = _entitySet->getEntityByMention(arg2->getUID());

	if (!ent1 || !ent2) return;

	int current_relation_id = max_relation_id + ++_unknown_relation_count;

	verb = RelationUtilities::get()->stemPredicate(verb, Proposition::VERB_PRED);
	stream << L"    <relation ID=\"" << doc_name << L"-R" << current_relation_id << "\"";

	if (_clutterFilter != 0) {
		// now try to filter this relation by construct a dummy relation
		Symbol type (L"UNKNOWN");
		// make sure the dummy Relation is not already in the filter's hash
		int dummyRelationID (0xbadf00d + current_relation_id);
		RelMention *relment = _new RelMention(arg1, arg2, type, 0, dummyRelationID, 0.);
		{
			Relation rel (relment, ent1->getID(), ent2->getID(), dummyRelationID);
#if 0
			cout << "adding unknown relation " << rel.getID() 
				<< " for E" << ent1->getID() << " and E" << ent2->getID() << endl;
#endif
			rel.applyFilter("ATEAEnglishRuledBasedClutterFilter", _clutterFilter);
			if (rel.isFiltered("ATEAEnglishRuledBasedClutterFilter")) {
				stream << L" CLUTTER=\"" << rel.getFilterScore("ATEAEnglishRuledBasedClutterFilter") << "\"";
			}
		}
		delete relment;
	}
	stream << L" TYPE=\"UNKNOWN\" VERB=\"" << verb.to_string() << L"\">\n";

	stream << L"      <relation_argument REFID=\"";
	stream << doc_name << L"-E" << ent1->getID();
	stream << L"\" ROLE=\"" << role1.to_string() << "\"/>\n";

	stream << L"      <relation_argument REFID=\"";
	stream << doc_name << L"-E" << ent2->getID();
	stream << L"\" ROLE=\"" << role2.to_string() << "\"/>\n";

	stream << L"        <relation_mention ID=\"";
	stream << doc_name << L"-R" << current_relation_id << L"-M1\">\n";

	stream << L"          <extent>\n";
	_printAPFMentionExtent(stream, arg1->getNode(), arg1->getSentenceNumber());
	stream << L"          </extent>\n";

	stream << L"          <relation_mention_argument REFID=\"";
	stream << doc_name << L"-E" << ent1->getID() << L"-M" << arg1->getUID();
	stream << L"\" ROLE=\"" << role1.to_string() << "\">\n";
	stream << L"            <extent>\n";
	_printAPFMentionExtent(stream, arg1->getNode(), arg1->getSentenceNumber());
	stream << L"            </extent>\n";
	stream << L"          </relation_mention_argument>\n";

	stream << L"          <relation_mention_argument REFID=\"";
	stream << doc_name << L"-E" << ent2->getID() << L"-M" << arg2->getUID();
	stream << L"\" ROLE=\"" << role2.to_string() << "\">\n";
	stream << L"            <extent>\n";
	_printAPFMentionExtent(stream, arg2->getNode(), arg2->getSentenceNumber());
	stream << L"            </extent>\n";
	stream << L"          </relation_mention_argument>\n";
	stream << L"        </relation_mention>\n";

	stream << L"    </relation>\n\n";

}

void ATEAResultCollector::_printAPFTitleMentions(OutputStream& stream, 
												 const wchar_t *doc_name,
									             Mention* ment,
									             Entity* ent)
{
	int mention_length;
	Symbol terminals[MAX_TOKENS_PER_MENTION];

	const SynNode *node = ment->getNode();
	mention_length = node->getTerminalSymbols(terminals, MAX_TOKENS_PER_MENTION);

	// for names, don't look past the head
	if (ment->getMentionType() == Mention::NAME) {
		int head_token = ment->getHead()->getStartToken();
		int last_token = ment->getNode()->getLastTerminal()->getEndToken();
		mention_length -= (last_token - head_token + 1);
	}

	int i;
	for (i = 0; i < mention_length; i++) {
		const SynNode *currentTerminal = node->getNthTerminal(i);

		TitleListNode *possibleTitle;
		//std::cout << terminals[i].to_debug_string() << " ";
	
		if (possibleTitle = _titles->lookup(terminals[i])) {
			while (possibleTitle != NULL) {

				if (_matchTitle(terminals, i, mention_length, possibleTitle)) {

					// any title for a descriptor must contain a head
					if (ment->getMentionType() == Mention::DESC &&
						!_containsHead(ment, i, possibleTitle->getTitleLength()))
					{
						possibleTitle = possibleTitle->getNext();
						continue;
					}

					// check to see if we found this title in another mention
					int j;
					bool found_title = false;
					for (j = 0; j < _n_printed_titles; j++) {
						if (possibleTitle == _printedTitles[j]) {
							found_title = true;
							break;
						}
					}

					if (!found_title) {
						_printTitle(stream, doc_name, ment, ent, i, possibleTitle->getTitleLength());
						if (_n_printed_titles < MAX_TITLES_PER_ENTITY)
							_printedTitles[_n_printed_titles++] = possibleTitle;
						else {
							SessionLogger::logger->beginWarning();
							*SessionLogger::logger << "MAX_TITLES_PER_DOCUMENT exceeded.\n";
							*SessionLogger::logger << "Might duplicate some titles.\n";
						}
					}

					// skip over the words in the mention that are used by the title
					i = i + possibleTitle->getTitleLength() - 1;
					break;
				}
				possibleTitle = possibleTitle->getNext();
			}
		}
	}
}

bool ATEAResultCollector::_containsHead(Mention *ment, int start, int length)
{
	const SynNode *node = ment->getNode();
	int i;
	for (i = start; i < start + length; i++) {
		if (node->getNthTerminal(i)->getParent() == node->getHeadPreterm())
			return true;
	}
	return false;
}

void ATEAResultCollector::_printTitle(OutputStream &stream, const wchar_t *doc_name, Mention *ment,
									 Entity *ent, int start_within_mention, int length)
{
	int sent_num = ment->getSentenceNumber();

	const SynNode *startTerminal = ment->getNode()->getNthTerminal(start_within_mention);
	const SynNode *endTerminal = ment->getNode()->getNthTerminal(start_within_mention + length - 1);

	int startTok = startTerminal->getStartToken();
	int endTok = endTerminal->getEndToken();

	stream << L"        <entity_mention TYPE=\"ROLE\"";
	stream << L" ID=\"" << doc_name << L"-E" << ent->ID << L"-M" << ment->getUID() << L"-T" << start_within_mention << L"\"";
	stream << L">\n";

	int sentNum = ment->getSentenceNumber();
	stream << L"          <extent>\n";

	int i;

	EDTOffset start = _tokenSequence[sentNum]->getToken(startTok)->getStartEDTOffset();
	EDTOffset end = _tokenSequence[sentNum]->getToken(endTok)->getEndEDTOffset();

	stream << L"              <charseq START=\"" << start << "\" END=\"" << end << "\">";
	for (i=startTok; i<=endTok; i++) {
		// check for double dash, which can't occur inside a comment
		// substitute it
		const wchar_t* symStr = _tokenSequence[sentNum]->getToken(i)->getSymbol().to_string();

		const wchar_t* match = wcsstr(symStr, L"--");
		
		if (i != startTok) stream << L" ";
		// normal case. no double dash
		if (match == 0)
			stream << symStr;
		else {
			int orig_len = wcslen(symStr);
			// worst case, it's all dashes
			wchar_t* safeStr = _new wchar_t[2*orig_len];
			int oldIdx = 0;
			int newIdx = 0;
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
		
	}
	
	stream << L"</charseq>\n";
	stream << L"          </extent>\n";

	stream << L"        </entity_mention>\n";
}

bool ATEAResultCollector::_matchTitle(const Symbol *terminals, int terminals_pos, int terminals_length,
									  TitleListNode *title)
{
	int title_pos;
	for (title_pos = 0; title_pos < title->getTitleLength(); title_pos++) {
		if (terminals_pos >= terminals_length) return false;
		if (terminals[terminals_pos] != title->getTitleWord(title_pos)) return false;

		terminals_pos++;
	}
	return true;
}

void ATEAResultCollector::_printDumpInfo(OutputStream &stream)
{
	for (int i = 0; i < _docTheory->getNSentences(); i++) {
		const SentenceTheory *st = _docTheory->getSentenceTheory(i);
		stream << L"  <sentence_theory sentence_id=\"" << i << L"\">\n";
	 
		const TokenSequence *ts = st->getTokenSequence();
		for (int j = 0; j < ts->getNTokens(); j++) {
			const Token *t = ts->getToken(j);

			stream << L"    <token token_id=\"" << j << L"\" word=\"" << t->getSymbol().to_string() << L"\" start_offset=\"" << 
				t->getStartEDTOffset() << L"\" end_offset=\"" << t->getEndEDTOffset() << L"\"/>\n";
		}

		const MentionSet *ms = st->getMentionSet();
		for (int j = 0; j < ms->getNMentions(); j++) {
			const Mention *m = ms->getMention(j);
			stream << L"    <mention mention_id=\"" << m->getUID() << L"\" mention_type=\"" << 
				Mention::getTypeString(m->mentionType) << L"\" entity_type=\"" <<
				m->getEntityType().getName().to_string() << L"\"";
			if (m->hasIntendedType())
				stream << L" intended_type=\"" << m->getIntendedType().getName().to_string() << L"\"";
			if (m->hasRoleType())
				stream << L" role_type=\"" << m->getRoleType().getName().to_string() << L"\"";
			stream << L" entity_subtype=\"" << m->getEntitySubtype().getName().to_string() << L"\"";
			stream << L" parse_node_id=\"" << m->getNode()->getID() << L"\"";
			if (m->getParent()) 
				stream << L" parent_mention_id=\"" << m->getParent()->getUID() << L"\"";
			if (m->getChild()) 
				stream << L" child_mention_id=\"" << m->getChild()->getUID() << L"\"";
			if (m->getNext()) 
				stream << L" next_mention_id=\"" << m->getNext()->getUID() << L"\"";
			stream << L"/>\n";
		}

		const Parse *p = st->getPrimaryParse();
		_printParse(stream, p->getRoot(), 0, ms);

		const PropositionSet *ps = st->getPropositionSet();
		for (int j = 0; j < ps->getNPropositions(); j++) {
			Proposition *pr = ps->getProposition(j);
			stream << L"    <proposition proposition_id=\"" << pr->getID() << L"\"";
			if (pr->getPredHead() != 0) 
				stream << L" pred_head_word=\"" << pr->getPredHead()->getHeadWord().to_string() << L"\"";
			stream << L" pred_type=\"" << Proposition::getPredTypeString(pr->getPredType()) << L"\"";
			if (pr->getParticle() != 0)
				stream << L" particle_head_word=\"" << pr->getParticle()->getHeadWord().to_string() << L"\"";
			if (pr->getAdverb() != 0)
				stream << L" adverb_head_word=\"" << pr->getAdverb()->getHeadWord().to_string() << L"\"";
			if (pr->getNegation() != 0)
				stream << L" negation_head_word=\"" << pr->getNegation()->getHeadWord().to_string() << L"\"";
			if (pr->getModal() != 0)
				stream << L" modal_head_word=\"" << pr->getModal()->getHeadWord().to_string() << L"\"";
			stream << L">\n";
			
			for (int k = 0; k < pr->getNArgs(); k++) {
				Argument *a = pr->getArg(k);
				stream << L"      <argument";
				if (a->getRoleSym() != Symbol()) {
					LocatedString role(a->getRoleSym().to_string());
					role.replace(L"<", L"&lt;");
					role.replace(L">", L"&gt;");
					stream << L" role=\"" << role.toString() << L"\"";
				}
				if (a->getType() == Argument::MENTION_ARG)
					stream << L" mention_id=\"" << a->getMention(st->getMentionSet())->getUID() << L"\"";
				else if (a->getType() == Argument::PROPOSITION_ARG)
					stream << L" proposition_id=\"" << a->getProposition()->getID() << L"\"";
				else if (a->getType() == Argument::TEXT_ARG) {
					stream << " text=\"";
					Symbol symArray[MAX_SENTENCE_TOKENS];
					int n_tokens = a->getNode()->getTerminalSymbols(symArray, MAX_SENTENCE_TOKENS);
					if (n_tokens > 0)
						stream << symArray[0].to_string();
					for (int i = 1; i < n_tokens; i++)
						stream << L" " << symArray[i].to_string();
					stream << L"\"";
				}
				stream << L"/>\n";
			}
			stream << L"    </proposition>\n";
		}
		stream << L"  </sentence_theory>\n";
	}
}

void ATEAResultCollector::_printParse(OutputStream &stream, const SynNode *node, const SynNode *parent, const MentionSet *ms)
{
	stream << L"    <parse_node node_id=\"" << node->getID() << L"\"";
	if (node->isTerminal()) 
		stream << L" word=\"" << node->getTag().to_string() << "\"";
	else 
		stream << L" tag=\"" << node->getTag().to_string() << "\"";

	stream << " start_token_id=\"" << node->getStartToken() << L"\" end_token_id=\"" << node->getEndToken() << L"\"";
	if (parent != 0) 
		stream << L" parent_node_id=\"" << parent->getID() << "\"";
	if (node->hasMention()) {
		const Mention *m = ms->getMention(node->getMentionIndex());
		stream << L" mention_id=\"" << m->getUID() << "\"";
	}

	stream << L"/>\n";

	for (int i = 0; i < node->getNChildren(); i++) {
		_printParse(stream, node->getChild(i), node, ms);
	}
}

