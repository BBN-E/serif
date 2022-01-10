#ifndef EN_RELATION_UTILITIES_H
#define EN_RELATION_UTILITIES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/relations/RelationUtilities.h"


class PropositionSet;
class SymbolHash;
class PotentialRelationInstance;

class EnglishRelationUtilities : public RelationUtilities {
private:
	friend class EnglishRelationUtilitiesFactory;
public:
	std::vector<bool> identifyFalseOrHypotheticalProps(const PropositionSet *propSet, 
	                                                          const MentionSet *mentionSet);
	SymbolHash * _unreliabilityIndicators;
	bool coercibleToType(const Mention *ment, Symbol type);
	Symbol stemPredicate(Symbol word, Proposition::PredType predType);	
	Symbol stemWord(Symbol word, Symbol pos);
	void artificiallyStackPrepositions(PotentialRelationInstance *instance);
	int getOrgStack(const Mention *mention, Mention **orgs, int max_orgs);	
	bool isPrepStack(RelationObservation *o);	
	bool isValidRelationEntityTypeCombo(Symbol validation_type, 
		const Mention* _m1, const Mention* _m2, Symbol relType);
	bool is2005ValidRelationEntityTypeCombo(const Mention* arg1, const Mention* arg2,
		Symbol relationType );

	int calcMentionDist(const Mention *m1, const Mention *m2);
	int getRelationCutoff();
	bool distIsLessThanCutoff(const Mention *m1, const Mention *m2);
	int getMentionStartToken(const Mention* m1);
	int getMentionEndToken(const Mention* m1);
	Symbol mapToTrainedOnSubtype(Symbol subtype);

	bool isATEARelationType(Symbol type);

private:

	static Symbol WEA;
	static Symbol VEH;
	static Symbol PER_SOC;
	static Symbol PHYS_LOCATED;
	static Symbol PHYS_NEAR;
	static Symbol PART_WHOLE_GEOGRAPHICAL;
	static Symbol PART_WHOLE_SUBSIDIARY;
	static Symbol PART_WHOLE_ARTIFACT;
	static Symbol ORG_AFF_EMPLOYMENT;
	static Symbol ORG_AFF_OWNERSHIP;
	static Symbol ORG_AFF_FOUNDER;
	static Symbol ORG_AFF_STUDENT_ALUM;
	static Symbol EDUCATIONAL;
	static Symbol ORG_AFF_SPORTS_AFFILIATION;
	static Symbol SPORTS;
	static Symbol ORG_AFF_INVESTOR_SHAREHOLDER;
	static Symbol ORG_AFF_MEMBERSHIP;
	static Symbol ART_USER_OWNER_INVESTOR_MANUFACTURER;
	static Symbol GEN_AFF_CITIZEN_RESIDENT_RELIGION_ETHNICITY;
	static Symbol GEN_AFF_ORG_LOCATION;
	static Symbol PER_SOC_SUBORDINATE;

};

// RelationModel factory
class EnglishRelationUtilitiesFactory: public RelationUtilities::Factory {

	EnglishRelationUtilities* utils;
	virtual RelationUtilities *get() { 
		if (utils == 0) {
			utils = _new EnglishRelationUtilities(); 
		} 
		return utils;
	}

public:
	EnglishRelationUtilitiesFactory() { utils = 0;}

};


#endif
