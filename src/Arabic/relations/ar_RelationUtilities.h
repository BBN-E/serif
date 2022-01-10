#ifndef AR_RELATION_UTILITIES_H
#define AR_RELATION_UTILITIES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/relations/RelationUtilities.h"

class PropositionSet;
class Mention;
class MentionSet;

class ArabicRelationUtilities : public RelationUtilities {
private:
	friend class ArabicRelationUtilitiesFactory;
public:
	std::vector<bool> identifyFalseOrHypotheticalProps(const PropositionSet *propSet, 
		const MentionSet *mentionSet);
	
	bool coercibleToType(const Mention *ment, Symbol type) { 
		return false; 
	
	}
	Symbol stemPredicate(Symbol word, Proposition::PredType predType) { 
		return word; 
	}

	bool validRelationArgs(Mention *m1, Mention *m2);


	const SynNode *findNPChunk(const SynNode *node);
	
	int calcMentionDist(const Mention *m1, const Mention *m2);
	int getRelationCutoff();
	int getAllowableRelationDistance();
	bool distIsLessThanCutoff(const Mention *m1, const Mention *m2);
	bool isValidRelationEntityTypeCombo(Symbol validation_type, 
		const Mention* _m1, const Mention* _m2, Symbol relType);
	bool is2005ValidRelationEntityTypeCombo(const Mention* arg1, const Mention* arg2,
		Symbol relationType );
	int getMentionStartToken(const Mention* m1);
	int getMentionEndToken(const Mention* m1);
	void fillClusterArray(const Mention* ment, int* clust);

};


// RelationModel factory
class ArabicRelationUtilitiesFactory: public RelationUtilities::Factory {

	ArabicRelationUtilities* utils;
	virtual RelationUtilities *get() { 
		if (utils == 0) {
			utils = _new ArabicRelationUtilities(); 
		} 
		return utils;
	}

public:
	ArabicRelationUtilitiesFactory() { utils = 0;}

};

#endif
