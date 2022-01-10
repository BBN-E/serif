// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENTITY_SET_H
#define ENTITY_SET_H

#include "Generic/common/GrowableArray.h"
#include "Generic/common/DebugStream.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/SentenceSubtheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/EntitySubtype.h"
#include "Generic/common/hash_map.h"
#include <boost/shared_ptr.hpp>

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;

#include <iostream>

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED EntitySet : public SentenceSubtheory {
public:
	EntitySet(int nSentences = 0);
	EntitySet(const EntitySet &other, bool copyMentionSet = true); // copy constructor
	EntitySet(const std::vector<EntitySet*> splitEntitySets, const std::vector<int> sentenceOffsets, const std::vector<MentionSet*> mergedMentionSets);
	//EntitySet(const EntitySet *oldEntitySet, const MentionSet *newMentionSet);
	virtual ~EntitySet();

	virtual SentenceTheory::SubtheoryType getSubtheoryType() const
	{ return SentenceTheory::ENTITY_SUBTHEORY; }

	void setScore(float score) { _score = score; }
	float getScore() const { return _score; }

	Entity *getEntity(int i) const;
	void loadMentionSet(const MentionSet *newSet);
	void loadDoNotCopyMentionSet(MentionSet *newSet);
	/**
	 * Get the entity associated with a mention
	 * get 0 if the mention isn't found in any entity in this set
	 *
	 * Note: if metonymy is active, this method may not always 
	 * behave as expected - i.e. the entity associated with the
	 * literal type of the mention will always be returned. In 
	 * the case where you want the entity associated with the
	 * intended mention, use either the version of this method
	 * that takes an entity type parameter or 
	 * getIntendedEntityByMention().
	 *
	 * @param ment The mention whose entity we want
	 * @return the associated entity, or 0 if none is found
	 */
	Entity* getEntityByMentionWithoutType(MentionUID uid) const;
	Entity* getEntityByMention(MentionUID uid) const;
	/**
	 * Get the entity of the given type associated with a mention
	 * get 0 if the mention isn't found in any entity of that type
	 * in this set.
	 *
	 * @param ment The mention whose entity we want
	 * @param type The type of entity we're looking for
	 * @return the associated entity, or 0 if none is found
	 */
	Entity* getEntityByMention(MentionUID uid, EntityType type) const;
	/**
	 * Get the intended entity associated with a mention
	 * get 0 if the mention isn't found in any entity in this set
	 *
	 * @param ment The mention whose intended entity we want
	 * @return the associated entity, or 0 if none is found
	 */
	Entity* getIntendedEntityByMention(MentionUID uid) const;
	int getNEntities() const ;
	int getNEntitiesByType(EntityType type) const;
	// number of mention sets imply number of active sentences
	int getNMentionSets() const;
	Mention *getMention(MentionUID uid) const;
	//EntitySet * fork();
	const GrowableArray <Entity *> &getEntities() const;
	const GrowableArray <Entity *> &getEntitiesByType(EntityType type) const;
	virtual void addNew(MentionUID uid, EntityType type); 
	virtual void add(MentionUID uid, int entityID);

	const MentionSet *getLastMentionSet() const { return _currMentionSet; }
	MentionSet *getNonConstLastMentionSet() const { return _currMentionSet; }
	
	MentionSet *getNonConstMentionSet(int i) const;
	const MentionSet *getMentionSet(int i) const;

	void customDebugPrint(DebugStream &out);
	void customDebugPrintWcout();
	/**
	 * Get a guess about the subtype for this entity given the subtype assigned
	 * to its mentions.  Guess prefers the subtype assinged to a name, to that of a desc, to that of a pron.
	 * Mentions are assigned a subtype during descriptor classifcation.  Currently this is rule based (using a list).  
	 * Mentions that can not be classified default to UNDET
	 * Warning: This will return UNDET if none of the mentions has a subtype!
	 *
	 * @param ent The entity whose subtype is in question
	 * @return a guessed subtype, possibly UNDET
	 **/
	EntitySubtype guessEntitySubtype(const Entity *entity) const;

	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out,
									 const EntitySet &it)
		{ it.dump(out, 0); return out; }

	// Called on the document-level entity set after it's populated, so we can cache mention confidences
	void determineMentionConfidences(DocTheory* docTheory);

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	EntitySet(StateLoader *stateLoader);
	static bool FakeEntitySet(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	void loadMentionSets();
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit EntitySet(SerifXML::XMLTheoryElement elem, const std::vector<MentionSet*> &sentenceMentionSets);
	const wchar_t* XMLIdentifierPrefix() const;

	
protected:

	GrowableArray <Entity *> _entities;
	GrowableArray <Entity *>* _entitiesByType;
	// one more for undet entities
	GrowableArray <Entity *> _undetEntities;

	MentionSet **_prevMentionSets;
	MentionSet *_currMentionSet;
	int _nPrevMentionSets, _nSentences;
	// Unfortunately sometimes we have ownership of MentionSets and sometimes we don't
	// This boolean and array keep track of which ones we own
	bool _currMentionSetOwned;
	std::vector<bool> _prevMentionSetsOwned;
	float _score;

	static DebugStream &_debugOut;

private:
	bool isEmpty() const {
		return (_nSentences == 0);
	}
	std::map<MentionUID, Entity*> _entityByMention;
public:
	/** 
	//Mentions can be moved from entity-to-entity, which makes truly lazy caching unsafe.
	//Instead, allow on-demand initialization of _entityByMention.
	//If the assignment of mentions to entities changes after populateEntityByMentionTable has been called,
	// clearEntityByMentionTable() MUST be called
	**/
	void populateEntityByMentionTable();
	void clearEntityByMentionTable();
	
	/** Return the entity associated with a mention.  
	 *	This is equivalent
	 * to calling getDocTheory()->getEntitySet()->getEntityByMention(uid),
	 * except that it is more efficient when called multiple times
	 * (because it (lazily) builds a cache of the map from mentions to entities,
	 * rather than scanning the list of entities each time. 
	 *
	 * Note: As with if getEntityByMentionWithoutType(). That is it returns the entity
	 * with the original (not Metonymic) type.  
	 *
	 **/	
	
	const Entity* lookUpEntityForMention(MentionUID uid);
};



#endif
