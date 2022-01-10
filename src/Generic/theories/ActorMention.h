// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ACTOR_MENTION_H
#define ACTOR_MENTION_H

#include "Generic/theories/Theory.h"
#include "Generic/theories/Mention.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/actors/Identifiers.h"
#include "Generic/actors/Gazetteer.h"
#include "Generic/actors/JabariTokenMatcher.h"
#include "Generic/actors/ActorInfo.h"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "Generic/common/BoostUtil.h"
#include <iosfwd>

// Forward declarations:
class SentenceTheory;
class DocTheory;

/** A SERIF theory used to identify a single mention of an "actor."  
  * Each ActorMention corresponds with a single entity mention (i.e., Mention 
  * object), and can optionally be labeled with ICEWSIdentifiers that link it 
  * to an external database of actors.
  *
  * The base class simply wraps a Mention object.  Subclasses are used
  * to add pointers into the external database of actors.  Two different
  * subclasses are defined: ProperNounActorMention is used for named
  * actors, and CompositeActorMention is used for "agents of named
  * actors," such as "an activist for the United Nations".
  *
  * ActorMention objects are meant to be accessed via boost shared
  * pointers.  They are not copyable.  To downcast from an ActorMention_ptr
  * to a subclass pointer, use boost::dynamic_pointer_cast<T>().
  *
  * New ActorMention objects should be created using boost::make_shared().
  * Raw pointers to ActorMentions (such as those returned by 
  * SerifXML::XMLTheoryElement::loadTheoryPointer) can be converted into
  * shared pointers using ActorMention::shared_from_this().
  */
class ActorMention : public Theory, public boost::enable_shared_from_this<ActorMention>, private boost::noncopyable {
public:	
	struct ActorIdentifiers {
		ActorId id;
		Symbol code;
		ActorPatternId patternId;
		bool is_acronym;
		bool requires_context;
		std::wstring actorName;
		explicit ActorIdentifiers(ActorId id=ActorId(), std::wstring actorName=L"UNKNOWN-ACTOR", Symbol code=Symbol(), ActorPatternId patternId=ActorPatternId(), bool is_acronym=false, bool requires_context=false): 
			id(id), code(code), patternId(patternId), is_acronym(is_acronym), requires_context(requires_context), actorName(actorName) {}
		ActorIdentifiers(const ActorMatch& match, std::wstring actorName, bool requires_context) :
			id(match.id), code(match.code), patternId(match.patternId), is_acronym(match.isAcronymMatch), requires_context(requires_context), actorName(actorName) {}
	};
	struct AgentIdentifiers {
		AgentId id;
		Symbol code;
		AgentPatternId patternId;
		std::wstring agentName;
		explicit AgentIdentifiers(AgentId id=AgentId(), std::wstring agentName=L"", Symbol code=Symbol(), AgentPatternId patternId=AgentPatternId()): id(id), code(code), patternId(patternId), agentName(agentName) {}
	};

	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(ActorMention, const SentenceTheory*, const Mention*, const Symbol&);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(ActorMention, SerifXML::XMLTheoryElement);
	virtual ~ActorMention() {}

	/** Return the entity mention that corresponds with this actor. */
	const Mention* getEntityMention() const;
	MentionUID getEntityMentionUID() const { return _mention_uid; }

	/** Return the SentenceTheory for the sentence that contains this mention. */
	const SentenceTheory* getSentence() const;

	/** In sentence-level processing, the SentenceTheorys get deleted */
	void setSentenceTheory(const SentenceTheory *theory) { _sentTheory = theory; }

	/** Return the "source note", which records how/where this actor was generated. */
	Symbol getSourceNote() const { return _sourceNote; }

	void addSourceNote(const wchar_t *note);
	void addSourceNote(const Symbol &note) { addSourceNote(note.to_string()); }

	/** Return a copy of this ActorMention (pointing at the same external
	  * database actor, if applicable) with a new entity mention. This is
	  * used when labeling a mention based on the fact that is coreferent
	  * with another mention. Used when merging DocTheory objects as 
	  * well. */
	virtual boost::shared_ptr<ActorMention> copyWithNewEntityMention(const SentenceTheory* sentTheory, const Mention* mention, const wchar_t *note);

	// State file serialization (not currently implemented)
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	virtual void resolvePointers(StateLoader * stateLoader);

	// SerifXML serialization/deserialization:
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual const wchar_t* XMLIdentifierPrefix() const { return L"actor"; }
	static boost::shared_ptr<ActorMention> loadXML(SerifXML::XMLTheoryElement elem);

	virtual void dump(std::wostream &stream);

	void setSourceNote(Symbol sourceNote) { _sourceNote = sourceNote; }
	
protected:
	ActorMention(const SentenceTheory *sentTheory, const Mention *mention, Symbol sourceNote);
	explicit ActorMention(SerifXML::XMLTheoryElement elem);

