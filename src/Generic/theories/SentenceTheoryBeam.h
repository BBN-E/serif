// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SENTENCE_THEORY_BEAM_H
#define SENTENCE_THEORY_BEAM_H

#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/Theory.h"

class DocTheory;
class Sentence;
class SentenceSubtheory;

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED SentenceTheoryBeam: public Theory {
public:
	SentenceTheoryBeam(const Sentence *sentence, size_t width=4);

	~SentenceTheoryBeam();

	/** Accessor for sentence */
	Sentence *getSentence() const;

	/** Modify the width of the beam -- not implemented yet. */
	void setBeamWidth(size_t width);

	/** Add a theory to the beam.
	  *
	  * The SentenceTheoryBeam adopts "ownership", meaning that it 
	  * is SOLELY responsibile for deleting the theory. Furthermore, the
	  * theory may be deleted at any time -- as soon as it falls off
	  * the end of the beam -- so do NOT refer to the theory after
	  * calling addTheory() on it.
	  */
	void addTheory(SentenceTheory *theory);

	/** Returns the number of theories in the beam (may be less than
	  * beam width).
	  */
	int getNTheories() const { return _n_theories; }

	/** Accessor for sentence theories in beam
	  */
	SentenceTheory *getTheory(int i) const;

	/** Return the highest-scoring theory in the beam (or NULL if there
	  * are currently no theories in the beam). */
	SentenceTheory *getBestTheory();

	/** Return the highest-scoring theory in the beam, and delete the
	  * rest. The returned theory is removed from the beam so that it
	  * won't be deleted when the beam is deleted.
	  */
	SentenceTheory *extractBestTheory();


	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	SentenceTheoryBeam(StateLoader *stateLoader, int sent_no,
					   DocTheory *docTheory, int width);
	void resolvePointers(StateLoader *stateLoader);

	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit SentenceTheoryBeam(SerifXML::XMLTheoryElement elem, const Sentence* sentence, const Symbol& docid);
	void resolvePointers(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;

private:
	const Sentence *_sentence;

	int _beam_width;
	int _n_theories;
	SentenceTheory **_theories;

	/** List in which each subtheory is represented exactly once.
	  * Used for state-saving and for mark-sweep GC. */
	mutable const SentenceSubtheory **_uniqueSubtheories;
	mutable int _n_unique_subtheories;
	mutable bool _unique_subtheories_up_to_date;
	void ensureUniqueSubtheoriesUpToDate() const;

	void saveSubtheory(const SentenceSubtheory *subtheory,
					   StateSaver *stateSaver) const;
	SentenceSubtheory *loadSubtheory(StateLoader *stateLoader);
};


#endif
