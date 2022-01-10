// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/OStringStream.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/EventSet.h"
#include "Generic/theories/Value.h"
#include "Generic/theories/ValueSet.h"
#include "Generic/theories/ValueType.h"
#include "Generic/theories/EventEntityRelation.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/RelationSet.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/theories/Relation.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/values/TemporalNormalizer.h"
#include "Generic/clutter/ACE2008EvalClutterFilter.h"
#include "Generic/results/ResultCollectionUtilities.h"
#include "Generic/apf/APF4GenericResultCollector.h"
#include "Generic/common/version.h"


using namespace std;

#define PRINT_PRONOUN_LEVEL_ENTITIES true

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

APF4GenericResultCollector::APF4GenericResultCollector(int mode_) 
	: _docTheory(0), _entitySet(0), _valueSet(0), _relationSet(0), _tokenSequence(0), 
	_outputStream(0), _includeXMLHeaderInfo(true), _is_downcased_doc(false),
	_useAbbreviatedIDs(false), MODE(mode_), _include_confidence_scores(false),
	  _in_post_xdoc_print_mode(false), _temporalNormalizer(TemporalNormalizer::build())
{
	Symbol input = ParamReader::getParam(L"input_type");
	if (input.is_null())
		_inputType = Symbol(L"text");
	else
		_inputType = input;

	_print_sentence_boundaries      = ParamReader::getOptionalTrueFalseParamWithDefaultVal("output_sentence_boundaries", false);
	_print_original_text            = ParamReader::getOptionalTrueFalseParamWithDefaultVal("output_original_text", false);
	_print_source_spans             = ParamReader::getOptionalTrueFalseParamWithDefaultVal("output_original_source_spans", true);
	_filterOverlappingMentions      = ParamReader::getOptionalTrueFalseParamWithDefaultVal("filter_out_overlapping_apf_mentions", false);
 
	std::string buffer = ParamReader::getParam("output_pronoun_words");
	if (!buffer.empty())
		_outputPronounWords = _new SymbolHash(buffer.c_str());
	else 
		_outputPronounWords = 0;

	buffer = ParamReader::getParam("output_name_words");
	if (!buffer.empty())
		_outputNameWords = _new SymbolHash(buffer.c_str());
	else 
		_outputNameWords = 0;
	
	buffer = ParamReader::getParam("output_nominal_words");
	if (!buffer.empty())
		_outputNominalWords = _new SymbolHash(buffer.c_str());
	else 
		_outputNominalWords = 0;

	_use_post_xdoc                  = ParamReader::getOptionalTrueFalseParamWithDefaultVal("post_xdoc_in_use", false);

	_dont_print_relation_timex_args = ParamReader::getOptionalTrueFalseParamWithDefaultVal("dont_print_relation_timex_args", false);
	_dont_print_events              = ParamReader::getOptionalTrueFalseParamWithDefaultVal("dont_print_events", false); 
	_dont_print_values              = ParamReader::getOptionalTrueFalseParamWithDefaultVal("dont_print_values", false); 
	_include_confidence_scores      = ParamReader::getOptionalTrueFalseParamWithDefaultVal("include_confidence_scores", false); 
}


