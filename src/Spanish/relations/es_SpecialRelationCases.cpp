// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Spanish/relations/es_SpecialRelationCases.h"
#include "Generic/relations/PotentialRelationInstance.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Spanish/parse/es_STags.h"
#include "Generic/relations/RelationTypeSet.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"

static Symbol apostrophe_mSym(L"'m");
static Symbol forSym(L"for");
static Symbol ofSym(L"of");
static Symbol inSym(L"in");
static Symbol atSym(L"at");
static Symbol nearSym(L"near");
static Symbol onSym(L"on");

Symbol SpecialRelationCases::EMPLOYEES;
Symbol SpecialRelationCases::GPE_PART;
Symbol SpecialRelationCases::LOCATED;
Symbol SpecialRelationCases::MEMBERS;
Symbol SpecialRelationCases::SUBORDINATE;

int SpecialRelationCases::findSpecialRelationType(PotentialRelationInstance *instance,
												  const Mention *first, const Mention *second)
{
	if (!EntityType::isValidEntityType(instance->getLeftEntityType()) ||
		!EntityType::isValidEntityType(instance->getRightEntityType()))
		return 0;

	EntityType leftType = EntityType(instance->getLeftEntityType());
	EntityType rightType = EntityType(instance->getRightEntityType());

	// for CNN, I'm Liz Boschee
	if (instance->getPredicate() == apostrophe_mSym &&
		leftType.matchesPER() &&
		rightType.matchesORG() &&
		instance->getRightRole() == forSym)
	{
		int type = RelationTypeSet::getTypeFromSymbol(getEmployeesType());
		if (type != RelationTypeSet::INVALID_TYPE)
			return type;
	}

	return 0;
}

bool SpecialRelationCases::isSpecialPrepositionStack(PotentialRelationInstance *instance,
												     const Mention *first,
													 const Mention *second)
{
	const SynNode *firstPP = first->getNode()->getParent();
	const SynNode *secondPP = second->getNode()->getParent();
	if (firstPP == 0 || secondPP == 0 ||
		firstPP->getTag() != SpanishSTags::PP ||
		secondPP->getTag() != SpanishSTags::PP)
		return false;
	const SynNode *parent = firstPP->getParent();
	if (parent == 0 || parent != secondPP->getParent())
		return false;
	bool stacked = false;
	for (int i = 0; i < parent->getNChildren() - 1; i++) {
		if (parent->getChild(i) == firstPP) {
			if (parent->getChild(i+1) == secondPP) {
				stacked = true;
				break;
			} else return false;
		}
	}
	if (!stacked) return false;

	if (!EntityType::isValidEntityType(instance->getLeftEntityType()) ||
		!EntityType::isValidEntityType(instance->getRightEntityType()))
		return 0;

	EntityType leftType = EntityType(instance->getLeftEntityType());
	EntityType rightType = EntityType(instance->getRightEntityType());

	// let's only do this for types we understand well
	if (!leftType.matchesFAC() &&
		!leftType.matchesGPE() &&
		!leftType.matchesLOC() &&
		!leftType.matchesORG() &&
		!leftType.matchesPER())
		return false;

	// no persons on the right side
	if (!rightType.matchesFAC() &&
		!rightType.matchesGPE() &&
		!rightType.matchesLOC() &&
		!rightType.matchesORG())
		return false;

	// usually names don't get stacked under descriptors...
	//   "he was killed in St. Petersburg in his apartment" != stack
	//   exception: "he was killed on Prospect Street in the city of Moscow"...
	//     but what can you do?
	if (first->getMentionType() == Mention::NAME &&
		second->getMentionType() == Mention::DESC)
		return false;

	if (instance->getRightRole() == ofSym) {
		if ((leftType.matchesGPE() || leftType.matchesLOC()) &&
			(rightType.matchesGPE() || rightType.matchesLOC()))
		{
			if (first->getMentionType() == Mention::DESC &&
				second->getMentionType() == Mention::NAME)
			{
				// get the heck out of here...
				//   this is probably something like "republic of Chechnya"
				return false;
			}
		}
	}

	if (instance->getLeftRole() == ofSym) {

		// of the building  in   Russia
		//        people    on   the street
		//        cafe      at   the Kremlin
		//        mountains near the lake
		//        north     of   the city
		if (instance->getRightRole() == onSym ||
		    instance->getRightRole() == atSym ||
		    instance->getRightRole() == nearSym ||
		    instance->getRightRole() == inSym ||
			instance->getRightRole() == ofSym)
		{
			return true;
		} else return false;

	}

	// don't use PER unless preceded by of (too dangerous)
	if (leftType.matchesPER()) {
		return false;
	}

	// "in" and "of" are usually good bets on the right-hand side
	//   (unless the lefthand side is a person, but we've already taken care of that)
	if (instance->getRightRole() == inSym ||
		instance->getRightRole() == ofSym)
	{
		return true;
	}

	// "on" and "near" are OK bets for the right-hand side, but
	//   only if the lefthand side is "in" or "at"
	if (instance->getRightRole() == onSym ||
		instance->getRightRole() == nearSym)
	{
		if (instance->getLeftRole() == inSym ||
		    instance->getLeftRole() == atSym)
		{
			return true;
		} else return false;

	}

	return false;
}

