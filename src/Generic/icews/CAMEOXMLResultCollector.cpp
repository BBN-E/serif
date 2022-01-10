// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/OStringStream.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/XMLUtil.h"

#include "Generic/icews/ICEWSEventMention.h"
#include "Generic/icews/ICEWSEventMentionSet.h"
#include "Generic/icews/ICEWSActorInfo.h"
#include "Generic/icews/CAMEOXMLResultCollector.h"

#include "Generic/state/XMLStrings.h"
#include "Generic/state/XMLElement.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/ActorMention.h"
#include "Generic/actors/ActorInfo.h"

#include <xercesc/dom/DOM.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

void CAMEOXMLResultCollector::loadDocTheory(DocTheory* theory) {
	_docTheory = theory;
}

std::wstring CAMEOXMLResultCollector::produceOutputHelper() {
	
	// Define macros
	static const XMLCh* X_CAMEO_XML = xercesc::XMLString::transcode("CAMEO_XML");
	static const XMLCh* X_Document = xercesc::XMLString::transcode("Document");
	static const XMLCh* X_Sentences = xercesc::XMLString::transcode("Sentences");
	static const XMLCh* X_Events = xercesc::XMLString::transcode("Events");
	static const XMLCh* X_Sentence = xercesc::XMLString::transcode("Sentence");
	static const XMLCh* X_Event = xercesc::XMLString::transcode("Event");
	static const XMLCh* X_id = xercesc::XMLString::transcode("id");
	static const XMLCh* X_tense = xercesc::XMLString::transcode("tense");
	static const XMLCh* X_type = xercesc::XMLString::transcode("type");
	static const XMLCh* X_char_offsets = xercesc::XMLString::transcode("char_offsets");
	static const XMLCh* X_sentence_id = xercesc::XMLString::transcode("sentence_id");
	
	Symbol SOURCE_SYM(L"SOURCE");
	Symbol TARGET_SYM(L"TARGET");

	// Make top element, and beneath it Sentences and Events elemetns
	xercesc::DOMImplementation* impl = xercesc::DOMImplementationRegistry::getDOMImplementation(SerifXML::X_Core);
	xercesc::DOMDocument *xercesDOMDocument = impl->createDocument(0, X_CAMEO_XML, 0);

	const wchar_t* doc_name = _docTheory->getDocument()->getName().to_string();
	SerifXML::XMLElement rootElem(xercesDOMDocument->getDocumentElement());
	SerifXML::XMLElement docElem = rootElem.addChild(X_Document);
	docElem.setAttribute(X_id, doc_name);
	SerifXML::XMLElement sentencesElem = docElem.addChild(X_Sentences);
	SerifXML::XMLElement eventsElem = docElem.addChild(X_Events);

	// Make Sentences elements
	for (int i = 0; i < _docTheory->getNSentences(); i++) {
		const Sentence *sentence = _docTheory->getSentence(i);
		CharOffset start_offset = sentence->getStartCharOffset();
		CharOffset end_offset = sentence->getEndCharOffset();
		std::wstringstream offsets;
		offsets << start_offset << L":" << end_offset;
		SerifXML::XMLElement sentenceElem = sentencesElem.addChild(X_Sentence);
		sentenceElem.setAttribute(X_id, i);
		sentenceElem.setAttribute(X_char_offsets, offsets.str());
		sentenceElem.addText(sentence->getString()->toString());
	}

	// Make Events elements from ICEWSEventMentions
	ICEWSEventMentionSet* eventMentionSet = _docTheory->getICEWSEventMentionSet();
	int i = 0;
	if (eventMentionSet) {
		int count = 0;
		BOOST_FOREACH(ICEWSEventMention_ptr em, (*eventMentionSet)) {

			if (em->isDatabaseWorthyEvent()) {
			
				SerifXML::XMLElement eventElem = eventsElem.addChild(X_Event);
				eventElem.setAttribute(X_id, i);
				i++;

				// Event Type, Tense, and Sentence ID
				eventElem.setAttribute(X_type, em->getEventType()->getEventCode());

				eventElem.setAttribute(X_tense, em->getEventTense().to_string());

				// Get Source Actor and Target Actor
				ActorMention_ptr sourceActor;
				ActorMention_ptr targetActor;
				typedef std::pair<Symbol, ActorMention_ptr> ParticipantPair;
				BOOST_FOREACH(const ParticipantPair &participantPair, em->getParticipantList()) {
					if (participantPair.first == TARGET_SYM) {
						targetActor = participantPair.second;
					} else if (participantPair.first == SOURCE_SYM) {
						sourceActor = participantPair.second;
					}
				}

				// Get sentence ID and offsets
				int serif_sent_no = sourceActor->getEntityMention()->getSentenceNumber();
				eventElem.setAttribute(X_sentence_id, serif_sent_no);
				const Sentence *sentence = _docTheory->getSentence(serif_sent_no);
				CharOffset start_offset = sentence->getStartCharOffset();
				CharOffset end_offset = sentence->getEndCharOffset();
				std::wstringstream offsets;
				offsets << start_offset << L":" << end_offset;
				eventElem.setAttribute(X_char_offsets, offsets.str());

				// Add actors
				addActorElement(eventElem, sourceActor, "Source");
				addActorElement(eventElem, targetActor, "Target");
			}
		}
	}

	return rootElem.toWString();
}

