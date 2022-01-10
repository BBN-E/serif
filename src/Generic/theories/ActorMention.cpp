// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/theories/ActorMention.h"
#include "Generic/actors/Gazetteer.h"
#include "Generic/actors/ActorInfo.h"
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
#include <boost/lexical_cast.hpp>

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

ActorMention::ActorMention(const SentenceTheory *sentTheory, const Mention *mention, Symbol sourceNote)
		: _sentTheory(sentTheory), _mention_uid(mention->getUID()), _sourceNote(sourceNote)
{}

ActorMention::ActorMention(SerifXML::XMLTheoryElement elem) {
	using namespace SerifXML;

	SerifXML::XMLIdMap *idMap = elem.getXMLSerializedDocTheory()->getIdMap();

	// register our id -- note this is the raw pointer, not shared_ptr!  Be sure
	// to use shared_from_this to convert to a shared pointer if you look it up!
	elem.loadId(this); 

	if (idMap->hasId(elem.getAttribute<SerifXML::xstring>(X_sentence_theory_id)))
		_sentTheory = elem.loadTheoryPointer<SentenceTheory>(X_sentence_theory_id);
	else
		_sentTheory = 0; // We are in sentence-level processing, we'll set this in resolvePointers
	_mention_uid = elem.loadTheoryPointer<Mention>(X_mention_id)->getUID();
	_sourceNote = elem.getAttribute<Symbol>(X_source_note);
}

void ActorMention::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	using namespace SerifXML;

	elem.saveTheoryPointer(X_sentence_theory_id, _sentTheory);
	Mention *mention = _sentTheory->getMentionSet()->getMention(_mention_uid);
	elem.saveTheoryPointer(X_mention_id, mention);
	elem.setAttribute(X_source_note, _sourceNote);
}

ActorMention_ptr ActorMention::loadXML(SerifXML::XMLTheoryElement elem) {
	using namespace SerifXML;

	if (elem.hasAttribute(X_actor_uid))
		return boost::make_shared<ProperNounActorMention>(elem);
	else if (elem.hasAttribute(X_paired_agent_uid))
		return boost::make_shared<CompositeActorMention>(elem);
	else
		return boost::make_shared<ActorMention>(elem);
}

const Mention* ActorMention::getEntityMention() const {
	return _sentTheory->getMentionSet()->getMention(_mention_uid);
}

const SentenceTheory* ActorMention::getSentence() const {
	return _sentTheory;
}

