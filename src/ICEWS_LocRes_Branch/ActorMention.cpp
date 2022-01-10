// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "ICEWS/ActorMention.h"
#include "ICEWS/Gazetteer.h"
#include "ICEWS/SentenceSpan.h"
#include "ICEWS/ActorInfo.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"
#include <xercesc/util/XMLString.hpp>
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/Metadata.h"
#include <boost/make_shared.hpp>

namespace ICEWS {

void ActorMention::updateObjectIDTable() const {
	throw InternalInconsistencyException("ActorMention::updateObjectIDTable",
		"ActorMention does not currently have state file support");
}
void ActorMention::saveState(StateSaver *stateSaver) const {
	throw InternalInconsistencyException("ActorMention::saveState",
		"ActorMention does not currently have state file support");
}
void ActorMention::resolvePointers(StateLoader * stateLoader) {
	throw InternalInconsistencyException("ActorMention::resolvePointers",
		"ActorMention does not currently have state file support");
}


ActorMention::ActorMention(SerifXML::XMLTheoryElement elem) {
	// register our id -- note this is the raw pointer, not shared_ptr!  Be sure
	// to use shared_from_this to convert to a shared pointer if you look it up!
	elem.loadId(this); 
	static const XMLCh* X_sentence = xercesc::XMLString::transcode("sentence_theory_id");
	static const XMLCh* X_mention = xercesc::XMLString::transcode("mention_id");
	static const XMLCh* X_source_note = xercesc::XMLString::transcode("source_note");

	_sentTheory = elem.loadTheoryPointer<SentenceTheory>(X_sentence);
	_mention = elem.loadTheoryPointer<Mention>(X_mention);
	_sourceNote = elem.getAttribute<Symbol>(X_source_note);
}

void ActorMention::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	static const XMLCh* X_sentence = xercesc::XMLString::transcode("sentence_theory_id");
	static const XMLCh* X_mention = xercesc::XMLString::transcode("mention_id");
	static const XMLCh* X_source_note = xercesc::XMLString::transcode("source_note");

	// In the current design, we can sometimes make copies of mentions, 
	// rather using pointers to them.  Therefore, it's possible that the
	// mention we just got isn't actually a mention that's been
	// serialized, so we won't have a pointer to it.  But there should
	// be an identical mention with the same MentionUID, so look that 
	// up.
	const Mention *mention = _mention;
	if (!elem.getXMLSerializedDocTheory()->getIdMap()->hasId(mention)) {
		const Mention *mentionFromSentence = elem.getXMLSerializedDocTheory()->lookupMentionById(mention->getUID());
		assert(mention->isIdenticalTo(*mentionFromSentence));
		mention = mentionFromSentence;
	}

	elem.saveTheoryPointer(X_sentence, _sentTheory);
	elem.saveTheoryPointer(X_mention, mention);
	elem.setAttribute(X_source_note, _sourceNote);
}

ActorMention_ptr ActorMention::loadXML(SerifXML::XMLTheoryElement elem) {
	static const XMLCh* X_actor_uid = xercesc::XMLString::transcode("actor_uid");
	static const XMLCh* X_paired_agent_uid = xercesc::XMLString::transcode("paired_agent_uid");
	if (elem.hasAttribute(X_actor_uid))
		return boost::make_shared<ProperNounActorMention>(elem);
	else if (elem.hasAttribute(X_paired_agent_uid))
		return boost::make_shared<CompositeActorMention>(elem);
	else
		return boost::make_shared<ActorMention>(elem);
}



ProperNounActorMention::ProperNounActorMention(SerifXML::XMLTheoryElement elem)
	: ActorMention(elem) 
{
	static const XMLCh* X_actor_uid = xercesc::XMLString::transcode("actor_uid");
	static const XMLCh* X_actor_code = xercesc::XMLString::transcode("actor_code");
	static const XMLCh* X_actor_pattern_id = xercesc::XMLString::transcode("actor_pattern_uid");
	static const XMLCh* X_actor_country = xercesc::XMLString::transcode("geo_country");
	_actor.id = ActorId(static_cast<ActorId::id_type>(elem.getAttribute<size_t>(X_actor_uid)));
	_actor.code = elem.getAttribute<Symbol>(X_actor_code);
	_actor.patternId = ActorPatternId(static_cast<ActorPatternId::id_type>(elem.getAttribute<size_t>(X_actor_pattern_id)));
	if (elem.hasAttribute(X_actor_country))
	{
		static const XMLCh* X_actor_latitude = xercesc::XMLString::transcode("geo_latitude");
		static const XMLCh* X_actor_longitude = xercesc::XMLString::transcode("geo_longitude");
		static const XMLCh* X_actor_geonameId = xercesc::XMLString::transcode("geo_uid");
		static const XMLCh* X_actor_geoname = xercesc::XMLString::transcode("geo_text");
		_geoResolution->cityname = elem.getAttribute<std::wstring>(X_actor_geoname);
		_geoResolution->countrycode = elem.getAttribute<std::wstring>(X_actor_country);
		_geoResolution->geonameid = elem.getAttribute<std::wstring>(X_actor_geonameId);
		_geoResolution->latitude = elem.getAttribute<std::wstring>(X_actor_latitude);
		_geoResolution->longitude = elem.getAttribute<std::wstring>(X_actor_longitude);
		_geoResolution->isEmpty = false;
	}
}

