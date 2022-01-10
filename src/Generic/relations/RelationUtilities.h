// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATION_UTILITIES_H
#define RELATION_UTILITIES_H

#include <boost/shared_ptr.hpp>

class PropositionSet;
class MentionSet;
class Mention;
class Symbol;
class RelationObservation;
class PotentialRelationInstance;

#include "Generic/theories/Proposition.h"

class RelationUtilities {
public:
	/** Create and return a new RelationUtilities. */
	static RelationUtilities *get() { return _factory()->get(); }
	/** Hook for registering new RelationUtilities factories. */
	struct Factory { 
		virtual RelationUtilities *get() = 0; 
	};
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }


	virtual std::vector<bool> identifyFalseOrHypotheticalProps(const PropositionSet *propSet, const MentionSet *mentionSet) =0;
	virtual bool coercibleToType(const Mention *mention, Symbol type) =0;

	virtual Symbol stemPredicate(Symbol word, Proposition::PredType predType) =0;
	virtual Symbol stemWord(Symbol word, Symbol pos) { return word; }

	virtual bool validRelationArgs(Mention *m1, Mention *m2) { return true; }
	virtual bool isValidRelationEntityTypeCombo(Symbol validation_type, 
		const Mention* _m1, const Mention* _m2, Symbol relType) {return true;};
	virtual bool is2005ValidRelationEntityTypeCombo(const Mention* arg1, const Mention* arg2,
		Symbol relationType ){return true;};

	virtual UTF8OutputStream& getDebugStream() { return _debugStream; };
	virtual bool debugStreamIsOn() { return false; } ;
	virtual bool isPrepStack(RelationObservation *o) { return false; }


	virtual int calcMentionDist(const Mention *m1, const Mention *m2){return 0;}
	virtual bool distIsLessThanCutoff(const Mention *m1, const Mention *m2){return false;}
	virtual Symbol mapToTrainedOnSubtype(Symbol subtype) { return subtype; }

	virtual void artificiallyStackPrepositions(PotentialRelationInstance *instance) {};
	virtual int getOrgStack(const Mention *mention, Mention **orgs, int max_orgs) { return 0; }

	virtual int getRelationCutoff() { return 0; }
	virtual int getAllowableRelationDistance() { return 0; }
	virtual const SynNode *findNPChunk(const SynNode *node) { return 0; }
	virtual int getMentionStartToken(const Mention* m1) { return 0; }
	virtual int getMentionEndToken(const Mention* m1) { return 0; }
	virtual void fillClusterArray(const Mention* ment, int* clust) {  }

	virtual bool isATEARelationType(Symbol type) { return false; }

	virtual ~RelationUtilities() {}

private:
	static boost::shared_ptr<Factory> &_factory();
	static UTF8OutputStream _debugStream;


};

//#ifdef ENGLISH_LANGUAGE
//	#include "English/relations/en_RelationUtilities.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/relations/ch_RelationUtilities.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/relations/ar_RelationUtilities.h"
//#else
//	#include "Generic/relations/xx_RelationUtilities.h"
//#endif



#endif
