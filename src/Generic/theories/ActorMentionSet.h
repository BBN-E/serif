// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ACTOR_MENTION_SET_H
#define ACTOR_MENTION_SET_H

#include "Generic/theories/SentenceSubtheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/ActorMention.h"
#include "Generic/actors/ActorInfo.h"
#include "Generic/common/hash_map.h"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/unordered_map.hpp>

// Forward declarations
class PatternMatcher; 
typedef boost::shared_ptr<PatternMatcher> PatternMatcher_ptr;

/** A document-level SERIF theory used to hold the set of ActorMentions that
  * have been identified in a given document.  The ActorMentionSet class
  * keeps track of the mapping between entity mentions and ActorMentions, and 
  * ensures that at most one ActorMention is included for any given entity 
  * mention.
  * 
  * ActorMentionSets are meant to be accessed via raw pointers (not shared_ptr); 
  * however, the individual ActorMention's contained in the ActorMentionSet are 
  * accessed via shared_ptrs (ActorMention_ptr).  ActorMentionSets are not
  * copyable.
  *
  * ActorMentionSet supports two access patterns: iteration (via standard
  * forward-traversal iterators), and Mention::UID-based lookup.
  */
class ActorMentionSet : public SentenceSubtheory, private boost::noncopyable {
public:	
	/** Construct a new empty ActorMentionSet. */
	ActorMentionSet();
	~ActorMentionSet();

	/** Used when merging ActorMentionSets due to a document split */
	ActorMentionSet(
		const std::vector<ActorMentionSet*> splitActorMentionSets, 
		const std::vector<int> sentenceOffsets, 
		const std::vector<MentionSet*> mergedMentionSets,
		const std::vector<SentenceTheory*> mergedSentenceTheories, 
		boost::unordered_map<ActorMention_ptr, ActorMention_ptr> &actorMentionMap);

	/** Add an actor mention to this set. If this actor mention set already
	  * contains an ActorMention whose entity mention is equal to the new
	  * ActorMention's entity mention, then the old ActorMention will be 
	  * replaced by the new ActorMention. */
	void addActorMention(ActorMention_ptr mention);

	/** Add an actor mention to this set. Same as addActorMention() but allows
	  * multiple ActorMentions per Mention */
	void appendActorMention(ActorMention_ptr mention);

	void discardActorMention(ActorMention_ptr mention);

	class const_iterator; // defined below.
	typedef const_iterator iterator;

	/** Return an iterator pointing to the first actor mention vector in this set. */
	const_iterator begin() const { return const_iterator(_actor_mentions.begin()); }

	/** Return an iterator pointing just past the last actor mention vector in this set. */
	const_iterator end() const { return const_iterator(_actor_mentions.end()); }

	/** Return the ActorMention for the given mention (or a "null" shared_ptr, if
	  * no ActorMention with the specified Mention::UID is in this set). Throws
	  * an exception if there are multiple entries for this mention UID. */
	ActorMention_ptr find(MentionUID mention_uid) const;

	/** Rerun all the ActorMentions for the given mention */
	std::vector<ActorMention_ptr> findAll(MentionUID mention_uid) const;

	/** Return a vector containing all ActorMentions in this set */
	std::vector<ActorMention_ptr> getAll() const;

	/** Return the number of ActorMentions contained in this set */
	size_t size() const { return _actor_mentions.size(); }

	/** Add "entity labels" to the given pattern matcher, idenifying each 
	  * ActorMention in this ActorMentionSet, as well as the sectors that 
	  * they are associated with. */
	void addEntityLabels(PatternMatcher_ptr matcher, ActorInfo_ptr actorInfo) const;

	/** Set sentence theory of all the ActorMentions. Needed for
	  * sentence level processing, as SentenceTheorys get deleted */
	void setSentenceTheories(const SentenceTheory *theory);

	// State file serialization (not currently implemented)
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	virtual void resolvePointers(StateLoader * stateLoader);

	// SerifXML serialization/deserialization:
	explicit ActorMentionSet(SerifXML::XMLTheoryElement elem, const DocTheory* theory=0);
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	void resolvePointers(SerifXML::XMLTheoryElement elem);
	virtual const wchar_t* XMLIdentifierPrefix() const { return L"actors"; }

	/** HashMap type used to store the ActorMentions */
	typedef serif::hash_map<MentionUID, std::vector<ActorMention_ptr>, MentionUID::HashOp, MentionUID::EqualOp> ActorMentionMap;

	virtual SentenceTheory::SubtheoryType getSubtheoryType() const
	{	return SentenceTheory::ACTOR_MENTION_SET_SUBTHEORY; }

	/** Iterator type for this set. */
	class const_iterator: public boost::iterator_facade<const_iterator, std::vector<ActorMention_ptr> const, boost::forward_traversal_tag> {
	private:
		friend class boost::iterator_core_access;
		friend class ActorMentionSet;
		explicit const_iterator(ActorMentionMap::const_iterator it): _base(it) {}
		void increment() { ++_base; }
		bool equal(const_iterator const& other) const { return _base==other._base; }
		std::vector<ActorMention_ptr> const& dereference() const { return (*_base).second; }
		ActorMentionMap::const_iterator _base;
	};

private:
	ActorMentionMap _actor_mentions;
};

#endif