bool ProperNounActorMention::isResolvedGeo() const {
	if (_geoResolution)  return !_geoResolution->isEmpty;
	else return false;
}

bool ProperNounActorMention::isNamedLocation() const {
	const Mention *ment = getEntityMention();
	EntityType entityType = ment->getEntityType();
	if ((ment->getMentionType() == Mention::NAME) && (entityType.matchesLOC() || entityType.matchesGPE())) return true;
	else return false;
}

void ProperNounActorMention::setGeoResolution(Gazetteer::GeoResolution_ptr geoResolution) {
	_geoResolution->longitude = geoResolution->longitude;
	_geoResolution->latitude = geoResolution->latitude;
	_geoResolution->cityname = geoResolution->cityname;
	_geoResolution->geonameid = geoResolution->geonameid;
	_geoResolution->isEmpty = geoResolution->isEmpty;
	_geoResolution->countryInfo = geoResolution->countryInfo;
	// occassionally a geoResolution is not a part of country (e.g. "Persian Gulf")
	if (!geoResolution->countryInfo)
		_geoResolution->countrycode = Symbol(L"N/A");
	else 
		_geoResolution->countrycode = geoResolution->countrycode;
	
}

void ProperNounActorMention::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	static const XMLCh* X_actor_uid = xercesc::XMLString::transcode("actor_uid");
	static const XMLCh* X_actor_code = xercesc::XMLString::transcode("actor_code");
	static const XMLCh* X_actor_pattern_id = xercesc::XMLString::transcode("actor_pattern_uid");

	ActorMention::saveXML(elem, context);
	elem.setAttribute<size_t>(X_actor_uid, _actor.id.getId());
	elem.setAttribute(X_actor_code, _actor.code);
	elem.setAttribute<size_t>(X_actor_pattern_id, _actor.patternId.getId());
	if (ParamReader::isParamTrue("icews_include_actor_names_in_serifxml")) {
		ActorInfo_ptr actorInfo = ActorInfo::getActorInfoSingleton();
		elem.addText(actorInfo->getActorName(_actor.id));
	}
	if (isNamedLocation() && isResolvedGeo()) {
		static const XMLCh* X_actor_latitude = xercesc::XMLString::transcode("geo_latitude");
		static const XMLCh* X_actor_longitude = xercesc::XMLString::transcode("geo_longitude");
		static const XMLCh* X_actor_geonameId = xercesc::XMLString::transcode("geo_uid");
		static const XMLCh* X_actor_geoname = xercesc::XMLString::transcode("geo_text");
		static const XMLCh* X_actor_country = xercesc::XMLString::transcode("geo_country");
		elem.setAttribute(X_actor_latitude, ((_geoResolution->latitude.is_null()) ? L"" : _geoResolution->latitude));
		elem.setAttribute(X_actor_longitude,((_geoResolution->longitude.is_null()) ? L"" : _geoResolution->longitude));
		elem.setAttribute(X_actor_geonameId, ((_geoResolution->geonameid.is_null()) ? L"" : _geoResolution->geonameid));
		elem.setAttribute(X_actor_geoname, ((_geoResolution->cityname.is_null()) ? L"" : _geoResolution->cityname));
		elem.setAttribute(X_actor_country, ((_geoResolution->countrycode.is_null()) ? L"" : _geoResolution->countrycode));
	}
}


Symbol ProperNounActorMention::getCountryIsoCode() const {
	if (isNamedLocation() && isResolvedGeo()) {
		return _geoResolution->countrycode;
	}
	else {
		return Gazetteer::getIsoCodeForActor(getActorId());
	}
}

size_t ProperNounActorMention::getGeoPopulation() const {
	if (isResolvedGeo()) {
		return _geoResolution->population;
	}
	return 0;
}

