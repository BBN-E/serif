// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SENTENCE_SUBTHEORY_H
#define SENTENCE_SUBTHEORY_H

#include "Generic/theories/Theory.h"
#include "Generic/theories/SentenceTheory.h"


#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

/** SentenceSubtheory is a sort-of self-contained piece of a sentence
  * theory. The different types of subtheories loosely correspond to 
  * Serif stages. The types are enumerated in SentenceTheory.h.
  */

class SERIF_EXPORTED SentenceSubtheory : public Theory {
public:
	SentenceSubtheory() : _refcount(0) {}
	virtual ~SentenceSubtheory() {}


	virtual SentenceTheory::SubtheoryType getSubtheoryType() const = 0;


	/** Reference counting methods. These should only be used by
	  * SentenceTheory and SentenceTheoryBeam. Everyone else should just
	  * treat SentenceSubtheorys as ordinary non-refcounted objects
	  * unless there is some compelling reason to introduce refcounting
	  * to some other part of the program. */
	void gainReference() const {
		const_cast<SentenceSubtheory*>(this)->_refcount++;
	}
	void loseReference() const {
		if (_refcount <= 0)
			throw InternalInconsistencyException("SentenceSubtheory::loseReference",
												 "Invalid reference count");
		const_cast<SentenceSubtheory*>(this)->_refcount--;
		if (_refcount == 0)
			delete this;
	}

	bool hasReference() const { return _refcount > 0; }

	// For saving state:
	virtual void updateObjectIDTable() const = 0;
	virtual void saveState(class StateSaver *stateSaver) const = 0;
	// For loading state:
	virtual void resolvePointers(StateLoader * stateLoader) = 0;
	// For XML serialization:
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const = 0;
	const wchar_t* XMLIdentifierPrefix() const = 0;

private:
	int _refcount;
};

#endif
