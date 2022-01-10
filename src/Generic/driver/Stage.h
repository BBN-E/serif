// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STAGE_H
#define STAGE_H

#include "Generic/common/hash_set.h"
#include "Generic/common/hash_map.h"
#include <boost/shared_ptr.hpp>

/** An index object corresponding with a single execution stage in the
  * Serif pipeline (e.g., tokenizing or parsing).  Each stage is identified
  * by a globally unique name (such as "parse").  Stages are strictly 
  * ordered according to their position in a gloal "stage sequence."  
  *
  * This sequence may be modified (e.g., by adding stages, or swapping
  * the positions of stages) using static Stage methods.
  *
  * Two special stages, named "start" and "end", are defined.  These stages
  * are meant to serve as markers -- the serif pipeline does not do any
  * actual work for these stages.  The positions of these two stages in the
  * global stage sequence is fixed.
  * 
  * Stages may be passed around by value, and can be compared using standard
  * numeric comparison operators. 
  *
  * See Stage.cpp for a list of the default stages, or run "Serif --stages".
  */

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED Stage {
public:
	/** Construct a new "NULL" stage object.  In terms of ordering, this
	* stage is considered to come after all other stages. */
	Stage();

	/** Construct a new Stage refering to the stage with the given name.
	  * If no such stage exists, then raise an UnexpectedInputException. */
	explicit Stage(const char *name);

	/** Return the name of this stage */
	const char *getName() const;

	/** Return a short description for this stage. */
	const char* getDescription() const;

	/** Is is ok to call the state saver after running this stage? */
	bool okToSaveStateAfterThisStage() const;

	/** Return this stage's current sequence number -- i.e., its index in
	  * the overall sequence of stages.  Note that a stage's sequence number
	  * may change if stages are added or rearranged. */
	int getSequenceNumber() const;

	/** Return the special "start" stage.  Note that this is no longer an
	  * alias for the first processing stage; it is a distinct value
	  * that is used as a marker. */
	static Stage getStartStage();

	/** Return the special "end" stage.  Note that this is no longer an
	  * alias for the last processing stage; it is a distinct value that
	  * is used as a marker. */
	static Stage getEndStage();

	/** Return the stage that comes immediately after this stage.  If this
	  * stage is the special 'end' stage, then return the NULL stage.  If
	  * this stage is NULL, then return NULL. */
	Stage getNextStage();

	/** Return the stage that comes immediately before this stage.  If this
	  * stage is the special 'start' stage, or is NULL, then return this 
	  * stage. */
	Stage getPrevStage();

	/** Update this stage to point to the next stage.  If this stage is 
	  * already the special 'end' stage, then move to the NULL stage.  If
	  * this stage is NULL, then do nothing.. */
	Stage &operator++();

	/** Update this stage to point to the previous stage.  If this stage is 
	  * already the special 'start' stage, or is NULL, then do nothing. */
	Stage &operator--();

	// Standard numerical comparison operators check stage ordering.
	bool operator==(const Stage &other) const;
	bool operator!=(const Stage &other) const;
	bool operator<(const Stage &other) const;
	bool operator<=(const Stage &other) const;
	bool operator>(const Stage &other) const;
	bool operator>=(const Stage &other) const;

	// Copy constructor & assignment operator.
	Stage(const Stage &other);
	Stage& operator=(const Stage &other);

	/** Return the first "real" stage in the global stage sequence.  This is
	  * equivalent to Stage("start").getNextStage(). */
	static Stage getFirstStage();

	/** Return the last sentence-level stage in the global stage sequence. */
	static Stage getLastSentenceLevelStage();

	/** Return the last "real" stage in the global stage sequence.  This is
	  * equivalent to Stage("end").getPrevStage(). */ 
	static Stage getLastStage();

	//=============== Stage Sequence Modification ========================

	/** Swap the positions of the two given stages in the global stage 
	  * sequence.  No other stages will be affected. */
	static void swapStageOrder(Stage stage1, Stage stage2);

	/** Add a new stage to the global stage sequence, immediately following 
	  * the stage "refStage".  Return the new Stage object.  Note: this
	  * will modify the sequence numbers of any stages that follow the
	  * newly inserted stage. */
	static Stage addNewStageAfter(Stage refStage, const char* name, 
	                              const char* description,
								  bool okToSaveStateAfterThisStage=true);

	/** Add a new stage to the global stage sequence, immediately preceeding
	  * the stage "refStage".  Return the new Stage object.  Note: this
	  * will modify the sequence numbers of any stages that follow the
	  * newly inserted stage. */
	static Stage addNewStageBefore(Stage refStage, const char* name, 
								   const char* description,
								   bool okToSaveStateAfterThisStage=true);

	//========================== Associated Types ========================
	size_t hash_code() const;
	struct Hash {size_t operator()(const Stage& s) const {return s.hash_code();}};
	struct Eq {bool operator()(const Stage& s1, const Stage& s2) const {return s1 == s2;}};
	typedef hash_set<Stage, Hash, Eq> HashSet;
	template<typename ValueT> class HashMap: public serif::hash_map<Stage, ValueT, Hash, Eq> {};

private:
	// StageInfo is a private class used to hold information about each
	// Stage.  Two Stage objects are equal iff they point at the same 
	// StageInfo object.
	struct StageInfo;
	typedef boost::shared_ptr<Stage::StageInfo> StageInfo_ptr;
	StageInfo_ptr _info;
	explicit Stage(StageInfo_ptr stageInfo): _info(stageInfo) {}

	// StageSequence is a private class used to keep track of the sequence
	// in which stages should be run.  Its single global instance is returned
	// by the method Stage::getStageSequence().
	class StageSequence;
	static StageSequence& getStageSequence();
};


#endif