void CAMEOXMLResultCollector::addActorElement(SerifXML::XMLElement& eventElem, ActorMention_ptr am, std::string role) {
	
	static const XMLCh* X_actor_id = xercesc::XMLString::transcode("actor_id");
	static const XMLCh* X_actor_name = xercesc::XMLString::transcode("actor_name");
	static const XMLCh* X_agent_id = xercesc::XMLString::transcode("agent_id");
	static const XMLCh* X_agent_name = xercesc::XMLString::transcode("agent_name");	
	static const XMLCh* X_char_offsets = xercesc::XMLString::transcode("char_offsets");
	static const XMLCh* X_role = xercesc::XMLString::transcode("role");
	static const XMLCh* X_Participant = xercesc::XMLString::transcode("Participant");

	ActorId actorID = ActorId();
	AgentId agentID = AgentId();
	std::wstring actorName = L"";
	std::wstring agentName = L"";

	if (ProperNounActorMention_ptr pm = boost::dynamic_pointer_cast<ProperNounActorMention>(am)) {
		actorID = pm->getActorId();
		actorName = pm->getActorName();
	} else if (CompositeActorMention_ptr cm = boost::dynamic_pointer_cast<CompositeActorMention>(am)) {
		agentID = cm->getPairedAgentId();
		agentName = cm->getPairedAgentName();
		actorID = cm->getPairedActorId();
		actorName = cm->getPairedActorName();
	} 

	if (!actorID.isNull()) {
		SerifXML::XMLElement actorElem = eventElem.addChild(X_Participant);
		actorElem.setAttribute(X_role, role);
		std::wstring actor_id = boost::lexical_cast<std::wstring>(actorID.getId());
		actorElem.setAttribute(X_actor_id, actor_id);
		actorElem.setAttribute(X_actor_name, actorName);
		if (!agentID.isNull()) {
			std::wstring agent_id = boost::lexical_cast<std::wstring>(agentID.getId());
			actorElem.setAttribute(X_agent_id, agent_id);
			actorElem.setAttribute(X_agent_name, agentName);
		}
		// Add in character offsets for this actor
		int start_sentence_number = am->getEntityMention()->getSentenceNumber();
		int start_token = am->getEntityMention()->getNode()->getStartToken();
		CharOffset start_offset = am->getSentence()->getTokenSequence()->getToken(start_token)->getStartCharOffset();
		int end_sentence_number = am->getEntityMention()->getSentenceNumber();
		int end_token = am->getEntityMention()->getNode()->getEndToken();
		CharOffset end_offset = am->getSentence()->getTokenSequence()->getToken(end_token)->getEndCharOffset();
		std::wstringstream offsets;
		offsets << start_offset << L":" << end_offset;
		actorElem.setAttribute(X_char_offsets, offsets.str());
	}
}

void CAMEOXMLResultCollector::produceOutput(const wchar_t *output_dir,
									     const wchar_t *doc_filename)
{
	// File name
	std::wstring output_file = std::wstring(output_dir) + LSERIF_PATH_SEP + std::wstring(doc_filename);
	if (!boost::algorithm::ends_with(output_file, L".xml")) {
		output_file += L".cameo.xml";
	} else {
		output_file = boost::algorithm::replace_last_copy(output_file, ".xml", ".cameo.xml");
	}

	// Open output stream
	UTF8OutputStream stream;
	stream.open(output_file.c_str());

	std::wstring results = produceOutputHelper();

	// Write out
	stream << results;

	stream.close();
}

void CAMEOXMLResultCollector::produceOutput(std::wstring *results) { *results = produceOutputHelper(); }
