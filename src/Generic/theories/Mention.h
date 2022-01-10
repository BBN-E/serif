// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTION_H
#define MENTION_H

#include "Generic/theories/Theory.h"
#include "Generic/common/limits.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/EntitySubtype.h"
#include "Generic/common/Attribute.h"
#include "Generic/theories/SentenceItemIdentifier.h"
#include <boost/functional/hash.hpp>

#include <iostream>

class MentionSet;
class SynNode;
class Entity;
class DocTheory;
class SentenceTheory;
class PropositionSet;

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;
namespace SerifXML { class XMLTheoryElement; }

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED Mention : public Theory {
public:
	// Document-level identifiers for mentions.
	typedef MentionUID UID;

	typedef enum {NONE = 0,
				  NAME,
				  PRON,
				  DESC,
				  PART,
				  APPO,
				  LIST,
				  INFL,
				  NEST}
		Type;

	static int getIndexFromUID(MentionUID uid);
	static int getSentenceNumberFromUID(MentionUID uid);

	static const char *getTypeString(Type type);
	static const Mention::Type getTypeFromString(const char *typeString);
	static const Mention::Type getTypeFromString(const wchar_t *typeString);
private:
	static const size_t N_TYPE_STRINGS;
	static const char *TYPE_STRINGS[];
	static const wchar_t *TYPE_WSTRINGS[];
	static MentionUID makeUID(int sentence, int index);

	MentionUID _uid;
	EntityType entityType;
	EntitySubtype entitySubtype;

public:
	const SynNode *node;

	//int nWords; // JJO 08 Aug 2011  - word inclusion desc. linking

	// Note: we intentionally declare mentionType adjacent to the int
	// fields _xyz_index, to give better memory packing (on 64-bit builds).
	Type mentionType;
private:
	// semantic structure information for part/appo/list mentions
	// and nested-name mentions:
	int _parent_index;
	int _child_index;
	int _next_index;

	MentionSet *_mentionSet;

	EntityType _intendedType;
	EntityType _role;
	bool _is_metonymy_mention;

	float _confidence;
	float _link_confidence;

public:
	Mention();
	Mention(MentionSet *mentionSet, int index,
					const SynNode *node);

	/** A sort of copy-ctor, except that it needs the mentionSet to 
	  * initialize properly */
	Mention(MentionSet *mentionSet, const Mention &source);

	bool isPopulated() const { return mentionType != NONE; }
	bool isOfRecognizedType() const {
		return entityType.isRecognized();
	}

	void setIndex(int index);

	int getSentenceNumber() const;
	int getIndex() const;
	const MentionSet *getMentionSet() const { return _mentionSet; };
	MentionUID getUID() const { return _uid; }
	void setUID(MentionUID uid) { _uid = uid; }
	inline Type getMentionType() const { return mentionType; }
	inline EntityType getEntityType() const { return entityType; }
	void setEntityType(EntityType etype);
	/**
	 * Get the EntitySubtype for this mention. 
	 * Mentions are assigned a subtype during descriptor classifcation.  Currently this is rule based (using a list).  
	 * Mentions that can not be classified default to UNDET. Entities do not ever formally receive a subtype, but rather
	 * the EntitySet::guessEntitySubtype() function is used during APF printing to choose a subtype for the whole entity
	 *
	 * @return a  subtype, UNDET if the rule based classifier didn't know what to do
	**/
	inline EntitySubtype getEntitySubtype() const { return entitySubtype; }
	void setEntitySubtype(EntitySubtype nsubtype);
	const SynNode *getNode() const { return node; }
	
	//int getNWords() { return nWords; } // JJO 08 Aug 2011 - word inclusion desc. linking

	bool hasApposRelationship() const;
	bool isPartOfAppositive() const;

	bool hasIntendedType() const;
	void setIntendedType(EntityType type) { _intendedType = type; }
	EntityType getIntendedType() const { return _intendedType; }

	bool hasRoleType() const;
	void setRoleType(EntityType type) { _role = type; }
	EntityType getRoleType() const { return _role; }

	/** This gets the mention-relevant head word (as opposed to the simple
	  * syntactic head word). In particular, for name mentions, it returns
	  * the name itself, even if the mention is structured in some way.
	  *
	  * (For appositives and lists, its behavior is someone more arbitrary:
	  * it returns the node of the first member mention of the structure.
	  * This is compatible with how ACE treats appositives.)
	  */
	const SynNode *getHead() const;

	/** The EDT head of a mention takes into account appositives and other
	  * structured types of mentions. Originally developed as the method
	  * for determining mention heads in APFResultCollector.
	  */
	const SynNode* getEDTHead() const;

	/** If this mention is a name, then return its name extent (NPP); 
	  * otherwise, return its syntactic head */
	const SynNode* getAtomicHead() const;

	MentionConfidenceAttribute brandyStyleConfidence(const DocTheory* dt, const SentenceTheory* st, std::set<Symbol>& ambiguousLastNames) const;

	// These are for part/appo/list/nested structure info
	Mention *getParent() const;
	Mention *getChild() const;
	Mention *getNext() const;

	void makeOnlyChildOf(Mention *parent);
	void makeNextSiblingOf(Mention *prevSib);
	void makeOrphan();

	Entity* getEntity(const DocTheory* docTheory) const;
	EntitySubtype guessEntitySubtype(const DocTheory *docTheory) const;

	void setMetonymyMention() { _is_metonymy_mention = true; }
	bool isMetonymyMention() const { return _is_metonymy_mention; }

	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out, const Mention &it)
		{ it.dump(out, 0); return out; }

	bool is1pPronoun() const;
	bool is2pPronoun() const;
	bool is3pPronoun() const;

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	void loadState(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit Mention(SerifXML::XMLTheoryElement elem, MentionSet *mentionSet, MentionUID uid);
	void resolvePointers(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;
	
	// TODO: save the confidence in state-files

	void setConfidence(float confidence) { _confidence = confidence; }
	float getConfidence() { return _confidence; }
	void setLinkConfidence(float conf) { _link_confidence = conf; }
	float getLinkConfidence() const { return _link_confidence; }

	// This was added for a sanity check in the serialization -- we want to 
	// make sure that two mention objects that are different objects actually
	// hold identical data.
	bool isIdenticalTo(const Mention& other) const;

	std::wstring toCasedTextString() const;
	std::wstring toAtomicCasedTextString() const;

	// Helper function for downstream processing
	const Mention *getMostRecentAntecedent(const DocTheory *docTheory) const;

	// Helper function for downstream processing
	// This may only make sense in English, but there is nothing technically
	//  language-specific about it.
	bool isBadNeutralPronoun(const MentionSet * mentionSet, PropositionSet * propSet) const;
};

template<typename StreamType>
StreamType& operator<<(StreamType &out, MentionUID uid) {
	//out << uid.sentno() << "." << uid.index(); return out;
	out << uid.toInt(); return out;
}

typedef std::pair<const Mention*, const Mention*> MentionPair;

#endif
