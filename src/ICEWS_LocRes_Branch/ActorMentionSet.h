// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_ACTOR_MENTION_SET_H
#define ICEWS_ACTOR_MENTION_SET_H

#include "Generic/theories/Theory.h"
#include "Generic/theories/DocTheory.h"
#include "ICEWS/EventMention.h"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/iterator/iterator_facade.hpp>

// Forward declarations
namespace ICEWS { class ActorInfo; }
class PatternMatcher; 
typedef boost::shared_ptr<PatternMatcher> PatternMatcher_ptr;

namespace ICEWS {

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
class ActorMentionSet : public Theory, private boost::noncopyable {
public:	
	/** Construct a new empty ActorMentionSet. */
	ActorMentionSet();
	~ActorMentionSet();

	/** Add an actor mention to this set.  If this actor mention set already
	  * contains an ActorMention whose entity mention is equal to the new
	  * ActorMention's entity mention, then the old ActorMention will be 
	  * replaced by the new ActorMention. */
	void addActorMention(ActorMention_ptr mention);

	void discardActorMention(ActorMention_ptr mention);

	class const_iterator; // defined below.
	typedef const_iterator iterator;

	/** Return an iterator pointing to the first actor mention in this set. */
	const_iterator begin() const { return const_iterator(_actor_mentions.begin()); }

	/** Return an iterator pointing just past the last actor mention in this set. */
	const_iterator end() const { return const_iterator(_actor_mentions.end()); }

	/** Return the ActorMention for the given mention (or a "null" shared_ptr, if
	  * no ActorMention with the specified Mention::UID is in this set). */
	ActorMention_ptr find(MentionUID mention_uid) const;

	/** Return the number of ActorMentions contained in this set */
	size_t size() const { return _actor_mentions.size(); }

	/** Add "entity labels" to the given pattern matcher, idenifying each 
	  * ActorMention in this ActorMentionSet, as well as the sectors that 
	  * they are associated with. */
	void addEntityLabels(PatternMatcher_ptr matcher, ActorInfo &actorInfo) const;

	// State file serialization (not currently implemented)
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	virtual void resolvePointers(StateLoader * stateLoader);

	// SerifXML serialization/deserialization:
	explicit ActorMentionSet(SerifXML::XMLTheoryElement elem, const DocTheory* theory=0);
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual const wchar_t* XMLIdentifierPrefix() const { return L"icewsactors"; }

	/** HashMap type used to store the ActorMentions */
	typedef hash_map<MentionUID, ActorMention_ptr, MentionUID::HashOp, MentionUID::EqualOp> ActorMentionMap;

	/** Iterator type for this set. */
	class const_iterator: public boost::iterator_facade<const_iterator, ActorMention_ptr const, boost::forward_traversal_tag> {
	private:
		friend class boost::iterator_core_access;
		friend class ActorMentionSet;
		explicit const_iterator(ActorMentionMap::const_iterator it): _base(it) {}
		void increment() { ++_base; }
		bool equal(const_iterator const& other) const { return _base==other._base; }
		ActorMention_ptr const& dereference() const { return (*_base).second; }
		ActorMentionMap::const_iterator _base;
	};
private:
	ActorMentionMap _actor_mentions;
};

} // end of ICEWS namespace

#endif
