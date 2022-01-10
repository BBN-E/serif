// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENTITY_H
#define ENTITY_H

#include "Generic/theories/Theory.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/Attribute.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/DebugStream.h"
#include "Generic/edt/CountsTable.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/Mention.h"

#include <iostream>
#include <map>
#include <string>

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;

class DocTheory;
class Entity;
class EntitySet;
class Mention;
class MentionConfidence;

class EntityClutterFilter {
public:
	virtual ~EntityClutterFilter() {}
	virtual bool filtered (const Entity *, double *score = 0) const = 0;
};

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED Entity : public Theory {
public:
	/// this function is now deprecated. You should use
	/// EntityType::getName() instead.  -- SRS
	static const char *getTypeString(EntityType type);

protected:
	static DebugStream &_debugOut;

	// filterName & score
	std::map<std::string, double> _filters;

	std::map<MentionUID, MentionConfidenceAttribute > _mention_confidences;

	bool _is_generic;
	int _GUID;
	int _can_name_n_symbols; // Canonical name number of symbols
	Symbol * _canonical_name;
	
	//NOT_ASSIGNED means cross-doc has not given a GUID to this entity
    static const int NOT_ASSIGNED = -1;
	/*
	// +++ JJO 05 Aug 2011 +++
	// Word inclusion desc. linking
	GrowableArray<Symbol> _wordSet;
	int _nStopWords;
	Symbol *_stopWords;
	// --- JJO 05 Aug 2011 ---
	*/

public:

	int ID;
	EntityType type;
	GrowableArray <MentionUID> mentions;

//	Entity(Entity &other);
	Entity(int ID_, EntityType type_);
	~Entity();

	/*
	// +++ JJO 05 Aug 2011 +++
	// Word inclusion desc. linking
	bool isStopWord(Symbol word);
	bool hasWord(Symbol word);
	void addWords(Symbol *words, int nWords);
	// --- JJO 05 Aug 2011 ---
	*/

	int getID() const { return ID; }
	EntityType getType() const { return type; } 

	//GUID, as opposed to ID, is global across documents (it's assigned by cross-doc)
	int getGUID() const { return _GUID; }
	void assignGUID(int GUID) { _GUID = GUID; }
	bool hasGUID() const {return _GUID != NOT_ASSIGNED; }

	// Canonical name
	void getCanonicalName(int & n_symbols, Symbol * symbols);
	std::wstring getCanonicalNameOneWord(bool add_spaces=false) const;
	void setCanonicalName(int n_symbols, Symbol * symbols);

	void addMention(MentionUID uid);
	void removeMention(MentionUID uid);

	int getNMentions() const { return mentions.length(); }
	MentionUID getMention(int index) const { return mentions[index]; }
	MentionUID getMention(size_t index) const { return mentions[(int) index]; }

	MentionConfidenceAttribute getMentionConfidence(MentionUID uid) const;
	void setMentionConfidence(MentionUID uid, MentionConfidenceAttribute confidence) {
		_mention_confidences[uid] = confidence;
	}
	
	bool isPopulated() const { return ID != -1; }

	/** Return the "best name" for this entity.  The best name is currently
	  * defined as:
	  *
	  *   - The atomic head of the longest mention whose Mention::Type is 
	  *     NAME (where length is defined as number of characters), if any
	  *     such mention exists.
	  *
	  *   - Otherwise, the first mention whose type is Mention::NAME (where  
	  *     first means that its start offset preceeds all other mentions'  
	  *     start offsets), if any such mention exists.
	  * 
	  *   - Otherwise, the special string "NO_NAME". */
	std::wstring getBestName(const DocTheory* docTheory) const;
	std::pair<std::wstring, const Mention*> getBestNameWithSourceMention(const EntitySet *entitySet) const;
	std::pair<std::wstring, const Mention*> getBestNameWithSourceMention(const DocTheory *docTheory) const;

	/** Return true if this entity contains at least one name mention. */
	bool hasNameMention(const EntitySet* entitySet) const;

	/** Return true if this entity contains at least one description mention. */
	bool hasDescMention(const EntitySet* entitySet) const;

	/** Return true if this entity contains at least one name or description mention. */
	bool hasNameOrDescMention(const EntitySet* entitySet) const;

	EntitySubtype guessEntitySubtype(const DocTheory *docTheory) const;

	/** Currently, this always returns zero.  In principle, it should return the 
	  * maximum frequency of the names in this mention, as reported by the 
	  * ConfidentNameDictionary (Brandy/distill-generic/Query/ConfidentNameDictionary.h).
	  * However, the code this function replaces (DocumentInfo::setSlotNameFrequencies)
	  * was commented out and always returned zero.  So we do the same thing here. */
	int getNameFrequency(const EntitySet* entitySet) const { return 0; }

	/**
	 * generic filter interface
	 */
	void applyFilter (const std::string& filterName, EntityClutterFilter *filter);
	bool isFiltered (const std::string& filterName) const;
	double getFilterScore (const std::string& filterName) const;
	//const std::map<std::string, double>& getFilterMap() const { return _filters; }

	/**
	 * Has the generic flag been set?
	 * @return true if the entity was judged generic
	 */
	bool isGeneric() const { return _is_generic; }
	/**
	 * If the entity is thought to be generic, set it here
	 */
	void setGeneric() { _is_generic = true; }
	/**
	 * If the entity is thought to be specific (and by default), set it here
	 */
	void setSpecific() { _is_generic = false; }

	/** 
	  * Determines if this entity contains the specified mention 
	  */
	bool containsMention(const Mention* m) const;

	void dump(std::ostream &out, int indent = 0, const EntitySet* entitySet = 0) const;
	friend std::ostream &operator <<(std::ostream &out, Entity &it)
		{ it.dump(out, 0); return out; }


	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	Entity(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit Entity(SerifXML::XMLTheoryElement elem, int entity_id);
	const wchar_t* XMLIdentifierPrefix() const;
protected:
		enum {UNSET, YES, NO};
		int _hasName;
		int _hasDesc;
public:
		/****
		//Support caching of _hasName and _hasDesc to reduce time spent in hasNameMention(), 
		//hasDescMention(), hasNameOrDescMention()
		//Mentions can be moved from entity-to-entity, which makes truly lazy caching unsafe.
		//Instead, allow on-demand initialization of hasName/hasDesc variables
		//If the cache has been initialized, and an entity changes
		//clearHasNameDescCache() should also be called. This is implemented for addMention() and removeMention() 
		*/
		void initializeHasNameDescCache(const EntitySet* entitySet);
		void clearHasNameDescCache();
		

};

#endif
