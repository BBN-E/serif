// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_EVENT_MENTION_SET_H
#define ICEWS_EVENT_MENTION_SET_H

#include "Generic/theories/Theory.h"
#include "Generic/theories/DocTheory.h"
#include "ICEWS/EventMention.h"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace ICEWS {

/** A document-level SERIF theory used to hold the set of ICEWSEventMentions
  * that have been identified in a given document.  
  * 
  * ICEWSEventMentionSets are meant to be accessed via raw pointers (not 
  * shared_ptr); however, the individual ICEWSEventMentions's contained in 
  * the ICEWSEventMentionSets are accessed via shared_ptrs 
  * (ICEWSEventMentions_ptr).  ICEWSEventMentionSets are not copyable. */
class ICEWSEventMentionSet : public Theory, private boost::noncopyable {
public:	
	ICEWSEventMentionSet();
	~ICEWSEventMentionSet();

	/** Add an event mention to this set. */
	void addEventMention(ICEWSEventMention_ptr emention);

	void removeEventMentions(std::set<ICEWSEventMention_ptr> ementions);

	// Iterator type.
	typedef std::vector<ICEWSEventMention_ptr>::const_iterator const_iterator;
	typedef std::vector<ICEWSEventMention_ptr>::iterator iterator;

	/** Return an iterator pointing to the first event mention in this set. */
	const_iterator begin() const { return _event_mentions.begin(); }
	iterator begin() { return _event_mentions.begin(); }

	/** Return an iterator pointing just past the last event mention in this set. */
	const_iterator end() const { return _event_mentions.end(); }
	iterator end() { return _event_mentions.end(); }

	/** Return the number of event mentions in this set. */
	size_t size() const { return _event_mentions.size(); }

	// State file serialization (not currently implemented)
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	virtual void resolvePointers(StateLoader * stateLoader);

	// SerifXML serialization/deserialization:
	explicit ICEWSEventMentionSet(SerifXML::XMLTheoryElement elem, const DocTheory* theory=0);
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual const wchar_t* XMLIdentifierPrefix() const { return L"icewseventset"; }

private:
	std::vector<ICEWSEventMention_ptr> _event_mentions;
};

} // end of ICEWS namespace

#endif