ProperNounActorMention::ProperNounActorMention(SerifXML::XMLTheoryElement elem)
	: ActorMention(elem) 
{
	using namespace SerifXML;

	_geoResolution = boost::make_shared<Gazetteer::GeoResolution>();

	_actor.id = ActorId(static_cast<ActorId::id_type>(elem.getAttribute<size_t>(X_actor_uid)), elem.getAttribute<Symbol>(X_actor_db_name));
	if (elem.hasAttribute(X_actor_code))
		_actor.code = elem.getAttribute<Symbol>(X_actor_code);
	else
		_actor.code = Symbol();
	if (elem.hasAttribute(X_actor_pattern_uid))
		_actor.patternId = ActorPatternId(static_cast<ActorPatternId::id_type>(elem.getAttribute<size_t>(X_actor_pattern_uid)), elem.getAttribute<Symbol>(X_actor_db_name));
	else
		_actor.patternId = ActorPatternId();
	if (elem.hasAttribute(X_actor_name))
		_actor.actorName = elem.getAttribute<std::wstring>(X_actor_name);

	_actor.is_acronym = elem.getAttribute<bool>(X_is_acronym, false);
	_actor.requires_context = elem.getAttribute<bool>(X_requires_context, false);

	if (elem.hasAttribute(X_geo_country))
	{
		_geoResolution->cityname = elem.getAttribute<std::wstring>(X_geo_text);
		_geoResolution->countrycode = elem.getAttribute<std::wstring>(X_geo_country);
		_geoResolution->geonameid = elem.getAttribute<std::wstring>(X_geo_uid);
		_geoResolution->latitude = (elem.getAttribute<std::wstring>(X_geo_latitude) == L"") ? std::numeric_limits<float>::quiet_NaN() : boost::lexical_cast<float>(elem.getAttribute<std::wstring>(X_geo_latitude));
		_geoResolution->longitude = (elem.getAttribute<std::wstring>(X_geo_latitude) == L"") ? std::numeric_limits<float>::quiet_NaN() : boost::lexical_cast<float>(elem.getAttribute<std::wstring>(X_geo_longitude));
		_geoResolution->isEmpty = false;

		if (elem.hasAttribute(X_country_id)) {
			_geoResolution->countryInfo = boost::make_shared<Gazetteer::CountryInfo>();
			_geoResolution->countryInfo->countryId = CountryId(static_cast<long>(elem.getAttribute<size_t>(X_country_id)));
			_geoResolution->countryInfo->isoCode = elem.getAttribute<Symbol>(X_iso_code);
			_geoResolution->countryInfo->actorId = ActorId(static_cast<ActorId::id_type>(elem.getAttribute<size_t>(X_country_info_actor_id)), elem.getAttribute<Symbol>(X_actor_db_name));
			if (elem.hasAttribute(X_country_info_actor_code))
				_geoResolution->countryInfo->actorCode = elem.getAttribute<Symbol>(X_country_info_actor_code);
			else
				_geoResolution->countryInfo->actorCode = Symbol();
		}
	}

	if (elem.hasAttribute(X_pattern_match_score))
		_actorMatchScores.pattern_match_score = elem.getAttribute<double>(X_pattern_match_score);
	if (elem.hasAttribute(X_pattern_confidence_score))
		_actorMatchScores.pattern_confidence_score = elem.getAttribute<double>(X_pattern_confidence_score);
	if (elem.hasAttribute(X_association_score))
		_actorMatchScores.association_score = elem.getAttribute<double>(X_association_score);
	if (elem.hasAttribute(X_edit_distance_score))
		_actorMatchScores.edit_distance_score = elem.getAttribute<double>(X_edit_distance_score);
	if (elem.hasAttribute(X_georesolution_score))
		_actorMatchScores.georesolution_score = elem.getAttribute<double>(X_georesolution_score);
	if (elem.hasAttribute(X_importance_score))
		_actorMatchScores.importance_score = elem.getAttribute<double>(X_importance_score);

	// Backwards compatibility
	if (!elem.hasAttribute(X_actor_name))
		_actor.actorName = elem.getText<std::wstring>();
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
	using namespace SerifXML;

	ActorMention::saveXML(elem, context);
	elem.setAttribute<size_t>(X_actor_uid, _actor.id.getId());
	elem.setAttribute<std::wstring>(X_actor_name, _actor.actorName);
	if (_actor.code != Symbol())
		elem.setAttribute(X_actor_code, _actor.code);
	if (!_actor.patternId.isNull())
		elem.setAttribute<size_t>(X_actor_pattern_uid, _actor.patternId.getId());
	elem.setAttribute<Symbol>(X_actor_db_name, _actor.id.getDbName());
	elem.setAttribute<bool>(X_is_acronym, _actor.is_acronym);
	elem.setAttribute<bool>(X_requires_context, _actor.requires_context);

	if (isResolvedGeo()) {
		// if latitude / longitude is NaN, use blank string
		elem.setAttribute(X_geo_latitude, ((_geoResolution->latitude != _geoResolution->latitude) ? L"" : boost::lexical_cast<std::wstring>(_geoResolution->latitude)));
		elem.setAttribute(X_geo_longitude,((_geoResolution->longitude != _geoResolution->longitude) ? L"" : boost::lexical_cast<std::wstring>(_geoResolution->longitude)));
		elem.setAttribute(X_geo_uid, ((_geoResolution->geonameid == L"") ? L"" : boost::lexical_cast<std::wstring>(_geoResolution->geonameid)));
		elem.setAttribute(X_geo_text, ((_geoResolution->cityname.is_null()) ? L"" : _geoResolution->cityname));
		elem.setAttribute(X_geo_country, ((_geoResolution->countrycode.is_null()) ? L"" : _geoResolution->countrycode));

		Gazetteer::CountryInfo_ptr countryInfo = _geoResolution->countryInfo;
		if (countryInfo) {
			elem.setAttribute<size_t>(X_country_id, countryInfo->countryId.getId());
			elem.setAttribute(X_iso_code, countryInfo->isoCode);
			elem.setAttribute<size_t>(X_country_info_actor_id, countryInfo->actorId.getId());
			if (countryInfo->actorCode != Symbol())
				elem.setAttribute(X_country_info_actor_code, countryInfo->actorCode);
		}
	}

	if (_actorMatchScores.pattern_match_score != 0)
		elem.setAttribute(X_pattern_match_score, _actorMatchScores.pattern_match_score);
	if (_actorMatchScores.pattern_confidence_score != 0)
		elem.setAttribute(X_pattern_confidence_score, _actorMatchScores.pattern_confidence_score);
	if (_actorMatchScores.association_score != 0)
		elem.setAttribute(X_association_score, _actorMatchScores.association_score);
	if (_actorMatchScores.edit_distance_score != 0)
		elem.setAttribute(X_edit_distance_score, _actorMatchScores.edit_distance_score);
	if (_actorMatchScores.georesolution_score != 0)
		elem.setAttribute(X_georesolution_score, _actorMatchScores.georesolution_score);
	if (_actorMatchScores.importance_score != 0)
		elem.setAttribute(X_importance_score, _actorMatchScores.importance_score);
}