CompositeActorMention::CompositeActorMention(SerifXML::XMLTheoryElement elem)
	: ActorMention(elem) 
{
	static const XMLCh* X_paired_agent_uid = xercesc::XMLString::transcode("paired_agent_uid");
	static const XMLCh* X_paired_agent_code = xercesc::XMLString::transcode("paired_agent_code");
	static const XMLCh* X_paired_agent_pattern_uid = xercesc::XMLString::transcode("paired_agent_pattern_uid");
	static const XMLCh* X_paired_actor_uid = xercesc::XMLString::transcode("paired_actor_uid");
	static const XMLCh* X_paired_actor_code = xercesc::XMLString::transcode("paired_actor_code");
	static const XMLCh* X_paired_actor_pattern_uid = xercesc::XMLString::transcode("paired_actor_pattern_uid");
	static const XMLCh* X_actor_agent_pattern = xercesc::XMLString::transcode("actor_agent_pattern");
	// Paired agent.
	_pairedAgent.id = AgentId(static_cast<AgentId::id_type>(elem.getAttribute<size_t>(X_paired_agent_uid)));
	_pairedAgent.code = elem.getAttribute<Symbol>(X_paired_agent_code);
	_pairedAgent.patternId = AgentPatternId(static_cast<AgentPatternId::id_type>(elem.getAttribute<size_t>(X_paired_agent_pattern_uid)));
	// Paired actor.
	if (elem.hasAttribute(X_paired_actor_uid))
		_pairedActor.id = ActorId(static_cast<ActorId::id_type>(elem.getAttribute<size_t>(X_paired_actor_uid)));
	_pairedActor.code = elem.getAttribute<Symbol>(X_paired_actor_code, Symbol());
	if (elem.hasAttribute(X_paired_actor_pattern_uid))
		_pairedActor.patternId = ActorPatternId(static_cast<ActorPatternId::id_type>(elem.getAttribute<size_t>(X_paired_actor_pattern_uid)));
	// Actor-Agent Pattern.
	_agentActorPatternName = elem.getAttribute<Symbol>(X_actor_agent_pattern, Symbol());

}

void CompositeActorMention::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	static const XMLCh* X_paired_agent_uid = xercesc::XMLString::transcode("paired_agent_uid");
	static const XMLCh* X_paired_agent_code = xercesc::XMLString::transcode("paired_agent_code");
	static const XMLCh* X_paired_agent_pattern_uid = xercesc::XMLString::transcode("paired_agent_pattern_uid");
	static const XMLCh* X_paired_actor_uid = xercesc::XMLString::transcode("paired_actor_uid");
	static const XMLCh* X_paired_actor_code = xercesc::XMLString::transcode("paired_actor_code");
	static const XMLCh* X_paired_actor_pattern_uid = xercesc::XMLString::transcode("paired_actor_pattern_uid");
	static const XMLCh* X_actor_agent_pattern = xercesc::XMLString::transcode("actor_agent_pattern");
	ActorMention::saveXML(elem, context);
	// Agent information.
	elem.setAttribute<size_t>(X_paired_agent_uid, _pairedAgent.id.getId());
	elem.setAttribute(X_paired_agent_code, _pairedAgent.code);
	elem.setAttribute<size_t>(X_paired_agent_pattern_uid, _pairedAgent.patternId.getId());
	// Actor information.
	if (!_pairedActor.id.isNull()) {
		elem.setAttribute<size_t>(X_paired_actor_uid, _pairedActor.id.getId());
		elem.setAttribute(X_paired_actor_code, _pairedActor.code);
		elem.setAttribute<size_t>(X_paired_actor_pattern_uid, _pairedActor.patternId.getId());
	}

	if (ParamReader::isParamTrue("icews_include_actor_names_in_serifxml")) {
		ActorInfo_ptr actorInfo = ActorInfo::getActorInfoSingleton();
		elem.addText(actorInfo->getAgentName(_pairedAgent.id));
		elem.addText(" FOR ");
		elem.addText(actorInfo->getActorName(_pairedActor.id));
	}
	// Actor-Agent Pattern.
	if (!_agentActorPatternName.is_null())
		elem.setAttribute(X_actor_agent_pattern, _agentActorPatternName);
}

namespace {
	Symbol COPY_SYM(L"COPY");
}

void ActorMention::addSourceNote(const wchar_t *note) {
	_sourceNote = Symbol((std::wstring(note)+L":"+getSourceNote().to_string()).c_str());
}