Symbol SpecialRelationCases::getLocatedType() {
	initHardCodedRelationTypes();
	return LOCATED;
}

Symbol SpecialRelationCases::getEmployeesType() {
	initHardCodedRelationTypes();
	return EMPLOYEES;
}

Symbol SpecialRelationCases::getGPEPartType() {
	initHardCodedRelationTypes();
	return GPE_PART;
}

Symbol SpecialRelationCases::getGroupMembers() {
	initHardCodedRelationTypes();
	return MEMBERS;
}

Symbol SpecialRelationCases::getSubordinateType() {
	initHardCodedRelationTypes();
	return SUBORDINATE;
}

void SpecialRelationCases::initHardCodedRelationTypes() {
	// currently, these types are only used within relation finder, so
	// it doesn't need to be static, but at one point it was used in generic filter,
	// etc., so it may need to be global... probably it should be in
	// RelationTypeSet. But whatever. Anyway, because it's global, I'm a little
	// scared of errors popping up at runtime when an expt happens to call it
	// that didn't before -- I'm paranoid. SO -- don't fail if it can't find the right
	// parameter... but complain.

	static int project_type = NOT_INIT;
	if (project_type == NOT_INIT) {
		// Use a default value of nullSymbol.
		EMPLOYEES = SymbolConstants::nullSymbol;
		GPE_PART = SymbolConstants::nullSymbol;
		LOCATED = SymbolConstants::nullSymbol;
		MEMBERS = SymbolConstants::nullSymbol;
		SUBORDINATE = SymbolConstants::nullSymbol;
		std::string rf_type = ParamReader::getRequiredParam("relation_finder_type");
		project_type = NONE;
		if (rf_type == "EELD") {
			LOCATED = Symbol(L"inRegion");
			EMPLOYEES = Symbol(L"employees");
			GPE_PART = Symbol(L"geographicalSubregions:reversed");
			MEMBERS = Symbol(L"groupMembers:reversed");
			project_type = EELD;
		} else if (rf_type == "ACE") {
			LOCATED = Symbol(L"AT.LOCATED");
			EMPLOYEES = Symbol(L"ROLE.GENERAL-STAFF");
			GPE_PART = Symbol(L"PART.PART-OF");
			MEMBERS = Symbol(L"ROLE.MEMBER");
			project_type = ACE;
		} else if (rf_type == "ACE2005") {
			LOCATED = Symbol(L"PHYS.Located");
			EMPLOYEES = Symbol(L"ORG-AFF.Employment");
			GPE_PART = Symbol(L"PART-WHOLE.Geographical");
			MEMBERS = Symbol(L"ORG-AFF.Membership");
			project_type = ACE2005;
		} else if (rf_type == "ITEA") {
			// don't do special relation cases for ITEA, but don't warn either
			SUBORDINATE = Symbol(L"PART.SUBSIDIARY");
		} else {
			SessionLogger::warn("inv_rel_0") 
				<< "SpecialRelationCases::initHardCodedRelationTypes: Invalid relation"
				<< " finder type: " << rf_type << "\n";
		}
	}
}