	/** The sentTheory that contains this actor mention.  Not owned.*/
	const SentenceTheory *_sentTheory;
	/** The entity mention's UID from the _sentTheory */
	MentionUID _mention_uid;

	/** Info about how this actor mention was generated */
	Symbol _sourceNote;
	
};
typedef boost::shared_ptr<ActorMention> ActorMention_ptr;


/** A subclass of ActorMention used for actor mentions that correspond with
  * a single named actor in the external actor database.   An example of
  * a named actor is "United Nations." 
  */
class ProperNounActorMention: public virtual ActorMention {
public:
	/** Each of these scores have their own scale */
	struct ActorMatchScores {
		double pattern_match_score;
		double pattern_confidence_score;
		double association_score;
		double edit_distance_score;
		double georesolution_score;
		double importance_score;
		ActorMatchScores() : pattern_match_score(0.0), pattern_confidence_score(0.0), association_score(0.0), edit_distance_score(0.0), georesolution_score(0.0), importance_score(0.0) {}
	};

	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(ProperNounActorMention, const SentenceTheory*, const Mention*, const Symbol&, const ActorMention::ActorIdentifiers&);
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(ProperNounActorMention, SentenceTheory*, const Mention*, const Symbol&, const ActorMention::ActorIdentifiers&);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(ProperNounActorMention, SerifXML::XMLTheoryElement);
	BOOST_MAKE_SHARED_5ARG_CONSTRUCTOR(ProperNounActorMention, const SentenceTheory*, const Mention*, const Symbol&, const ActorMention::ActorIdentifiers&, const Gazetteer::GeoResolution_ptr);

	/** Return a foreign key into the dict_actors table, identifying the 
	  * named actor who is identified by this ActorMention. */
	ActorId getActorId() const { return _actor.id; }

	/** Return the "unique_code" value for the named actor who is identified
	  * by this ActorMention (as defined in the rcdr.dict_actors table). */
	Symbol getActorCode() const { return _actor.code; }

	Symbol getCountryIsoCode(ActorInfo_ptr actorInfo) const;

	/** Return a foreign key into the dict_actorpatterns table, identifying
	  * the Jabari-style pattern that was used to match the named actor who
	  * is identified by this ActorMention. */
	ActorPatternId getActorPatternId() const { return _actor.patternId; }

	virtual ActorMention_ptr copyWithNewEntityMention(const SentenceTheory* sentTheory, const Mention* mention, const wchar_t *note);

	// serialization
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual void dump(std::wostream &stream);

	bool isResolvedGeo() const;
	bool isNamedLocation() const;
	void setGeoResolution(Gazetteer::GeoResolution_ptr geo);
	Gazetteer::GeoResolution_ptr getGeoResolution() const { return _geoResolution; }
	size_t getGeoPopulation() const;

	void setPatternMatchScore(double score);
	void setPatternConfidenceScore(double score);
	void setAssociationScore(double score);
	void setEditDistanceScore(double score);
	void setGeoresolutionScore(double score);
	void setImportanceScore(double score);

	void copyScores(boost::shared_ptr<ProperNounActorMention> pnam);

	double getPatternMatchScore();
	double getPatternConfidenceScore();
	double getAssociationScore();
	double getEditDistanceScore();
	double getGeoresolutionScore();
	double getImportanceScore();

	bool isAcronym() { return _actor.is_acronym; }
	bool requiresContext() { return _actor.requires_context; }
	std::wstring getActorName() { return _actor.actorName; }

	ActorIdentifiers getActorIdentifiers() { return _actor; }

protected:
	ActorIdentifiers _actor;
	Gazetteer::GeoResolution_ptr _geoResolution;
	ProperNounActorMention(const SentenceTheory *sentTheory, const Mention *mention, Symbol sourceNote, const ActorIdentifiers& actorInfo)
		: ActorMention(sentTheory, mention, sourceNote), _actor(actorInfo)
	{ 
		assert(!actorInfo.id.isNull());
		_geoResolution = boost::make_shared<Gazetteer::GeoResolution>();
	}
	ProperNounActorMention(const SentenceTheory *sentTheory, const Mention *mention, Symbol sourceNote, const ActorIdentifiers& actorInfo, const Gazetteer::GeoResolution_ptr geoResolution)
		: ActorMention(sentTheory, mention, sourceNote), _actor(actorInfo)
	{ 
		assert(!actorInfo.id.isNull());
		_geoResolution = geoResolution;
	}

	/** Info about how this match was made and confidence */
	ActorMatchScores _actorMatchScores;

	explicit ProperNounActorMention(SerifXML::XMLTheoryElement elem);
};
typedef boost::shared_ptr<ProperNounActorMention> ProperNounActorMention_ptr;


/** A subclass of ActorMention used for actor mentions that correspond with
  * an agent of a named actor in the external actor database.  An example of
  * an agent of a named actor is "an activist for the United Nations." 
  *
  * A "NULL" agent may be used in contexts where we have identified an agent,
  * but have not yet determined what named actor they are assocaited with.
  *
  * Each composite actor mention consists of two pieces: a named actor,
  * such as "United Nations", and an agent type, such as "activist".
  */
