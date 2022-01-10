#ifndef CH_RELATION_UTILITIES_H
#define CH_RELATION_UTILITIES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/relations/RelationUtilities.h"


#include "Generic/common/UTF8OutputStream.h"
class PropositionSet;
class Symbol;
class MaxEntEvent;
class PotentialRelationInstance;

class ChineseRelationUtilities : public RelationUtilities {
private:
	friend class ChineseRelationUtilitiesFactory;
public:
	std::vector<bool> identifyFalseOrHypotheticalProps(const PropositionSet *propSet, 
		const MentionSet *mentionSet);

	bool coercibleToType(const Mention *ment, Symbol type);
	Symbol stemPredicate(Symbol word, Proposition::PredType predType);

	UTF8OutputStream& getDebugStream();
	bool debugStreamIsOn();

	bool isValidRelationEntityTypeCombo(Symbol validation_type, 
		const Mention* _m1, const Mention* _m2, Symbol relType);
	bool is2005ValidRelationEntityTypeCombo(const Mention* arg1, const Mention* arg2,
		Symbol relationType );

private:

	static Symbol SET_SYM;
	static Symbol COMP_SYM;
};


// RelationModel factory
class ChineseRelationUtilitiesFactory: public RelationUtilities::Factory {

	ChineseRelationUtilities* utils;
	virtual RelationUtilities *get() { 
		if (utils == 0) {
			utils = _new ChineseRelationUtilities(); 
		} 
		return utils;
	}

public:
	ChineseRelationUtilitiesFactory() { utils = 0;}

};

#endif