ActorMention_ptr ActorMention::copyWithNewEntityMention(const SentenceTheory* sentTheory, const Mention* mention, const wchar_t *note) {
	return boost::make_shared<ActorMention>(sentTheory, mention, 
		Symbol((std::wstring(note)+L":"+getSourceNote().to_string()).c_str()));
}
ActorMention_ptr ProperNounActorMention::copyWithNewEntityMention(const SentenceTheory* sentTheory, const Mention* mention, const wchar_t *note) {
	return boost::make_shared<ProperNounActorMention>(sentTheory, mention,
		Symbol((std::wstring(note)+L":"+getSourceNote().to_string()).c_str()),
		ActorIdentifiers(_actor.id, _actor.code, _actor.patternId));
}
ActorMention_ptr CompositeActorMention::copyWithNewEntityMention(const SentenceTheory* sentTheory, const Mention* mention, const wchar_t *note) {
	return boost::make_shared<CompositeActorMention>(sentTheory, mention,
		Symbol((std::wstring(note)+L":"+getSourceNote().to_string()).c_str()),
		AgentIdentifiers(_pairedAgent.id, _pairedAgent.code, _pairedAgent.patternId), 
		ActorIdentifiers(_pairedActor.id, _pairedActor.code, _pairedActor.patternId), 
		_agentActorPatternName);
}

size_t ActorMention::getIcewsSentNo(const DocTheory* docTheory) const {
	if (!ParamReader::getRequiredTrueFalseParam("use_icews_sentence_numbers"))
		return _mention->getSentenceNumber();
		
	static Symbol SENTENCE_SPAN_SYM(L"ICEWS_Sentence");
	Metadata* metadata = docTheory->getDocument()->getMetadata();
	const TokenSequence *ts = docTheory->getSentenceTheory(_mention->getSentenceNumber())->getTokenSequence();

	// Return the sentence number for the sentence containing the first
	// character of the actor mention.
	EDTOffset start = ts->getToken(_mention->getNode()->getStartToken())->getStartEDTOffset();
	if (IcewsSentenceSpan *sspan = dynamic_cast<IcewsSentenceSpan*>(metadata->getCoveringSpan(start, SENTENCE_SPAN_SYM))) {
		return sspan->getSentNo();
	}

	// Fallback: return the sentence number for the sentence containing 
	// the last character of the actor mention.  (Should never be necessary.)
	EDTOffset end = ts->getToken(_mention->getNode()->getEndToken())->getEndEDTOffset();
	if (IcewsSentenceSpan *sspan = dynamic_cast<IcewsSentenceSpan*>(metadata->getCoveringSpan(end, SENTENCE_SPAN_SYM))) {
		return sspan->getSentNo();
	}

	throw UnexpectedInputException("ActorMention::getIcewsSentNo",
		"No ICEWS_Sentence metadata span found covering actor mention!");
}

void ActorMention::dump(std::wostream &stream) {
	stream << L"<UNKNOWN_ACTOR (\"" << getEntityMention()->toCasedTextString() << "\")>";
}

void ProperNounActorMention::dump(std::wostream &stream) {
	ActorInfo_ptr actorInfo = ActorInfo::getActorInfoSingleton();
	stream << L"<" << actorInfo->getActorName(getActorId()) << L" ("
		<< getActorId().getId() << ")>";
}

void CompositeActorMention::dump(std::wostream &stream) {
	ActorInfo_ptr actorInfo = ActorInfo::getActorInfoSingleton();
	stream << L"<" << actorInfo->getAgentName(getPairedAgentId()) << L" ("
		<< getPairedAgentId().getId() << ") FOR " 
		<< actorInfo->getActorName(getPairedActorId()) << L" ("
		<< getPairedActorId().getId() << ")>";
}

void CompositeActorMention::setPairedActorMention(ProperNounActorMention_ptr pairedActor, const wchar_t *note) {
	_pairedActor.id = pairedActor->getActorId();
	_pairedActor.code = pairedActor->getActorCode();
	_pairedActor.patternId = pairedActor->getActorPatternId();
	setSourceNote(Symbol((std::wstring(note)+getSourceNote().to_string()).c_str()));
}

void CompositeActorMention::setPairedActorIdentifiers(const ActorIdentifiers& pairedActorInfo, const wchar_t *note) {
	_pairedActor = pairedActorInfo;
	setSourceNote(Symbol((std::wstring(note)+getSourceNote().to_string()).c_str()));
}


} // end of ICEWS namespace


std::wostream & operator << ( std::wostream &stream, ICEWS::ActorMention_ptr actorMention ) {
	if (actorMention) actorMention->dump(stream);
	else stream << L"<NULL>";
	return stream;
}

std::ostream & operator << ( std::ostream &stream, ICEWS::ActorMention_ptr actorMention ) {
	if (actorMention) {
		std::wostringstream s;
		actorMention->dump(s);
		stream << UnicodeUtil::toUTF8StdString(s.str());
	} else {
		stream << "<NULL>";
	}
	return stream;
}

SessionLogger::LogMessageMaker & operator << ( SessionLogger::LogMessageMaker &stream, ICEWS::ActorMention_ptr actorMention ) {
	if (actorMention) {
		std::wostringstream s;
		actorMention->dump(s);
		stream << UnicodeUtil::toUTF8StdString(s.str());
	} else {
		stream << "<NULL>";
	}
	return stream;
}


