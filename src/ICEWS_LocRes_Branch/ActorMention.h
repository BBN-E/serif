// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_ACTOR_MENTION_H
#define ICEWS_ACTOR_MENTION_H

#include "Generic/theories/Theory.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SessionLogger.h"
#include "ICEWS/Identifiers.h"
#include "ICEWS/Gazetteer.h"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "Generic/common/BoostUtil.h"
#include <iosfwd>

// Forward declarations:
class SentenceTheory;
class Mention;
class DocTheory;

namespace ICEWS {

/** A SERIF theory used to identify a single mention of an "ICEWS actor."  
  * Each ActorMention corresponds with a single entity mention (i.e., Mention 
  * object), and can optionally be labeled with ICEWSIdentifiers that link it 
  * to an external database of actors.
  *
  * The base class simply wraps a Mention object.  Subclasses are used
  * to add pointers into the external database of actors.  Two different
  * subclasses are defined: ProperNounActorMention is used for named
  * actors, and CompositeActorMention is used for "agents of named
  * actors," such as "an activist for the Georgian Party".
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
		explicit ActorIdentifiers(ActorId id=ActorId(), Symbol code=Symbol(), ActorPatternId patternId=ActorPatternId()): id(id), code(code), patternId(patternId) {}
	};
	struct AgentIdentifiers {
		AgentId id;
		Symbol code;
		AgentPatternId patternId;
		explicit AgentIdentifiers(AgentId id=AgentId(), Symbol code=Symbol(), AgentPatternId patternId=AgentPatternId()): id(id), code(code), patternId(patternId) {}
	};

	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(ActorMention, const SentenceTheory*, const Mention*, const Symbol&);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(ActorMention, SerifXML::XMLTheoryElement);
	virtual ~ActorMention() {}

	/** Return the entity mention that corresponds with this actor. */
	const Mention* getEntityMention() const { return _mention; }

	/** Return the SentenceTheory for the sentence that contains this mention. */
	const SentenceTheory* getSentence() const { return _sentTheory; }

	/** Return the "source note", which records how/where this actor was generated. */
	Symbol getSourceNote() const { return _sourceNote; }

	void addSourceNote(const wchar_t *note);
	void addSourceNote(const Symbol &note) { addSourceNote(note.to_string()); }

	/** Return a copy of this ActorMention (pointing at the same external
	  * database actor, if applicable) with a new entity mention.  This is
	  * used when labeling a mention based on the fact that is coreferent
	  * with another mention. */
	virtual boost::shared_ptr<ActorMention> copyWithNewEntityMention(const SentenceTheory* sentTheory, const Mention* mention, const wchar_t *note);

	/** Determine the ICEWS sentence number for the sentence containing
	  * this actor mention.  Note that we use our own sentence
	  * segmentation, so this is *not* the same as the Serif sentence
	  * number.  It is looked up using SentenceSpan fields in the 
	  * document's metadata.  (This method will raise an exception if 
	  * no SentenceSpan data is included in the metadata.) */
	size_t getIcewsSentNo(const DocTheory* docTheory) const;

	// State file serialization (not currently implemented)
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	virtual void resolvePointers(StateLoader * stateLoader);

	// SerifXML serialization/deserialization:
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual const wchar_t* XMLIdentifierPrefix() const { return L"icewsactor"; }
	static boost::shared_ptr<ActorMention> loadXML(SerifXML::XMLTheoryElement elem);

	virtual void dump(std::wostream &stream);
protected:
	ActorMention(const SentenceTheory *sentTheory, const Mention *mention, Symbol sourceNote)
		: _sentTheory(sentTheory), _mention(mention), _sourceNote(sourceNote) {}
	explicit ActorMention(SerifXML::XMLTheoryElement elem);
private:

	/** The sentTheory that contains this actor mention.  Not owned.*/
	const SentenceTheory* _sentTheory;
	/** The entity mention corresponding to this actor.  Not owned.*/
	const Mention* _mention;
	/** Info about how this actor mention was generated */
	Symbol _sourceNote;
protected:
	void setSourceNote(Symbol sourceNote) { _sourceNote = sourceNote; }
};
typedef boost::shared_ptr<ActorMention> ActorMention_ptr;


/** A subclass of ActorMention used for actor mentions that correspond with
  * a single named actor in the external actor database.   An example of
  * a named actor is "the Georgian Party." 
  */
class ProperNounActorMention: public ActorMention {
public:
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(ProperNounActorMention, const SentenceTheory*, const Mention*, const Symbol&, const ICEWS::ActorMention::ActorIdentifiers&);
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(ProperNounActorMention, SentenceTheory*, const Mention*, const Symbol&, const ICEWS::ActorMention::ActorIdentifiers&);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(ProperNounActorMention, SerifXML::XMLTheoryElement);
	BOOST_MAKE_SHARED_5ARG_CONSTRUCTOR(ProperNounActorMention, const SentenceTheory*, const Mention*, const Symbol&, const ICEWS::ActorMention::ActorIdentifiers&, const ICEWS::Gazetteer::GeoResolution_ptr);

	/** Return a foreign key into the dict_actors table, identifying the 
	  * named actor who is identified by this ActorMention. */
	ActorId getActorId() const { return _actor.id; }

	/** Return the "unique_code" value for the named actor who is identified
	  * by this ActorMention (as defined in the rcdr.dict_actors table). */
	Symbol getActorCode() const { return _actor.code; }

	Symbol getCountryIsoCode() const;

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

private:
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

	explicit ProperNounActorMention(SerifXML::XMLTheoryElement elem);
};
typedef boost::shared_ptr<ProperNounActorMention> ProperNounActorMention_ptr;


/** A subclass of ActorMention used for actor mentions that correspond with
  * an agent of a named actor in the external actor database.  An example of
  * an agent of a named actor is "an activist for the Georgian Party." 
  *
  * A "NULL" agent may be used in contexts where we have identified an agent,
  * but have not yet determined what named actor they are assocaited with.
  *
  * Each composite actor mention consists of two pieces: a named actor,
  * such as "the Georgian Party", and an agent type, such as "activist".
  */
class CompositeActorMention: public ActorMention {
public:
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(CompositeActorMention, const SentenceTheory*, const Mention*, const Symbol&,
		const ICEWS::ActorMention::AgentIdentifiers&, const ICEWS::ActorMention::ActorIdentifiers&, const Symbol&);
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(CompositeActorMention, const SentenceTheory*, const Mention*, const Symbol&,
		const ICEWS::ActorMention::AgentIdentifiers&, ICEWS::ProperNounActorMention_ptr, Symbol);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(CompositeActorMention, SerifXML::XMLTheoryElement);

	/** Return a foreign key into the dict_agents table, identifying what 
	  * type of agent is identified by this actor mention. */
	AgentId getPairedAgentId() const { return _pairedAgent.id; }

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

	// Serialization
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual void dump(std::wostream &stream);
private:
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
			_pairedActor.code = pairedActor->getActorCode();
			_pairedActor.patternId = pairedActor->getActorPatternId();
		}
	}

	explicit CompositeActorMention(SerifXML::XMLTheoryElement elem);
};
typedef boost::shared_ptr<CompositeActorMention> CompositeActorMention_ptr;

} // end namespace ICEWS

std::wostream & operator << ( std::wostream &stream, ICEWS::ActorMention_ptr actorMention );
std::ostream & operator << ( std::ostream &stream, ICEWS::ActorMention_ptr actorMention );
SessionLogger::LogMessageMaker & operator << ( SessionLogger::LogMessageMaker &stream, ICEWS::ActorMention_ptr actorMention );

#endif