Symbol ProperNounActorMention::getCountryIsoCode(ActorInfo_ptr actorInfo) const {
	if (isNamedLocation() && isResolvedGeo()) {
		return _geoResolution->countrycode;
	}
	else {
		return actorInfo->getIsoCodeForActor(getActorId());
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
	using namespace SerifXML;

	// Paired agent.
	_pairedAgent.id = AgentId(static_cast<AgentId::id_type>(elem.getAttribute<size_t>(X_paired_agent_uid)), elem.getAttribute<Symbol>(X_actor_db_name));
	_pairedAgent.code = elem.getAttribute<Symbol>(X_paired_agent_code);
	if (elem.hasAttribute(X_paired_agent_pattern_uid))
		_pairedAgent.patternId = AgentPatternId(static_cast<AgentPatternId::id_type>(elem.getAttribute<size_t>(X_paired_agent_pattern_uid)), elem.getAttribute<Symbol>(X_actor_db_name));
	else
		_pairedAgent.patternId = AgentPatternId();
	if (elem.hasAttribute(X_paired_agent_name))
		_pairedAgent.agentName = elem.getAttribute<std::wstring>(X_paired_agent_name);
	
	// Paired actor.
	if (elem.hasAttribute(X_paired_actor_uid))
		_pairedActor.id = ActorId(static_cast<ActorId::id_type>(elem.getAttribute<size_t>(X_paired_actor_uid)), elem.getAttribute<Symbol>(X_actor_db_name));
	else
		_pairedActor.id = ActorId();
	if (elem.hasAttribute(X_paired_actor_name))
		_pairedActor.actorName = elem.getAttribute<std::wstring>(X_paired_actor_name);

	if (elem.hasAttribute(X_paired_actor_code))
		_pairedActor.code = elem.getAttribute<Symbol>(X_paired_actor_code, Symbol());
	else
		_pairedActor.code = Symbol();

	if (elem.hasAttribute(X_paired_actor_pattern_uid))
		_pairedActor.patternId = ActorPatternId(static_cast<ActorPatternId::id_type>(elem.getAttribute<size_t>(X_paired_actor_pattern_uid)), elem.getAttribute<Symbol>(X_actor_db_name));
	else
		_pairedActor.patternId = ActorPatternId();
	_pairedActor.is_acronym = elem.getAttribute<bool>(X_paired_actor_is_acronym, false);
	_pairedActor.requires_context = elem.getAttribute<bool>(X_paired_actor_requires_context, false);	

	// Actor-Agent Pattern.
	_agentActorPatternName = elem.getAttribute<Symbol>(X_actor_agent_pattern, Symbol());

	// Backwards compatibility
	if (!elem.hasAttribute(X_paired_actor_name) && !elem.hasAttribute(X_paired_actor_name)) {
		std::wstring text = elem.getText<std::wstring>();
		if (text.size() > 0) {
			size_t pos = text.find(L" FOR ");
			if (pos != std::wstring::npos) {
				_pairedAgent.agentName = text.substr(0, pos);
				_pairedActor.actorName = text.substr(pos + 5);
			}
		}
	}
}

// Scores
void ProperNounActorMention::setPatternMatchScore(double score) { _actorMatchScores.pattern_match_score = score; }
void ProperNounActorMention::setPatternConfidenceScore(double score) { _actorMatchScores.pattern_confidence_score = score; }
void ProperNounActorMention::setAssociationScore(double score) { _actorMatchScores.association_score = score; }
void ProperNounActorMention::setEditDistanceScore(double score) { _actorMatchScores.edit_distance_score = score; }
void ProperNounActorMention::setGeoresolutionScore(double score) { _actorMatchScores.georesolution_score = score; }
void ProperNounActorMention::setImportanceScore(double score) { _actorMatchScores.importance_score = score; }

void ProperNounActorMention::copyScores(ProperNounActorMention_ptr pnam) {
	setPatternMatchScore(pnam->getPatternMatchScore());
	setPatternConfidenceScore(pnam->getPatternConfidenceScore());
	setAssociationScore(pnam->getAssociationScore());
	setEditDistanceScore(pnam->getEditDistanceScore());
	setGeoresolutionScore(pnam->getGeoresolutionScore());
	setImportanceScore(pnam->getImportanceScore());
}

double ProperNounActorMention::getPatternMatchScore() { return _actorMatchScores.pattern_match_score; }
double ProperNounActorMention::getPatternConfidenceScore() { return _actorMatchScores.pattern_confidence_score; }
double ProperNounActorMention::getAssociationScore() { return _actorMatchScores.association_score; }
double ProperNounActorMention::getEditDistanceScore() { return _actorMatchScores.edit_distance_score; }
double ProperNounActorMention::getGeoresolutionScore() { return _actorMatchScores.georesolution_score; }
double ProperNounActorMention::getImportanceScore() { return _actorMatchScores.importance_score; }


void CompositeActorMention::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	using namespace SerifXML;

	ActorMention::saveXML(elem, context);
	// Agent information.
	elem.setAttribute<size_t>(X_paired_agent_uid, _pairedAgent.id.getId());
	elem.setAttribute<std::wstring>(X_paired_agent_name, _pairedAgent.agentName);
	elem.setAttribute(X_paired_agent_code, _pairedAgent.code);
	if (!_pairedAgent.patternId.isNull())
		elem.setAttribute<size_t>(X_paired_agent_pattern_uid, _pairedAgent.patternId.getId());
	elem.setAttribute<Symbol>(X_actor_db_name, _pairedAgent.id.getDbName());
	
	// Actor information.
	if (!_pairedActor.id.isNull()) {
		elem.setAttribute<size_t>(X_paired_actor_uid, _pairedActor.id.getId());
		elem.setAttribute<std::wstring>(X_paired_actor_name, _pairedActor.actorName);
		if (!_pairedActor.code.is_null())
			elem.setAttribute(X_paired_actor_code, _pairedActor.code);
		if (!_pairedActor.patternId.isNull())
			elem.setAttribute<size_t>(X_paired_actor_pattern_uid, _pairedActor.patternId.getId());
		elem.setAttribute<bool>(X_paired_actor_is_acronym, _pairedActor.is_acronym);
		elem.setAttribute<bool>(X_paired_actor_requires_context, _pairedActor.requires_context);
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
	ProperNounActorMention_ptr newActorMention = boost::make_shared<ProperNounActorMention>(sentTheory, mention,
		Symbol((std::wstring(note)+L":"+getSourceNote().to_string()).c_str()),
		ActorIdentifiers(_actor));
	if (isResolvedGeo())
		newActorMention->setGeoResolution(_geoResolution);
	return newActorMention;
}
ActorMention_ptr CompositeActorMention::copyWithNewEntityMention(const SentenceTheory* sentTheory, const Mention* mention, const wchar_t *note) {
	return boost::make_shared<CompositeActorMention>(sentTheory, mention,
		Symbol((std::wstring(note)+L":"+getSourceNote().to_string()).c_str()),
		AgentIdentifiers(_pairedAgent), 
		ActorIdentifiers(_pairedActor), 
		_agentActorPatternName);
}

void ActorMention::dump(std::wostream &stream) {
	stream << L"<UNKNOWN_ACTOR (\"" << getEntityMention()->toCasedTextString() << "\")>";
}

void ProperNounActorMention::dump(std::wostream &stream) {
	stream << L"<" << _actor.actorName << L" ("
		<< getActorId().getId() << ")>";
}

void CompositeActorMention::dump(std::wostream &stream) {
	stream << L"<" << _pairedAgent.agentName << L" ("
		<< getPairedAgentId().getId() << ") FOR " 
		<< _pairedActor.actorName << L" ("
		<< getPairedActorId().getId() << ")>";
}

void CompositeActorMention::setPairedActorMention(ProperNounActorMention_ptr pairedActor, const wchar_t *note) {
	_pairedActor.id = pairedActor->getActorId();
	_pairedActor.code = pairedActor->getActorCode();
	_pairedActor.patternId = pairedActor->getActorPatternId();
	_pairedActor.actorName = pairedActor->getActorName();
	setSourceNote(Symbol((std::wstring(note)+getSourceNote().to_string()).c_str()));
}

void CompositeActorMention::setPairedActorIdentifiers(const ActorIdentifiers& pairedActorInfo, const wchar_t *note) {
	_pairedActor = pairedActorInfo;
	setSourceNote(Symbol((std::wstring(note)+getSourceNote().to_string()).c_str()));
}

std::wostream & operator << ( std::wostream &stream, ActorMention_ptr actorMention ) {
	if (actorMention) actorMention->dump(stream);
	else stream << L"<NULL>";
	return stream;
}

std::ostream & operator << ( std::ostream &stream, ActorMention_ptr actorMention ) {
	if (actorMention) {
		std::wostringstream s;
		actorMention->dump(s);
		stream << UnicodeUtil::toUTF8StdString(s.str());
	} else {
		stream << "<NULL>";
	}
	return stream;
}

SessionLogger::LogMessageMaker & operator << ( SessionLogger::LogMessageMaker &stream, ActorMention_ptr actorMention ) {
	if (actorMention) {
		std::wostringstream s;
		actorMention->dump(s);
		stream << UnicodeUtil::toUTF8StdString(s.str());
	} else {
		stream << "<NULL>";
	}
	return stream;
}