APF4GenericResultCollector::~APF4GenericResultCollector() {	
	finalize(); 
	delete _temporalNormalizer;
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

void APF4GenericResultCollector::loadDocTheory(DocTheory* theory) { 
	_printedMentions.clear();
	finalize(); // get rid of old stuff

	_docTheory = theory;
	_entitySet = theory->getEntitySet();
	_valueSet = theory->getValueSet();
	_relationSet = theory->getRelationSet();
	_eventSet = theory->getEventSet();
	int numSents = theory->getNSentences();
	_tokenSequence = _new const TokenSequence*[numSents];
	for (int i = 0; i < numSents; i++)
		_tokenSequence[i] = theory->getSentenceTheory(i)->getTokenSequence();	
}

void APF4GenericResultCollector::produceOutput(const wchar_t *output_dir, const wchar_t* document_filename) {
	wstring output_file = wstring(output_dir) + LSERIF_PATH_SEP + wstring(document_filename) + L".apf";

	UTF8OutputStream stream;
	stream.open(output_file.c_str());
	produceOutput(stream);

	if (_use_post_xdoc) {
		wstring xdoc_file_name = wstring(document_filename) + L".xdoc";
		produceXDocOutput(output_dir, xdoc_file_name.c_str());
	}
}

void APF4GenericResultCollector::produceOutput(std::wstring *results) { 
	OStringStream stream(*results);
	produceOutput(stream);
}

void APF4GenericResultCollector::produceOutput(OutputStream &stream) {
	std::set<int> printed_entities;

	const wchar_t* docName = _docTheory->getDocument()->getName().to_string();

	_printAPFDocumentHeader(stream, docName, _docTheory->getDocument()->getSourceType().to_string());
	_printAPFEntitySet(stream, printed_entities);	
	_printAPFValueSet(stream);
	_printAPFRelationSet(stream, printed_entities);
	_printAPFEventSet(stream);

	if (_print_sentence_boundaries)
		_printSentenceBoundaries(stream, docName);
	if (_print_original_text)
		_printOriginalText(stream);

	_printAPFDocumentFooter(stream);
	stream.close();
}

void APF4GenericResultCollector::_printAPFValueSet(OutputStream &stream) {
	if (_valueSet == NULL || MODE == APF2004 || _dont_print_values) return;

	const wchar_t* doc_name = _docTheory->getDocument()->getName().to_string();

	int n_values = _valueSet->getNValues();
	for (int i = 0; i < n_values; i++) {
		Value *val = _valueSet->getValue(i);
		if (val->isTimexValue()) {
			_printAPFTimexHeader(stream, val, doc_name);
			_printAPFTimexMention(stream, val, doc_name);
			_printAPFTimexFooter(stream);
		}
		else {
			_printAPFValueHeader(stream, val, doc_name);
			_printAPFValueMention(stream, val, doc_name);
			_printAPFValueFooter(stream);
		}
	}

	_printDocumentDateTimeField(stream);
}

void APF4GenericResultCollector::_printAPFRelationSet(OutputStream& stream, std::set<int>& printed_entities) {
	const wchar_t* doc_name = _docTheory->getDocument()->getName().to_string();

	if (_relationSet != NULL) {
		int n_rels = _relationSet->getNRelations();
		_printedRelationTimeRole = _new Symbol[_valueSet->getNValues()];
		for (int i = 0; i < n_rels; i++) {
			Relation *rel = _relationSet->getRelation(i);
			//skip identity relations since the scorer doesn't know about them...
			if(rel->getType() == Symbol(L"IDENT")){
				continue;
			}
			
			if (!isValidRelationParticipant(rel->getLeftEntityID()) ||
				!isValidRelationParticipant(rel->getRightEntityID()))
				continue;

			if (printed_entities.find(rel->getLeftEntityID()) == printed_entities.end() ||
				printed_entities.find(rel->getRightEntityID()) == printed_entities.end()) 
			{
				ostringstream ostr;
				ostr << "APF4GenericResultCollector::_printAPFRelationSet(): ";
				ostr << "Relation won't be printed because the component Entities weren't printed!!!!\n";
				ostr << " Relation ID: " << rel->getID() << "\n";
				ostr << " Left Entity ID: " << rel->getLeftEntityID() << "\n";
				ostr << " Right Entity ID: " << rel->getRightEntityID() << "\n";
				ostr << "Mentions:\n";
				const Relation::LinkedRelMention *mentions = rel->getMentions();
				if (mentions == 0)
					continue;
				while (mentions != 0) {
					RelMention *ment = mentions->relMention;
					ostr << ment->getLeftMention()->getNode()->toDebugString(0) << "\n";
					ostr << ment->getRightMention()->getNode()->toDebugString(0) << "\n\n";
					mentions = mentions->next;
				}
				SessionLogger::warn("apf4") << ostr.str();
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
				SessionLogger::warn("apf4")
					<< "APF4GenericResultCollector::_printAPFRelationSet(): "
					<< "Relation won't be printed because no relation mentions printed\n"
					<< " Relation ID: " << rel->getID() << "\n";
				continue;
			}

			// filter relations that the ClutterFilter decided not to include (this is used for 2008 eval)
			// for post processing xdoc we output the relation with a special tag (in _printAPFRelationHeader()).
			if(rel->getFilterScore(ACE2008EvalClutterFilter::filterName) > 0 && !_in_post_xdoc_print_mode) {
				continue;
			}

			// check whether there are any printed RelMentions
			mentions = rel->getMentions();
			bool found_printed_rel_mention = false;
			while (mentions != 0) {
				if (_isPrintedMention(mentions->relMention->getLeftMention()) && 
					_isPrintedMention(mentions->relMention->getRightMention()))
				{
					found_printed_rel_mention = true;
					break;
				}
				mentions = mentions->next;
			}
			if (!found_printed_rel_mention) {
				ostringstream ostr;
				ostr << "APF4GenericResultCollector::_printAPFRelationSet(): ";
				ostr << "Relation won't be printed because no relation mentions printed\n";
				ostr << " Relation ID: " << rel->getID() << "\n";
				ostr << " Left Entity ID: " << rel->getLeftEntityID() << "\n";
				ostr << " Right Entity ID: " << rel->getRightEntityID() << "\n";
				ostr << "Mentions:\n";
				const Relation::LinkedRelMention *mentions = rel->getMentions();
				if (mentions == 0)
					continue;
				while (mentions != 0) {
					RelMention *ment = mentions->relMention;
					ostr << ment->getLeftMention()->getNode()->toDebugString(0) << "\n";
					ostr << ment->getRightMention()->getNode()->toDebugString(0) << "\n\n";
					mentions = mentions->next;
				}
				SessionLogger::warn("apf4") << ostr.str();
				continue;
			} else { // print the relation
				_printAPFRelationHeader(stream, rel, doc_name);
				while (mentions != 0) {
					_printAPFRelMention(stream, mentions->relMention, rel, doc_name);
					mentions = mentions->next;
				}
				_printAPFRelationFooter(stream);
			}
		}
		delete [] _printedRelationTimeRole;
	}
}

void APF4GenericResultCollector::_printAPFEventSet(OutputStream& stream) {

	if (_eventSet == NULL || _dont_print_events) return;

	const wchar_t* doc_name = _docTheory->getDocument()->getName().to_string();

	if (_entitySet == 0) {
		SessionLogger::warn("apf4") << 
			"APF4GenericResultCollector::_printAPFEventSet(): Warning: collecting events with a nonexistent entity set\n";
		
		_printedEventEntityRole = _new Symbol[0];
	}
	else {
		_printedEventEntityRole = _new Symbol[_entitySet->getNEntities()];
	}
	if (_valueSet == 0) {
		SessionLogger::warn("apf4") << 
			"APF4GenericResultCollector::_printAPFEventSet(): Warning: collecting events with a nonexistent value set\n";

		_printedEventValueRole = _new Symbol[0];
	}
	else {
		_printedEventValueRole = _new Symbol[_valueSet->getNValues()];
	}
	int n_events = _eventSet->getNEvents();
	for (int i = 0; i < n_events; i++) {
		Event *event = _eventSet->getEvent(i);

		Event::LinkedEventMention *mentions = event->getEventMentions();
		if (mentions == 0)
			continue;

		_printAPFEventHeader(stream, event, doc_name);
		while (mentions != 0) {
			_printAPFEventMention(stream, mentions->eventMention, event, doc_name);
			mentions = mentions->next;
		}
		_printAPFEventFooter(stream);
	}
	delete [] _printedEventEntityRole;
	delete [] _printedEventValueRole;
}

void APF4GenericResultCollector::_printDocumentDateTimeField(OutputStream& stream) {
	const LocatedString *dtString = _docTheory->getDocument()->getDateTimeField();
	const wchar_t* doc_name = _docTheory->getDocument()->getName().to_string();

	if (dtString == 0) return;

	// we are faking the UID of the value mention as well as of the value!
	// this needs to be updated if the way we print out TIMEX values ever changes
	stream << L"    <timex2 ID=\"";
	if (!_useAbbreviatedIDs)
		stream << doc_name << L"-";
	stream << L"T" << MAX_DOCUMENT_VALUES << L"\"";
//#ifdef ENGLISH_LANGUAGE
	if (SerifVersion::isEnglish()) {
		stream << " VAL=\"";
		stream << _docTheory->getDocument()->normalizeDocumentDate() << "\"";
	}
//#endif
	stream <<">\n";
	stream << L"        <timex2_mention ID=\"";
	if(!_useAbbreviatedIDs)
		stream << doc_name << L"-" << L"T" << MAX_DOCUMENT_VALUES << L"-M";
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

const wchar_t* APF4GenericResultCollector::_convertMentionTypeToAPF(Mention* ment)
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
			return L"PRO";
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

void APF4GenericResultCollector::_printAPFDocumentHeader(OutputStream& stream,
												  const wchar_t* doc_name,
												  const wchar_t* doc_source)
{
	if(_includeXMLHeaderInfo) {
		stream << L"<?xml version=\"1.0\"?>\n";
		if (MODE == APF2004)
			stream << L"<!DOCTYPE source_file SYSTEM \"apf.v4.0.1.dtd\">\n";
		else if (MODE == APF2005)
			stream << L"<!DOCTYPE source_file SYSTEM \"apf.v5.1.1.dtd\">\n";	
		else if (MODE == APF2007)
			stream << L"<!DOCTYPE source_file SYSTEM \"apf.v5.1.2.dtd\">\n";
		else if (MODE == APF2008)
			stream << L"<!DOCTYPE source_file SYSTEM \"apf.v5.2.0.dtd\">\n";
	}
	stream << L"<source_file SOURCE=\"" << (doc_source?doc_source:L"UNKNOWN") 
		<< L"\" TYPE=\"text\" VERSION=\"2.0\" URI=\"" << doc_name
		<< L"\">\n";
	stream << L"  <document DOCID=\"" << doc_name << L"\">\n\n";
}

void APF4GenericResultCollector::_printAPFEntityHeader(OutputStream& stream, 
												       Entity* ent, 
												       const wchar_t* doc_name)
{
	stream << L"    <entity ID=\"";
	_printEntityID(stream, doc_name, ent);
	stream << L"\"";
	_printEntityType(stream, ent);
	_printEntitySubtype(stream, ent);
	_printEntityClass(stream, ent);
	_printEntityGUID(stream, ent);
	_printEntityCanonicalName(stream, ent);
	stream << L">\n";

}

void APF4GenericResultCollector::_printAPFAttributeHeader(OutputStream& stream) 
{
	stream << L"        <entity_attributes>\n";
}

void APF4GenericResultCollector::_printAPFAttributeFooter(OutputStream& stream) 
{
	stream << L"        </entity_attributes>\n";
}


// NOTE: in the rare case that we want to print a list as a mention,
//       print each of its parts instead.
void APF4GenericResultCollector::_printAPFListMention(OutputStream& stream,
										   Mention* ment,
										   Entity* ent, 
										   const wchar_t* doc_name,
										   std::set<MentionUID>& printed_mentions_for_this_entity)
{
	Mention* child = ment->getChild();
	while (child != 0) {
		if (child->getMentionType() != Mention::APPO) {
			_printAPFMention(stream, child, ent, doc_name, printed_mentions_for_this_entity);
		}
		child = child->getNext();
	}
}

void APF4GenericResultCollector::_printAPFNameAttribute(OutputStream& stream,
												 Mention* ment)
{
	const SynNode *node = ment->getEDTHead();
	int startTok = node->getStartToken();
	int endTok = node->getEndToken();
	int sentNum = ment->getSentenceNumber();
	stream << L"          <name NAME=\"";
	_printAPFSpanText(stream, startTok, endTok, sentNum);
	stream << L"\">\n";
	_printAPFMentionExtent(stream, startTok, endTok, sentNum);
	stream << L"          </name>\n";
}

void APF4GenericResultCollector::_printAPFMentionHeader(OutputStream& stream,
												 Mention* ment,
												 Entity* ent,
												 const wchar_t* doc_name)
{
	stream << L"        <entity_mention";
	_printEntityMentionType(stream, ment);
	stream << L" ID=\"";
	_printEntityMentionID(stream, doc_name, ment, ent);
	stream << L"\"";
	_printEntityMentionRole(stream, ment);
	_printEntityMentionMetonymy(stream, ment);
	stream << L">\n";
}

void APF4GenericResultCollector::_printAPFMentionExtent(OutputStream& stream,
												 const SynNode* node,
												 int sentNum)
{
	int startTok = node->getStartToken();
	int endTok = node->getEndToken();
	
	_printAPFMentionExtent(stream, startTok, endTok, sentNum);
}

void APF4GenericResultCollector::_printAPFMentionExtent(OutputStream& stream,
												 int startTok,
												 int endTok,
												 int sentNum)
{
	if (_inputType == Symbol(L"asr")) {
		float start = _tokenSequence[sentNum]->getToken(startTok)->getStartASRTime().value();
		float end = _tokenSequence[sentNum]->getToken(endTok)->getEndASRTime().value();
		wchar_t sbuf[10];
		wchar_t ebuf[10];
		swprintf(sbuf, 10, L"%.2f", start);
		swprintf(ebuf, 10, L"%.2f", end);
		stream << L"              <timespan START=\"" << sbuf << "\" END=\"" << ebuf << "\">";
		_printAPFSpanText(stream, startTok, endTok, sentNum);
		stream << L"</timespan>\n";

	} else {
		int start_edt = _tokenSequence[sentNum]->getToken(startTok)->getStartEDTOffset().value();
		int end_edt = _tokenSequence[sentNum]->getToken(endTok)->getEndEDTOffset().value(); 

		stream << L"              <charseq START=\"" << start_edt << "\" END=\"" << end_edt << "\">";
		_printAPFSpanText(stream, startTok, endTok, sentNum);
		stream << L"</charseq>\n";
	}
}

void APF4GenericResultCollector::_printAPFSpanText(OutputStream& stream,
												   int startTok,
												   int endTok,
												   int sentNum)
{
	if (_print_source_spans) {
		EDTOffset start_edt = _tokenSequence[sentNum]->getToken(startTok)->getStartEDTOffset();
		EDTOffset end_edt = _tokenSequence[sentNum]->getToken(endTok)->getEndEDTOffset(); 

		_printAPFSourceSpanText(stream, start_edt, end_edt, sentNum);
	}
	else {
		_printAPFTokenSpanText(stream, startTok, endTok, sentNum);
	}
}


void APF4GenericResultCollector::_printAPFTokenSpanText(OutputStream& stream, 
												 int startTok,
												 int endTok,
											     int sentNum) 
{	
	for (int i = startTok; i <= endTok; i++) {
		const wchar_t* symStr = _tokenSequence[sentNum]->getToken(i)->getSymbol().to_string();
		_printSafeString(stream, symStr);		
		if (i != endTok)
			stream << L" ";
	}
}

void APF4GenericResultCollector::_printAPFSourceSpanText(OutputStream& stream,
														 EDTOffset start_edt,
														 EDTOffset end_edt,
														 int sentNum)
{
	const LocatedString *origStr = _docTheory->getDocument()->getOriginalText();

	int start = origStr->positionOfStartOffset(start_edt);
	int end = origStr->positionOfEndOffset(end_edt);
	
	if (start == -1 || end == -1) {
		char message[1024];
		_snprintf(message, 1024, "APF4ResultCollector could not find source offset when _print_source_spans is true.\nsentNum: %d of %d start: %d end: %d\n", 
			sentNum, _docTheory->getNSentences(), start_edt.value(), end_edt.value());
		throw InternalInconsistencyException("APF4ResultCollector::_printAPFSourceSpanText()", message);	
	}		
	LocatedString *substr = origStr->substring(start, end+1);
	const wchar_t* span_str = substr->toString();
	_printSafeString(stream, span_str); 
	delete substr;
	
}

void APF4GenericResultCollector::_printEntityType(OutputStream& stream, Entity* ent) {
	stream << L" TYPE=\"" << ent->type.getName().to_string() <<  L"\"";
}

void APF4GenericResultCollector::_printEntityGUID(OutputStream& stream, Entity* ent) {

	if (ent->getGUID() == -1) return;

	stream << L" CDOCID=\"";
	stream << ent->getGUID();
	stream << L"\"";
}

void APF4GenericResultCollector::_printEntityCanonicalName(OutputStream& stream, Entity* ent) {
	Symbol can_name_symbols[500];
	int n_symbols = -1;
	ent->getCanonicalName(n_symbols, can_name_symbols);
	if (n_symbols > 0) {
		stream << L" CANONICAL_NAME=\"";
		for (int i = 0; i < n_symbols; i++) {
			if (i > 0)
				stream << L" ";
			stream << can_name_symbols[i].to_string();
		}
		stream << "\"";
	}
}

void APF4GenericResultCollector::_printEntityMentionType(OutputStream& stream, Mention *ment) {
	stream << L" TYPE=\"" << _convertMentionTypeToAPF(ment) << L"\"";
}


void APF4GenericResultCollector::_printEntityMentionMetonymy(OutputStream& stream, Mention *ment) {
	if (ment->isMetonymyMention())
		stream << L" METONYMY_MENTION=\"TRUE\"";
}

void APF4GenericResultCollector::_printSafeString(OutputStream& stream,
										   const wchar_t* origStr)
{
	size_t orig_len = wcslen(origStr);
	// worst case, it's all quotes or apostrophes
	wchar_t* safeStr = _new wchar_t[6*orig_len + 1];
	size_t oldIdx = 0;
	size_t newIdx = 0;
	// copy, replacing & with &amp; where appropriate
	for (; oldIdx < orig_len; oldIdx++) {
		wchar_t wch = origStr[oldIdx];
		// Replace harmful unicode with spaces. The below tests were copied from 
		//     int Tokenizer::removeHarmfulUnicodes(LocatedString *string)
		if ((wch < 0x0009) ||   // 0x09 is tab, 
			// 0x0a is linefeed
			// 0x0b is vertical tab, 0x0c is form feed
			(wch == 0x000b)  || (wch == 0x000c) ||  
			// 0x0d is carriage return, others are C0 ctls
			((wch > 0x000d)  && (wch < 0x0020)) || // 0x20 is blank
			// 0x7f is delete, 0x85 is unicode newline
			((wch >= 0x007f) && (wch < 0x0085)) || 
			// 0x80 -> 0x9f are Unicode controls C1
			((wch >= 0x0086) && (wch <= 0x009f)) ||
			(wch == 0x00ad) || //0xad is "sly/shy hyphen"
			(wch == 0x034f) || //0x34f is combining grapheme joiner
			(wch == 0x0640) || //Arabic "tatweel" token elongation char
		// change these to spaces for token separation
			//((wch >= 0x200c) && (wch <= 0x200f)) || // typography (include ltr and rtl)
			//((wch >= 0x202a) && (wch <= 0x202e)) || // embedding direction to print
			((wch >= 0x206a) && (wch <= 0x206f)) || // Arabic typography
			((wch >= 0x2ff0) && (wch <= 0x2ffb)) || // ideographic char desc
			(wch == 0x303e) ||  // ideographic variation
			((wch >= 0xfdd0) && (wch <= 0xfdef)) || // non-codes
			((wch >= 0xfe00) && (wch <= 0xfe0f)) || // variation selectors
			(wch == 0xfeff) ||  // ByteOrdeMark or "endian indicator" added to front 
			((wch >= 0xfff9) && (wch <= 0xfffc)) || // non-codes
			(wch == 0xffff) ){  // "eof" tag accidentally appended 				
				safeStr[newIdx++] = L' ';
		} else if (origStr[oldIdx] == L'&') {
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
		} else if (origStr[oldIdx] == L'"') {
			safeStr[newIdx++] = L'&';
			safeStr[newIdx++] = L'q';
			safeStr[newIdx++] = L'u';
			safeStr[newIdx++] = L'o';
			safeStr[newIdx++] = L't';
			safeStr[newIdx++] = L';';
		} else if (origStr[oldIdx] == L'\'') {
			safeStr[newIdx++] = L'&';
			safeStr[newIdx++] = L'a';
			safeStr[newIdx++] = L'p';
			safeStr[newIdx++] = L'o';
			safeStr[newIdx++] = L's';
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

void APF4GenericResultCollector::_printAPFValueHeader(OutputStream& stream, 
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

void APF4GenericResultCollector::_printAPFTimexHeader(OutputStream& stream, 
											   Value* val,  
											   const wchar_t* doc_name)
{
	//<timex2 ID="CNN_CF_20030304.1900.01-T1">
	stream << L"    <timex2 ID=\"";
	_printTimexID(stream, doc_name, val);
	stream << L"\"";
	if (!val->getTimexVal().is_null()) 
		stream << L" VAL=\"" << val->getTimexVal().to_string() << L"\"";
	if (!val->getTimexMod().is_null())
		stream << L" MOD=\"" << val->getTimexMod().to_string() << L"\"";
	if (!val->getTimexSet().is_null())
		stream << L" SET=\"" << val->getTimexSet().to_string() << L"\"";
	if (!val->getTimexAnchorVal().is_null())
		stream << L" ANCHOR_VAL=\"" << val->getTimexAnchorVal().to_string() << L"\"";
	if (!val->getTimexAnchorDir().is_null())
		stream << L" ANCHOR_DIR=\"" << val->getTimexAnchorDir().to_string() << L"\"";
	if (!val->getTimexNonSpecific().is_null())
		stream << L" NON_SPECIFIC=\"" << val->getTimexNonSpecific().to_string() << L"\"";
	stream << ">\n";
}

void APF4GenericResultCollector::_printAPFValueMention(OutputStream& stream,
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

void APF4GenericResultCollector::_printAPFTimexMention(OutputStream& stream,
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

void APF4GenericResultCollector::_printAPFRelationHeader(OutputStream& stream, 
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
	stream << L" TYPE=\"" << RelationConstants::getBaseTypeSymbol(type).to_string();
	if (RelationConstants::getSubtypeSymbol(type) != RelationConstants::NONE)
		stream << L"\" SUBTYPE=\"" << RelationConstants::getSubtypeSymbol(type).to_string();
	if(rel->getFilterScore(ACE2008EvalClutterFilter::filterName) > 0)
		stream << L"\" FILTERED=\"TRUE";

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
	} else { //if (MODE == APF2005 || MODE == APF2007 ||  MODE == APF2008) {
		arg_name = L"relation_argument";
		id_name = L"REFID";
		role_prefix = L"ROLE=\"Arg-";
	}

	// 2004: <rel_entity_arg ENTITYID="APW20001002.0615.0146-E12" ARGNUM="1" />
  	// 2005: <relation_argument REFID="SAMPLE-E52" ROLE="Arg-1"/>
	// 2008: We exclude time argument from regular output
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
		if (!_dont_print_relation_timex_args) {
			if (mentions->relMention->getTimeArgument() != 0) {
				const Value *value = mentions->relMention->getTimeArgument()->getDocValue();
				Symbol role = mentions->relMention->getTimeRole();
				if (value == 0) {
					SessionLogger::warn("apf4")
						<< "APF4GenericResultCollector::_printAPFRelationHeader():"
						<< "No value for value mention "
						<< mentions->relMention->getTimeArgument()->getUID().toInt() << "\n";
				}
				else if (_printedRelationTimeRole[value->getID()].is_null()) {
					_printedRelationTimeRole[value->getID()] = role;
					stream << L"      <" << arg_name << L" " << id_name << L"=\"";
					_printTimexID(stream, doc_name, value);
					stream << L"\" ROLE=\"" << role.to_string() << L"\"/>\n";
				}
			}
		}
		mentions = mentions->next;
	}

}

void APF4GenericResultCollector::_printAPFRelMention(OutputStream& stream,
												RelMention* ment,
												Relation* rel, 
												const wchar_t* doc_name)
{
	bool left_printed = _isPrintedMention(ment->getLeftMention());
	bool right_printed = _isPrintedMention(ment->getRightMention());
	if (!left_printed || !right_printed)
	{
		ostringstream ostr;
		ostr << "APF4GenericResultCollector::_printAPFRelMention(): Relation won't be printed because the component Mentions weren't printed!!!!\n";
		ostr << "Relation ID: " << rel->getID() << "\n";
		
		if (!left_printed) {
			ostr << "Left mention not printed ID: " << ment->getLeftMention()->getUID() << "\n";
			ostr << ment->getLeftMention()->node->toFlatString().data() << "\n";
		}
		if (!right_printed) {
			ostr << "Right mention not printed ID: " << ment->getRightMention()->getUID() << "\n";
			ostr << ment->getRightMention()->node->toFlatString().data() << "\n";
		}
		SessionLogger::warn("apf4") << ostr.str();
		return;
	}

	std::wstring arg_name;
	std::wstring id_name;
	std::wstring role_prefix;

	if (MODE == APF2004) {
		arg_name = L"rel_mention_arg";
		id_name = L"ENTITYMENTIONID";
		role_prefix = L"ARGNUM=\"";
	} else { //if (MODE == APF2005 || MODE == APF2007 || MODE == APF2008) {
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

	if (_include_confidence_scores) {
		stream << L" CONFIDENCE=\"" << ment->getScore() << L"\"";
	}

	stream << ">\n";

	// 2005 -- FAKED!! Currently this just prints the extent of the left mention...
	if (MODE != APF2004) {
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
			SessionLogger::warn("apf4")
				<< "APF4GenericResultCollector::_printAPFRelMention():"
				<< "No value for value mention "
				<< ment->getTimeArgument()->getUID().toInt() << "\n";
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

void APF4GenericResultCollector::_printAPFEventHeader(OutputStream& stream, 
												  Event* event,  
												  const wchar_t* doc_name)
{
	std::wstring str = event->getType().to_string();
	size_t index = str.find(L".");
	Symbol type = (index == wstring::npos) ? str.c_str() : Symbol(str.substr(0, index).c_str());
	Symbol subtype = (index == wstring::npos) ? L"" : Symbol(str.substr(index + 1).c_str());

	// <event ID="SAMPLE-EV1" TYPE="Justice" SUBTYPE="Arrest-Jail" MODALITY="Asserted" 
	//		POLARITY="Positive" GENERICITY="Specific" TENSE="Past">
	stream << L"    <event ID=\"";
	_printEventID(stream, doc_name, event);
	stream << L"\"";
	stream << L" TYPE=\"" << type.to_string();
	stream << L"\" SUBTYPE=\"" << subtype.to_string();
	stream << L"\" MODALITY=\"" << event->getModality().toString();
	stream << L"\" POLARITY=\"" << event->getPolarity().toString();
	stream << L"\" GENERICITY=\"" << event->getGenericity().toString();
	stream << L"\" TENSE=\"" << event->getTense().toString();
	stream << L"\">\n";

	Event::EventArguments *eventArgs = _new Event::EventArguments();
	event->getArgumentEntities(eventArgs, _entitySet, _valueSet, _printedMentions);
	
	for (size_t i=0; i < eventArgs->entities.size(); i++) {
		int ent_id = eventArgs->entities[i]->getID();
		Symbol role = eventArgs->entity_roles[i];
		// <event_argument REFID="SAMPLE-E5" ROLE="Person"/>
		stream << L"      <event_argument REFID=\"";
		_printEntityID(stream, doc_name, ent_id);
		stream << L"\" ROLE=\"";
		stream << role.to_string() << L"\"/>\n";
		_printedEventEntityRole[ent_id] = role;
	}
	for (size_t i=0; i < eventArgs->values.size(); i++) {
		const Value* value = eventArgs->values[i];
		Symbol role = eventArgs->value_roles[i];
		// <event_argument REFID="SAMPLE-V5" ROLE="Time-Within"/>
		stream << L"      <event_argument REFID=\"";
		if (value->isTimexValue())
			_printTimexID(stream, doc_name, value);
		else _printValueID(stream, doc_name, value);
		stream << L"\" ROLE=\"" << role.to_string() << L"\"/>\n";
		_printedEventValueRole[value->getID()] = role;
	}
	delete eventArgs;
}

void APF4GenericResultCollector::_printAPFEventMention(OutputStream& stream,
												  EventMention* em,
												  Event* event, 
												  const wchar_t* doc_name)
{
	// <event_mention ID="SAMPLE-EV1-1">
	stream << L"      <event_mention ID=\""; 
	_printEventMentionID(stream, doc_name, em, event);
	stream << L"\">\n";

    //  <extent>
    //    <charseq START="704" END="706">one</charseq>
    //  </extent>

	// this should eventually be replaced with real extent!
	stream << L"          <extent>\n";
	_printAPFMentionExtent(stream, em->getAnchorNode(), em->getSentenceNumber());
	stream << L"          </extent>\n";
	
	// <anchor>
    //  <charseq START="671" END="678">arrested</charseq>
    // </anchor>
	stream << L"          <anchor>\n";
	_printAPFMentionExtent(stream, em->getAnchorNode(), em->getSentenceNumber());
	stream << L"          </anchor>\n";

	// <event_mention_argument REFID="SAMPLE-E5-7" ROLE="Person">
    //  <extent>
    //    <charseq START="704" END="706">one</charseq>
    //  </extent>
    // </event_mention_argument>
	for (int arg = 0; arg < em->getNArgs(); arg++) {
		Symbol role = em->getNthArgRole(arg).to_string();
		const Mention *argMention = em->getNthArgMention(arg);
		if (!_isPrintedMention(argMention))
			continue;
		Entity *entity = _entitySet->getEntityByMentionWithoutType(argMention->getUID());
		if (entity == 0 && argMention->getMentionType() == Mention::NONE) {
			const Mention *m = argMention;				
			while (m != 0 && m->getMentionType() == Mention::NONE) {
				m = m->getParent();
			}
			entity = _entitySet->getEntityByMentionWithoutType(m->getUID());
		}
		// If a LIST is part of an entity, the members of that LIST will get written out as part of the entity
		if (entity == 0 && argMention->getParent() && argMention->getParent()->getMentionType() == Mention::LIST) {
			entity = _entitySet->getEntityByMentionWithoutType(argMention->getParent()->getUID());
		}
		if (entity == 0) {
			SessionLogger::warn("apf4")
				<< "APF4GenericResultCollector::_printAPFEventMention():"
				<< "No entity (or incorrectly-typed entity) for mention\n"
				<< argMention->getUID() << "\n";
			throw InternalInconsistencyException(
				"APF4GenericResultCollector::_printAPFEventMention()",
				"current mention has no entity!");
		}
		if (entity->isGeneric())
			continue;
		int ent_id = entity->getID();
		// we will only print entities with one role per event
		if (_printedEventEntityRole[ent_id] != role) {
			continue;
		}
		
		stream << L"        <event_mention_argument REFID=\"";
		_printEntityMentionID(stream, doc_name, argMention, entity);
		stream << L"\" ROLE=\"" << role.to_string() << L"\">\n";
		stream << L"          <extent>\n";
		_printAPFMentionExtent(stream, argMention->getNode(), argMention->getSentenceNumber());
		stream << L"          </extent>\n";
		stream << L"        </event_mention_argument>\n";
	}

	// <event_mention_argument REFID="SAMPLE-V5-7" ROLE="Time-Within">
    // </event_mention_argument>
	for (int value_arg = 0; value_arg < em->getNValueArgs(); value_arg++) {
		Symbol role = em->getNthArgValueRole(value_arg).to_string();
		const ValueMention *argValueMention = em->getNthArgValueMention(value_arg);
		Value *value= _valueSet->getValueByValueMention(argValueMention->getUID());
		if (value == 0) {
			SessionLogger::warn("apf4")
				<< "APF4GenericResultCollector::_printAPFEventHeader():"
				<< "No value for value mention\n"
				<< argValueMention->getUID().toInt() << "\n";
			continue;
		}
		
		// we will only print entities with one role per event
		if (_printedEventValueRole[value->getID()] != role) {
			continue;
		}

		stream << L"          <event_mention_argument REFID=\"";
		if (value->isTimexValue())
			_printTimexMentionID(stream, doc_name, value);
		else _printValueMentionID(stream, doc_name, value);
		stream << L"\" ROLE=\"" << role.to_string() << L"\">\n";
		stream << L"            <extent>\n";
		_printAPFMentionExtent(stream, argValueMention->getStartToken(), 
			argValueMention->getEndToken(), argValueMention->getSentenceNumber());
		stream << L"            </extent>\n";
		stream << L"          </event_mention_argument>\n";
	}

	stream << L"        </event_mention>\n"; 

}


void APF4GenericResultCollector::_printAPFEntityFooter(OutputStream& stream)
{
	stream << L"    </entity>\n\n";
}

void APF4GenericResultCollector::_printAPFValueFooter(OutputStream& stream)
{
	stream << L"    </value>\n\n";
}

void APF4GenericResultCollector::_printAPFTimexFooter(OutputStream& stream)
{
	stream << L"    </timex2>\n\n";
}

void APF4GenericResultCollector::_printAPFRelationFooter(OutputStream& stream)
{
	stream << L"    </relation>\n\n";
}

void APF4GenericResultCollector::_printAPFEventFooter(OutputStream& stream)
{
	stream << L"    </event>\n\n";
}

void APF4GenericResultCollector::_printAPFDocumentFooter(OutputStream& stream)
{
	stream << L"  </document>\n";
	stream << L"</source_file>\n";
}

bool APF4GenericResultCollector::_isTopMention(const Mention* ment, EntityType type)
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
		err.append(ment->getEntityType().getName().to_debug_string());
		err.append(" - ");
		err.append(Mention::getTypeString(ment->mentionType));
		err.append(")\n");
		throw InternalInconsistencyException("APF4GenericResultCollector::_isTopMention()",
			(char*)err.c_str());

	}
	if (parentEnt != 0 && parentEnt->getID() == ent->getID())
		return false;

	return true;
}

bool APF4GenericResultCollector::_isItemOfUnprintedList(const Mention* ment)
{
	if (ment->getParent() == 0)
		return false;

	// only applies to DESCs and NAMEs
	if (ment->getMentionType() != Mention::DESC && ment->getMentionType() != Mention::NAME)
		return false;

	if (ment->getParent()->getMentionType() == Mention::LIST) {
		Entity* parentEnt = _entitySet->getEntityByMentionWithoutType(ment->getParent()->getUID());
		if (parentEnt != 0 && _isPrintableMention(ment->getParent(), parentEnt))
			return false;
		return true;
	}
	return false;
}

bool APF4GenericResultCollector::_isPartOfAppositive(const Mention* ment)
{
	return ment->isPartOfAppositive();
}

// TODO: need role information before this is useful
bool APF4GenericResultCollector::_isGPEModifierOfPersonGPE(const Mention* ment)
{
	return false;
}

// mention must be a name, must have a parent whose edt head is different
// and must have a parent whose entity is different
bool APF4GenericResultCollector::_isNameNotInHeadOfParent(const Mention* ment, EntityType type)
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
				throw InternalInconsistencyException("APF4GenericResultCollector::_isNameNotInHeadOfParent()",
				"current mention has no entity!");
			if (parEnt->getID() == ent->getID())
				return false;
		}
	}
	return true;
}

bool APF4GenericResultCollector::_sharesHeadWithAlreadyPrintedMention(const Mention* ment)
{
	int sentNum = ment->getSentenceNumber();
	const MentionSet *mentionSet = _entitySet->getMentionSet(sentNum);
	const SynNode *head = ment->getEDTHead();
	
	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		const Mention *other = mentionSet->getMention(i);
		const SynNode *otherHead = other->getEDTHead();
		if ((other != ment) && _isPrintedMention(other) && 
			_doesOverlap(head, otherHead, ment->getSentenceNumber(), other->getSentenceNumber()))
		{
			return true;
		}
	}
	return false;
}


bool APF4GenericResultCollector::_sharesHeadWithEarlierMentionOfSameEntity(const Mention* ment,
																	Entity *ent)
{
	const SynNode *head = ment->getEDTHead();

	GrowableArray<MentionUID> &mens = ent->mentions;
	for	(int j = 0;	j <	mens.length(); j++)	{
		const Mention *other = _entitySet->getMention(mens[j]);
		const SynNode *otherHead = other->getEDTHead();

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

bool APF4GenericResultCollector::_doesOverlap(const SynNode *node1, const SynNode *node2, int sent_num1, int sent_num2)
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

bool APF4GenericResultCollector::_isPrintableMention(const Mention *ment, Entity *ent) {
	if(MODE != APF2004){
		
		//Note: this is a fairly english specific hack to avoid printing 
		//mentions in bn/cts that are not in the non-speaker turn regions of the document.
		//If it causes problems, we could do language specific utilities for the APFCollector

		//Simple rule for non-CA Serif would be, if your most immediate covering span is turn, 
		//otherwise don't print, but in CASerif, enamex spans are added, so these need to be ignored
		/*if((_docTheory->getSourceType() == Symbol(L"broadcast conversation"))  ||
			_docTheory->getSourceType() == Symbol(L"telephone")){
				int sentnum = ment->getSentenceNumber();
				int sent_offset = _docTheory->getSentence(sentnum)->getStartEDTOffset();
				Metadata* metadata = _docTheory->getMetadata();
				if(metadata != 0){
					Span* turnspan = metadata->getCoveringSpan(sent_offset, Symbol(L"TURN"));
					Span* speakerspan = metadata->getCoveringSpan(sent_offset, Symbol(L"SPEAKER"));
					//Warn Since this implies Annotation errors
					SessionLogger::err("") << "CAResultCollectorAPF4::_isPrintableMention(): "
						<< "Mention won't be printed because it is in invalid region\n"
						<< "  Mention ID: " << ment->getUID()<<" "<<ment->getNode()->toDebugString(0) << "\n";
					if((turnspan == 0) || (speakerspan != 0)){
						return false;
					}
				}
			}*/
	}
	if (_filterOverlappingMentions &&
		(_sharesHeadWithAlreadyPrintedMention(ment) ||
		_sharesHeadWithEarlierMentionOfSameEntity(ment, ent)))
	{
		return false;
	}

	return ((_isPartOfAppositive(ment) && ment->getMentionType() != Mention::LIST) || 
			 _isGPEModifierOfPersonGPE(ment) ||
  		    (_isTopMention(ment, ent->getType()) && ment->getMentionType() != Mention::APPO) ||
			 _isNameNotInHeadOfParent(ment, ent->getType()) ||
		     _isItemOfUnprintedList(ment));
}

bool APF4GenericResultCollector::_isPrintedMention(const Mention *ment) {
	if (_printedMentions.find(ment->getUID()) != _printedMentions.end())
		return true;
	else
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

void APF4GenericResultCollector::_printEntityID(OutputStream& stream, const wchar_t* doc_name, Entity *ent) {
	_printEntityID(stream, doc_name, ent->ID);
}
void APF4GenericResultCollector::_printEntityID(OutputStream& stream, const wchar_t* doc_name, int id) {
	if (!_useAbbreviatedIDs)
		stream << doc_name << L"-";
	stream << L"E" << id;
}
void APF4GenericResultCollector::_printEntityMentionID(OutputStream& stream, const wchar_t* doc_name, 
												const Mention *ment, Entity *ent) 
{
	_printEntityMentionID(stream, doc_name, ment->getUID(), ent->ID);
}
void APF4GenericResultCollector::_printEntityMentionID(OutputStream& stream, const wchar_t* doc_name, 
												MentionUID mid, int eid) 
{
	if (!_useAbbreviatedIDs) {
		_printEntityID(stream, doc_name, eid);
		stream << L"-M";
	}
	stream << mid;
}
void APF4GenericResultCollector::_printValueID(OutputStream& stream, 
										  const wchar_t* doc_name, 
										  const Value *val) 
{
	if (!_useAbbreviatedIDs)
		stream << doc_name << L"-";
	stream << L"V" << val->getID();
}
void APF4GenericResultCollector::_printValueMentionID(OutputStream& stream, 
											   const wchar_t* doc_name, 
											   const Value *val) 
{
	if(!_useAbbreviatedIDs) {
		_printValueID(stream, doc_name, val);
		stream << L"-M";
	}
	stream << L"1";
}
void APF4GenericResultCollector::_printTimexID(OutputStream& stream, 
										  const wchar_t* doc_name, 
										  const Value *val) 
{
	if (!_useAbbreviatedIDs)
		stream << doc_name << L"-";
	stream << L"T" << val->getID();
}
void APF4GenericResultCollector::_printTimexMentionID(OutputStream& stream, 
											     const wchar_t* doc_name, 
											     const Value *val) 
{
	if(!_useAbbreviatedIDs) {
		_printTimexID(stream, doc_name, val);
		stream << L"-M";
	}
	stream << L"1";
}
void APF4GenericResultCollector::_printRelationID(OutputStream& stream, const wchar_t* doc_name, Relation *rel) {
	if (!_useAbbreviatedIDs)
		stream << doc_name << L"-";
	stream << L"R" << rel->getID();
}
void APF4GenericResultCollector::_printRelationMentionID(OutputStream& stream, 
												  const wchar_t* doc_name, 
												  RelMention *rment, Relation *rel) 
{
	if(!_useAbbreviatedIDs) {
		_printRelationID(stream, doc_name, rel);
		stream << L"-M";
	}
	stream << rment->getUID().toInt();
}
void APF4GenericResultCollector::_printEventID(OutputStream& stream, const wchar_t* doc_name, Event *event) {
	if (!_useAbbreviatedIDs)
		stream << doc_name << L"-";
	stream << L"EV" << event->getID();
}
void APF4GenericResultCollector::_printEventMentionID(OutputStream& stream, const wchar_t* doc_name, 
											   EventMention *evment, Event *event) 
{
	if(!_useAbbreviatedIDs) {
		_printEventID(stream, doc_name, event);
		stream << L"-M";
	}
	stream << evment->getUID().toInt();
}


void APF4GenericResultCollector::_printSentenceBoundaries(OutputStream &stream, const wchar_t *doc_name)
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

void APF4GenericResultCollector::_printOriginalText(OutputStream &stream)
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
	int pos = textCopy.length() - 1;
	while (pos >= 0 && textCopy.charAt(pos) == L'-') {
		textCopy.replace(pos, 1, L" ");
		pos--;
	}

	stream << textCopy.toString();

	stream << "-->\n    </source>\n";
}

void APF4GenericResultCollector::produceXDocOutput(const wchar_t *output_dir,
										const wchar_t *doc_filename)
{
	// temporarily change parameters to XDoc parameters
	bool tmp_print_sentence_boundaries = _print_sentence_boundaries;
	bool tmp_print_original_text = _print_original_text;
	bool tmp_print_source_spans = _print_source_spans;
	bool tmp_dont_print_relation_timex_args = _dont_print_relation_timex_args;
	bool tmp_dont_print_events = _dont_print_events;
	bool tmp_dont_print_values = _dont_print_values;

	_in_post_xdoc_print_mode = true;
	_print_sentence_boundaries = true;
	_print_original_text = true;
	_print_source_spans = false;
	_dont_print_relation_timex_args = false;
	_dont_print_events = false;
	_dont_print_values = false;

	// call produceOutput with XDoc parameter settings
	wstring output_file = wstring(output_dir) + LSERIF_PATH_SEP + wstring(doc_filename) + L".apf";;

	UTF8OutputStream stream;
	stream.open(output_file.c_str());
	produceOutput(stream);

	// return the parameters to their original values;
	_in_post_xdoc_print_mode = false;
	_print_sentence_boundaries = tmp_print_sentence_boundaries;
	_print_original_text = tmp_print_original_text;
	_print_source_spans = tmp_print_source_spans;
	_dont_print_relation_timex_args = tmp_dont_print_relation_timex_args;
	_dont_print_events = tmp_dont_print_events;
	_dont_print_values = tmp_dont_print_values;
}
