// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_SPECIAL_RELATION_CASES_H
#define es_SPECIAL_RELATION_CASES_H

class Symbol;
class PotentialRelationInstance;
class Mention;

class SpecialRelationCases {
private:
	static Symbol EMPLOYEES;
	static Symbol GPE_PART;
	static Symbol MEMBERS;
	static Symbol LOCATED;
	static Symbol SUBORDINATE;
	
	enum { NOT_INIT, NONE, ACE, EELD, ACE2005 };
	
public:
	SpecialRelationCases() {}
	static void initHardCodedRelationTypes();
	int findSpecialRelationType(PotentialRelationInstance *instance,
		const Mention *first, const Mention *second);
	bool isSpecialPrepositionStack(PotentialRelationInstance *instance, 
		const Mention *first, const Mention *second);
	static Symbol getLocatedType();
	static Symbol getEmployeesType();
	static Symbol getGPEPartType();
	static Symbol getGroupMembers();
	static Symbol getSubordinateType();
};

#endif