class CompositeActorMention: public virtual ActorMention {
public:
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(CompositeActorMention, const SentenceTheory*, const Mention*, const Symbol&,
		const ActorMention::AgentIdentifiers&, const ActorMention::ActorIdentifiers&, const Symbol&);
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(CompositeActorMention, const SentenceTheory*, const Mention*, const Symbol&,
		const ActorMention::AgentIdentifiers&, ProperNounActorMention_ptr, Symbol);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(CompositeActorMention, SerifXML::XMLTheoryElement);

	/** Return a foreign key into the dict_agents table, identifying what 
	  * type of agent is identified by this actor mention. */
	AgentId getPairedAgentId() const { return _pairedAgent.id; }

	/** Get canonical name for paired actor */
	std::wstring getPairedAgentName() { return _pairedAgent.agentName; }

	/** Return the "agent_code" value for the type of agent that is identified
	  * by this actor mention. */
	Symbol getPairedAgentCode() const { return _pairedAgent.code; }

	/** Return a foreign key into the dict_agentpatterns table, identifying
	  * the Jabari-style pattern that was used to match the agent-type that
	  * is used by this ActorMention. */ 
	AgentPatternId getPairedAgentPatternId() const { return _pairedAgent.patternId; }

	/** Return a foreign key into the dict_actors table, identifying the 
	  * named actor whose agent is identified by this ActorMention. */
	ActorId getPairedActorId() const { return _pairedActor.id; }

	/** Get canonical name for paired actor */
	std::wstring getPairedActorName() { return _pairedActor.actorName; }

	/** Return the "unique_code" value for the named actor whose agent is 
	  * identified by this ActorMention (as defined in the rcdr.dict_actors 
	  * table). */
	Symbol getPairedActorCode() const { return _pairedActor.code; }

	/** Return a foreign key into the dict_actorpatterns table, identifying
	  * the Jabari-style pattern that was used to match the named actor
	  * whose agent is identified by this ActorMention. */
	ActorPatternId getPairedActorPatternId() const { return _pairedActor.patternId; }

	/** Return the name of the pattern that was used to connect the agent 
	  * with its actor.  This could be a pattern name from the 
	  * icews_actor_agent_patterns file, or could be a special name from a 
	  * hard-coded pattern, such as "DEFAULT_COUNTRY_ACTOR" */
	Symbol getAgentActorPatternName() const { return _agentActorPatternName; }

	virtual ActorMention_ptr copyWithNewEntityMention(const SentenceTheory* sentTheory, const Mention* mention, const wchar_t *note);

	void setPairedActorMention(ProperNounActorMention_ptr pairedActor, const wchar_t *note);
	void setPairedActorIdentifiers(const ActorIdentifiers& pairedActorInfo, const wchar_t *note);

	ActorIdentifiers getPairedActorIdentifiers() { return _pairedActor; }

	// Serialization
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual void dump(std::wostream &stream);
protected:
	AgentIdentifiers _pairedAgent;
	ActorIdentifiers _pairedActor;
	Symbol _agentActorPatternName;
	CompositeActorMention(const SentenceTheory *sentTheory, const Mention *mention, Symbol sourceNote,
	                   const AgentIdentifiers &pairedAgentInfo, const ActorIdentifiers& pairedActorInfo, Symbol agentActorPatternName) 
		: ActorMention(sentTheory, mention, sourceNote), _pairedAgent(pairedAgentInfo), _pairedActor(pairedActorInfo),
	      _agentActorPatternName(agentActorPatternName)
	{ assert(!pairedAgentInfo.id.isNull()); }
	CompositeActorMention(const SentenceTheory *sentTheory, const Mention *mention, Symbol sourceNote,
	                   const AgentIdentifiers &pairedAgentInfo, ProperNounActorMention_ptr pairedActor, Symbol agentActorPatternName)
		   : ActorMention(sentTheory, mention, sourceNote), _pairedAgent(pairedAgentInfo), _agentActorPatternName(agentActorPatternName) 
	{
		assert(!pairedAgentInfo.id.isNull());
		if (pairedActor) {
			_pairedActor.id = pairedActor->getActorId();
			_pairedActor.actorName = pairedActor->getActorName();
			_pairedActor.code = pairedActor->getActorCode();
			_pairedActor.patternId = pairedActor->getActorPatternId();
		}
	}

	explicit CompositeActorMention(SerifXML::XMLTheoryElement elem);
};
typedef boost::shared_ptr<CompositeActorMention> CompositeActorMention_ptr;

std::wostream & operator << ( std::wostream &stream, ActorMention_ptr actorMention );
std::ostream & operator << ( std::ostream &stream, ActorMention_ptr actorMention );
SessionLogger::LogMessageMaker & operator << ( SessionLogger::LogMessageMaker &stream, ActorMention_ptr actorMention );

#endif
