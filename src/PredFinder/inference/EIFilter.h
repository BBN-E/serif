#include "boost/foreach.hpp"
#include "boost/make_shared.hpp"
#include "Generic/common/Offset.h"
#include "Generic/common/bsp_declare.h"
#include "Generic/common/SessionLogger.h"
#include "PredFinder/inference/EIUtils.h"
#include "PredFinder/elf/ElfRelation.h"
#include "boost/tuple/tuple.hpp"
#include <set>
#include <string>
#include <vector>
#include <map>

class EIFilter;

class NameEquivalenceTable;
BSP_DECLARE(EIFilter);
BSP_DECLARE(ElfDocument);
BSP_DECLARE(ElfIndividual);
BSP_DECLARE(ElfRelation);
BSP_DECLARE(ElfRelationArg);
BSP_DECLARE(EIDocData);
BSP_DECLARE(ElfInference);
BSP_DECLARE(ElfType);
#pragma once

typedef std::pair<std::wstring,int> Prerequisite;

/**
 * Simple class used to store the number of relations to add,
 * relations to remove, and individuals to remove at a point
 * in time (e.g., upon entering a method). Used by EIFilter.
 *
 * @author afrankel@bbn.com
 * @date 2011.05.26
 **/

class EIFilterSnapshot {
public:
	EIFilterSnapshot(EIFilter * filt); // stores counts as of the point when object is constructed
	~EIFilterSnapshot() {}
	size_t getOrigCountOfRelationsToAdd() const {return _orig_count_of_relations_to_add;}
	size_t getOrigCountOfRelationsToRemove() const {return _orig_count_of_relations_to_remove;}
	size_t getOrigCountOfIndividualsToRemove() const {return _orig_count_of_individuals_to_remove;}
	size_t getOrigCountOfIndividualsToAdd() const { return _orig_count_of_individuals_to_add;}
private:
	size_t _orig_count_of_relations_to_add;
	size_t _orig_count_of_relations_to_remove;
	size_t _orig_count_of_individuals_to_remove;
	size_t _orig_count_of_individuals_to_add;
};


// To add a new filter:
// See the documentation below
// For additional information, see http://wiki.d4m.bbn.com/wiki/Erudite/ElfInference
// 1. write the filter code
// 2. register your filter in the ElfInference.cpp filed in the initializeFilters function
// 3. Add your filter's name to the filter sequence file refered to in the .par file.

/**
 * Base class for implementing a filter, which can be used to perform one or more
 * of the following actions on the elements in a set of ELF documents:
 * - add relations
 * - remove selected relations
 * - remove selected individuals
 * - add individuals (unimplemented as of yet, since there has been no necessity for this functionality)
 * 
 * The action_type returns handled in the EIFilter base class here (as far as I undersand) were for future functionality
 * that allowed for the undoing of filter actions. This functionality was never completed, but the infrastructure to return
 * this enumeration has been kept. It is not in use. The snapshots are used to track the changes that a subclass does so that
 * it can return the correct action_type.
 *
 * The reason for the processAll, processIndividual, and processRelation split is so that iteration over individuals and relations can
 * be abstracted away and future usage can execute multiple filters on a single pass through the relation or individual set. This functionality
 * is only partially supported in the EIFilterManager code.
 *
 * @author afrankel@bbn.com
 * revision mshafir@bbn.com
 * @date 2011.05.26
 **/
class EIFilter {
public:
	virtual ~EIFilter() {}

	typedef unsigned short action_type;
	static const action_type NOP = 0x0000;
	static const action_type ADD_RELATION = 0x0001;
	static const action_type REMOVE_RELATION = 0x0002;
	static const action_type REMOVE_INDIVIDUAL = 0x0004;
	static const action_type ADD_INDIVIDUAL = 0x0008;
	static const action_type UNKNOWN = 0x0016;
	
	virtual void processAll(EIDocData_ptr docData) { SessionLogger::err("EIFilterManager") << L"Filter " << _name << L" specified processAll, but the derrived function was not found...\n"; }
	virtual void processIndividual(EIDocData_ptr docData, const ElfIndividual_ptr individual) { SessionLogger::err("EIFilterManager") << L"Filter " << _name << L" specified processRelation, but the derrived function was not found...\n"; }
	virtual void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) { SessionLogger::err("EIFilterManager") << L"Filter " << _name << L" specified processIndividual, but the derrived function was not found...\n"; }

	//provided for backwards compatibility
	virtual void apply(EIDocData_ptr docData);

	virtual action_type handleAll(EIDocData_ptr docData) {
		EIFilterSnapshot snapshot(this);
		this->processAll(docData);
		return handleSnapshot(docData, snapshot);
	}
	virtual action_type handleIndividual(EIDocData_ptr docData, const ElfIndividual_ptr individual) {
		EIFilterSnapshot snapshot(this);
		this->processIndividual(docData,individual);
		return handleSnapshot(docData, snapshot);
	}
	virtual action_type handleRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
		EIFilterSnapshot snapshot(this);
		this->processRelation(docData,relation);
		return handleSnapshot(docData, snapshot);
	}
	
	virtual bool matchesIndividual(EIDocData_ptr docData, const ElfIndividual_ptr individual) const {
		if (!matchIndividuals.empty()) {
			BOOST_FOREACH(std::wstring t, matchIndividuals) {
				if (individual->has_type(t))
					return true;
			}
			return false;
		}
		return true;
	}
	virtual bool matchesRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) const {
		return matchRelations.empty() || (matchRelations.find(relation->get_name()) != matchRelations.end());
	}

	virtual std::set<Prerequisite> getPrerequisites() const { return prerequisites; }
	virtual const std::wstring & getFilterName() {return _name;}
	
	bool isRelationFilter() const {return _relation_filter;}
	bool isIndividualFilter() const {return _individual_filter;}
	bool isProcessAllFilter() const {return _process_all;}
	
	size_t getCountOfRelationsToAdd() {return _relationsToAdd.size();}
	size_t getCountOfRelationsToRemove() {return _relationsToRemove.size();}
	size_t getCountOfIndividualsToRemove() {return _individualsToRemove.size();}
	size_t getCountOfIndividualsToAdd() {return _individualsToAdd.size();}

	std::set<ElfRelation_ptr> getRelationsToAdd() {return _relationsToAdd;}
	std::set<ElfRelation_ptr> getRelationsToRemove() {return _relationsToRemove;}
	std::set<ElfIndividual_ptr> getIndividualsToAdd() {return _individualsToAdd;}
	std::set<ElfIndividual_ptr> getIndividualsToRemove() {return _individualsToRemove;}

	void clearRelationsToRemove() {_relationsToRemove.clear();}
	void clearRelationsToAdd() {_relationsToAdd.clear();}
	void clearIndividualsToRemove() {_individualsToRemove.clear();}
	void clearIndividualsToAdd() {_individualsToAdd.clear();}

	void addPrerequisite(std::wstring filter, int count) { 
		prerequisites.insert(Prerequisite(filter,count)); 
	}

	std::wstring getDecoratedFilterName() {
		std::wstring filter_name(L"eru:");
		filter_name += getFilterName();
		return filter_name;
	}

	virtual std::wstring getOrderPrintout() {
		return getDecoratedFilterName();
	}

protected:
	// This constructor is protected because we only want to call it from derived classes.
	EIFilter(const std::wstring & name, bool relationFilter=false, bool individualFilter=false, bool processAll=false) : _name(name) {
		_relation_filter = relationFilter;
		_individual_filter = individualFilter;
		_process_all = processAll;
	}
	
	void addRelationMatch(std::wstring relation) { matchRelations.insert(relation); }
	void addIndividualMatch(std::wstring relation) { matchIndividuals.insert(relation); }
	
	const std::wstring _name;
	bool _relation_filter;
	bool _individual_filter;
	bool _process_all;
	std::set<Prerequisite> prerequisites;

	std::set<ElfRelation_ptr> _relationsToAdd;
	std::set<ElfRelation_ptr> _relationsToRemove;
	ElfIndividualSet _individualsToAdd;
	ElfIndividualSet _individualsToRemove;
	
	std::set<std::wstring> matchRelations;
	std::set<std::wstring> matchIndividuals;
private:
	// convenience method called by processAll(), processAllRelations(), and processAllIndividuals() methods
	action_type handleSnapshot(EIDocData_ptr docData, const EIFilterSnapshot & snapshot);

};

class EIRemovalFilter : public EIFilter {
public:
	virtual ~EIRemovalFilter() {}

	virtual void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {
		if (this->doRemoveRelation(docData,relation)) {
			_relationsToRemove.insert(relation);
		}
	}

	virtual void processIndividual(EIDocData_ptr docData, const ElfIndividual_ptr individual) {
		if (this->doRemoveIndividual(docData,individual)) {
			_individualsToRemove.insert(individual);
		}
	}

protected:
	EIRemovalFilter(const std::wstring & name, bool relationFilter=false, bool individualFilter=false, bool processAllFilter=false) 
		: EIFilter(name,relationFilter,individualFilter,processAllFilter) {}
	
	virtual bool doRemoveRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) {SessionLogger::err("EIFilterManager") << L"Removal Filter " << _name << L" did not provide doRemoveRelation...\n"; return false;}
	virtual bool doRemoveIndividual(EIDocData_ptr docData, const ElfIndividual_ptr individual) {SessionLogger::err("EIFilterManager") << L"Removal Filter " << _name << L" did not provide doRemoveIndividual...\n"; return false;}
};

/**
 * Class derived from EIFilter for removing any relation in which the same arg
 * (i.e., the arg with the same ID) appears in different roles.
 *
 * @author afrankel@bbn.com
 * revision mshafir@bbn.com
 * @date 2011.05.26
 **/
class DoubleEntityFilter : public EIRemovalFilter {
public:
	DoubleEntityFilter() : EIRemovalFilter(L"DoubleEntityFilter",true) {}
	~DoubleEntityFilter() {}
	bool matchesRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) const {
		return (relation->arity() == 2);
	}
	bool doRemoveRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

/**
 * Class derived from EIFilter for removing any relation in which an NFLTeam arg
 * cannot be mapped to an NFL team individual.
 *
 * @author afrankel@bbn.com
 * revision mshafir@bbn.com
 * @date 2011.05.26
 **/
class UnmappedNFLTeamFilter : public EIRemovalFilter {
public:
	UnmappedNFLTeamFilter() : EIRemovalFilter(L"UnmappedNFLTeamFilter",true) {}
	~UnmappedNFLTeamFilter() {}
	bool doRemoveRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class LeadershipFilter : public EIRemovalFilter {
public:
	LeadershipFilter() : EIRemovalFilter(L"LeadershipFilter",true) {
		addRelationMatch(L"eru:isLedBy");
		addRelationMatch(L"eru:HasTopMemberOrEmployee");
	}
	~LeadershipFilter() {}
	bool doRemoveRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class EmploymentFilter : public EIRemovalFilter {
public:
	EmploymentFilter() : EIRemovalFilter(L"EmploymentFilter",true) {
		addRelationMatch(L"eru:employs");
		addRelationMatch(L"eru:HasEmployer");
	}
	~EmploymentFilter() {}
	bool doRemoveRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class LocationFilter : public EIRemovalFilter {
public:
	LocationFilter() : EIRemovalFilter(L"LocationFilter",true) {
		addRelationMatch(L"eru:gpeHasSubGpe");
	}
	~LocationFilter() {}
	bool doRemoveRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class MarriageFilter : public EIFilter {
public:
	MarriageFilter() : EIFilter(L"MarriageFilter",true) {
		addRelationMatch(L"eru:marriedInYear");
	}
	~MarriageFilter() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class PersonnelHiredFiredFilter : public EIFilter {
public:
	PersonnelHiredFiredFilter() : EIFilter(L"PersonnelHiredFiredFilter",true) {
		addRelationMatch(L"eru:terminatedEmployment");
		addRelationMatch(L"eru:hired");
	}
	~PersonnelHiredFiredFilter() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class PersonnelHeldPositionFilter : public EIFilter {
public:
	PersonnelHeldPositionFilter() : EIFilter(L"PersonnelHeldPositionFilter",true) {
		addRelationMatch(L"eru:heldPosition");
	}
	~PersonnelHeldPositionFilter() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class GenericViolenceFilter : public EIRemovalFilter {
public:
	GenericViolenceFilter() : EIRemovalFilter(L"GenericViolenceFilter",true,true) {
		addIndividualMatch(L"NONE");
	}
	~GenericViolenceFilter() {}
	bool matchesRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) const {
		return (EIUtils::isGenericViolence(relation));
	}
	bool doRemoveRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) { return true; }
	bool doRemoveIndividual(EIDocData_ptr docData, const ElfIndividual_ptr individual) {return true;}
};

class GenderFilter : public EIFilter {
public:
	GenderFilter() : EIFilter(L"GenderFilter", false, false, true) {	}
	~GenderFilter() {}
	void processAll(EIDocData_ptr docData);
};

class MilitaryAttackFilter : public EIFilter {
public:
	MilitaryAttackFilter() : EIFilter(L"MilitaryAttackFilter",true) {}
	~MilitaryAttackFilter() {}
	bool matchesRelation(EIDocData_ptr docData, const ElfRelation_ptr relation) const {
		return (EIUtils::getTBDEventType(relation) != L"");
	}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class BoundIDMemberishFilter : public EIFilter {
public:
	BoundIDMemberishFilter() : EIFilter(L"BoundIDMemberishFilter",true) {}
	~BoundIDMemberishFilter() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class NatBoundIDFilter : public EIFilter {
public:
	NatBoundIDFilter() : EIFilter(L"NatBoundIDFilter",true) {}
	~NatBoundIDFilter() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class InformalMemberFilter : public EIFilter {
public:
	InformalMemberFilter() : EIFilter(L"InformalMemberFilter",true) {
		addRelationMatch(L"eru:hasInformalMember");
	}
	~InformalMemberFilter() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class OrgLocationFilter : public EIFilter {
public:
	OrgLocationFilter() : EIFilter(L"OrgLocationFilter",false,false,true) {}
	~OrgLocationFilter() {}
	void processAll(EIDocData_ptr docData);
};

class PerLocationFromDescriptorFilter : public EIFilter {
public:
	PerLocationFromDescriptorFilter() : EIFilter(L"PerLocationFromDescriptorFilter",false,false,true) {}
	~PerLocationFromDescriptorFilter() {}
	void processAll(EIDocData_ptr docData);
};

class PerLocationFromAttackFilter : public EIFilter {
public:
	PerLocationFromAttackFilter(); // see source file
	~PerLocationFromAttackFilter() {}
	// uses processRelation(), unlike PerLocationFromDescriptorFilter
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
protected:
	std::set<std::wstring> _victimRoles;
	EntitySubtype _nationSubtype;
};

class LeadershipPerOrgFilter : public EIFilter {
public:
	LeadershipPerOrgFilter(const std::wstring& ontology); 
	~LeadershipPerOrgFilter() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
private:
	std::wstring LEADERSHIP_RELATION_NAME;
	std::wstring LEADERSHIP_PERSON_ROLE;
	std::wstring LEADERSHIP_ORG_ROLE;

	std::wstring EMPLOYS_RELATION_NAME;
	std::wstring EMPLOYS_PERSON_ROLE;
	std::wstring EMPLOYS_ORG_ROLE;

	std::wstring MEMBER_PERSON_RELATION_NAME;
	std::wstring MEMBER_PERSON_PERSON_ROLE;
	std::wstring MEMBER_PERSON_ORG_ROLE;

	std::wstring MEMBER_RELATION_NAME;
	std::wstring MEMBER_PERSON_ROLE;
	std::wstring MEMBER_ORG_ROLE;
};

class EmploymentPerOrgFilter : public EIFilter {
public:
	EmploymentPerOrgFilter() : EIFilter(L"EmploymentPerOrgFilter",true) {
		addRelationMatch(L"eru:employs");
	}
	~EmploymentPerOrgFilter() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class LocationFromSubSuperFilter : public EIFilter {
public:
	LocationFromSubSuperFilter() : EIFilter(L"LocationFromSubSuperFilter",true) {}
	~LocationFromSubSuperFilter() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class PersonGroupEntityTypeFilter : public EIFilter {
public:
	PersonGroupEntityTypeFilter() : EIFilter(L"PersonGroupEntityTypeFilter",true,true) {}
	~PersonGroupEntityTypeFilter() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
	void processIndividual(EIDocData_ptr docData, const ElfIndividual_ptr individual);
};

//KBP Filters
class RemoveUnusedIndividualsFilter : public EIFilter {
public:
	RemoveUnusedIndividualsFilter() : EIFilter(L"RemoveUnusedIndividualsFilter",false,false,true) {}
	~RemoveUnusedIndividualsFilter() {}
	void processAll(EIDocData_ptr docData);
};

class RemovePersonGroupRelations : public EIRemovalFilter {
public:
	RemovePersonGroupRelations() : EIRemovalFilter(L"RemovePersonGroupRelations",true) {
		addRelationMatch(L"eru:BirthEvent");
		addRelationMatch(L"eru:DeathEvent");
		addRelationMatch(L"eru:AttendsSchool");
		addRelationMatch(L"eru:HasSpouse");
		addRelationMatch(L"eru:ResidesInGPE-spec");
		addRelationMatch(L"eru:HasEmployer");
		addRelationMatch(L"eru:PersonTitleInOrganization");
		addRelationMatch(L"eru:BelongsToHumanOrganization");
		addRelationMatch(L"eru:HasTopMemberOrEmployee");
		addRelationMatch(L"eru:IsAffiliateOf");
	}
	~RemovePersonGroupRelations() {}
	bool doRemoveRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class AddTitleSubclassFilter : public EIFilter {
public:
	AddTitleSubclassFilter() : EIFilter(L"AddTitleSubclassFilter",true) {
		addRelationMatch(L"eru:PersonTitleInOrganization");
	}
	~AddTitleSubclassFilter() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class RenameMembershipEmployment : public EIFilter {
public: 
	RenameMembershipEmployment() : EIFilter(L"RenameMembershipEmployment",true){
		addRelationMatch(L"eru:IsAffiliateOf");
	}
	~RenameMembershipEmployment() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class KBPTemporalFilter : public EIFilter {
public:
	KBPTemporalFilter() : EIFilter(L"KBPTemporalFilter",false,false,true) {
		addRelationMatch(L"eru:BirthEvent");
		addRelationMatch(L"eru:DeathEvent");
		addRelationMatch(L"eru:AttendsSchool");
		addRelationMatch(L"eru:HasSpouse");
		addRelationMatch(L"eru:ResidesInGPE-spec");
		addRelationMatch(L"eru:HasEmployer");
		addRelationMatch(L"eru:PersonTitleInOrganization");
		addRelationMatch(L"eru:BelongsToHumanOrganization");
		addRelationMatch(L"eru:HasTopMemberOrEmployee");
		addRelationMatch(L"eru:IsAffiliateOf");
	}
	~KBPTemporalFilter() {}
	void processAll(EIDocData_ptr docData);
};

class KBPConflictingDateTemporalFilter : public EIFilter {
public:
	KBPConflictingDateTemporalFilter() : EIFilter(L"KBPConflictingDateTemporalFilter",false,false,true) {
		addRelationMatch(L"eru:BirthEvent");
		addRelationMatch(L"eru:DeathEvent");
		addRelationMatch(L"eru:AttendsSchool");
		addRelationMatch(L"eru:HasSpouse");
		addRelationMatch(L"eru:ResidesInGPE-spec");
		addRelationMatch(L"eru:HasEmployer");
		addRelationMatch(L"eru:PersonTitleInOrganization");
		addRelationMatch(L"eru:BelongsToHumanOrganization");
		addRelationMatch(L"eru:HasTopMemberOrEmployee");
		addRelationMatch(L"eru:IsAffiliateOf");
	}
	~KBPConflictingDateTemporalFilter() {}
	void processAll(EIDocData_ptr docData);
};

class KBPBirthDeathTemporalFilter : public EIFilter{
public:
	KBPBirthDeathTemporalFilter() : EIFilter(L"KBPBirthDeathTemporalFilter",false,false,true) {
		addRelationMatch(L"eru:BirthEvent");
		addRelationMatch(L"eru:DeathEvent");
	};
	~KBPBirthDeathTemporalFilter(){}
	void processAll(EIDocData_ptr docData);
};

class KBPConflictingBirthDeathTemporalFilter : public EIFilter {
public:
	KBPConflictingBirthDeathTemporalFilter() : EIFilter(L"KBPConflictingBirthDeathTemporalFilter",false,false,true) {
		addRelationMatch(L"eru:AttendsSchool");
		addRelationMatch(L"eru:HasSpouse");
		addRelationMatch(L"eru:ResidesInGPE-spec");
		addRelationMatch(L"eru:HasEmployer");
		addRelationMatch(L"eru:PersonTitleInOrganization");
		addRelationMatch(L"eru:BelongsToHumanOrganization");
		addRelationMatch(L"eru:HasTopMemberOrEmployee");
		addRelationMatch(L"eru:IsAffiliateOf");
	}
	~KBPConflictingBirthDeathTemporalFilter() {}
	void processAll(EIDocData_ptr docData);
};

class InferHoldsThroughout : public EIFilter {
public:
	InferHoldsThroughout() : EIFilter(L"InferHoldsThroughout",false,false,true) {}
	~InferHoldsThroughout() {}
	void processAll(EIDocData_ptr docData);
private:
	bool nonTemporalArgumentsMatch(ElfRelation_ptr rel1, ElfRelation_ptr rel2);
	bool containsArguments(std::vector<ElfRelationArg_ptr>& args1,
			std::vector<ElfRelationArg_ptr>& args2);
	void copyTemporalArguments(EIDocData_ptr docData, ElfRelation_ptr rel1, 
			ElfRelation_ptr rel2);
	std::vector<ElfRelationArg_ptr> temporalArguments(ElfRelation_ptr rel);
	std::vector<ElfRelationArg_ptr> nonTemporalArguments(ElfRelation_ptr rel);
	std::vector<ElfRelationArg_ptr> argsOfType(ElfRelation_ptr ret, bool temporalOrNot);
	std::wstring stringRep(EIDocData_ptr docData, ElfRelationArg_ptr arg);
	EIUtils::DatePeriodPair calcHWSpan(const std::wstring& cb, 
			const std::wstring& cf, bool& bad_clipping);
	bool addMissingArguments(const std::vector<ElfRelationArg_ptr> sourceArgs,
			const std::vector<ElfRelationArg_ptr> destArgs,
			ElfRelation_ptr destRel);
};

class KBPMatchLearnedAndManualDatesFilter : public EIFilter {
public:
	KBPMatchLearnedAndManualDatesFilter() : EIFilter(L"KBPMatchLearnedAndManualDatesFilter",true) {
		addRelationMatch(L"eru:AttendsSchool");
		addRelationMatch(L"eru:HasSpouse");
		addRelationMatch(L"eru:ResidesInGPE-spec");
		addRelationMatch(L"eru:HasEmployer");
		addRelationMatch(L"eru:PersonTitleInOrganization");
		addRelationMatch(L"eru:BelongsToHumanOrganization");
		addRelationMatch(L"eru:HasTopMemberOrEmployee");
		addRelationMatch(L"eru:IsAffiliateOf");
	}
	~KBPMatchLearnedAndManualDatesFilter() {}
	void processAll(EIDocData_ptr docData);
};

class CopyPTIOEmploymentMembershipFilter : public EIFilter {
public:
	CopyPTIOEmploymentMembershipFilter() : EIFilter(L"CopyPTIOEmploymentMembershipFilter",true) {
		addRelationMatch(L"eru:PersonTitleInOrganization");
		addRelationMatch(L"eru:PersonInOrganization");
		addRelationMatch(L"eru:HasTopMemberOrEmployee");
	}
	 ~CopyPTIOEmploymentMembershipFilter() {}
	 void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class BoundTitleFilter : public EIFilter {
public:
	BoundTitleFilter() : EIFilter(L"BoundTitleFilter",true) {
		addRelationMatch(L"eru:PersonTitleInOrganization");
	}
	~BoundTitleFilter() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
private:
	bool titleSubClass(ElfType_ptr type);
};

class DuplicateRelationFilter : public EIFilter {
public:
	DuplicateRelationFilter() : EIFilter(L"DuplicateRelationFilter",false,false,true) {}
	~DuplicateRelationFilter() {}
	void processAll(EIDocData_ptr docData);
};

class DuplicateArgFilter : public EIFilter {
public:
	DuplicateArgFilter() : EIFilter(L"DuplicateArgFilter",true) {}
	~DuplicateArgFilter() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class KBPStartEndFilter : public EIFilter {
public:
	KBPStartEndFilter() : EIFilter(L"KBPStartEndFilter",true) {}
	~KBPStartEndFilter() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class KBPCompletePTIOFilter : public EIFilter {
public:
	KBPCompletePTIOFilter() : EIFilter(L"KBPCompletePTIOFilter",true) {
		addRelationMatch(L"eru:PersonTitleInOrganization");
		addRelationMatch(L"eru:PersonInOrganization");
	}
	~KBPCompletePTIOFilter() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
private:
	bool addMissingPersonToPTIO(ElfRelation_ptr rel,
		ElfRelationArg_ptr title, EIDocData_ptr docData);
	bool addMissingTitleToPTIO(ElfRelation_ptr rel,
		ElfRelationArg_ptr person, EIDocData_ptr docData);
	void trimTitleOfPTIO(ElfRelation_ptr rel,
		ElfRelationArg_ptr title, EIDocData_ptr docData);
	void deleteBadTitle(ElfRelationArg_ptr titleArg, ElfRelation_ptr rel, EIDocData_ptr docData); 
};

class MakePTIOsFilter : public EIFilter {
public:
	MakePTIOsFilter() : EIFilter(L"MakePTIOsFilter",true) {
		addRelationMatch(L"eru:HasEmployer");
		addRelationMatch(L"eru:BelongsToHumanOrganization");
		addRelationMatch(L"eru:HasTopMemberOrEmployee");
	}
	~MakePTIOsFilter() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class BrokenArgFilter : public EIFilter {
public:
	BrokenArgFilter() : EIFilter(L"BrokenArgFilter",true) {}
	~BrokenArgFilter() {}
	bool checkCoref(ElfRelationArg_ptr target, std::vector<ElfRelationArg_ptr> coArgs, std::vector<ElfRelationArg_ptr> notCoArgs); 
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class MarriageWithSetOrOfficialFilter : public EIFilter {
public:
	MarriageWithSetOrOfficialFilter() : EIFilter(L"MarriageWithSetOrOffialFilter",true) {
		addRelationMatch(L"eru:HasSpouse");
	}
	~MarriageWithSetOrOfficialFilter() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class KBPMinisterGPEFilter : public EIFilter {
public:
	KBPMinisterGPEFilter() : EIFilter(L"KBPMinisterGPEFilter",true) {
		addRelationMatch(L"eru:PersonTitleInOrganization");
	}
	~KBPMinisterGPEFilter() {}
	void processRelation(EIDocData_ptr docData, const ElfRelation_ptr relation);
};

class FocusGPEResidenceFilter : public EIFilter {
public:
	FocusGPEResidenceFilter() : EIFilter(L"FocusGPEResidentFilter",false,false,true) {}
	~FocusGPEResidenceFilter()  {}
	void processAll(EIDocData_ptr docData);
};

class CrazyBoundIDFilter : public EIFilter {
public:
	CrazyBoundIDFilter(); 
	~CrazyBoundIDFilter() {}
	void processAll(EIDocData_ptr docData);
private:
	void candidateCrazyRelation(EIDocData_ptr docData, const std::wstring& boundID, 
			ElfRelation_ptr relation, ElfRelationArg_ptr arg);
	bool boundIDFoundInSentenceWith(EIDocData_ptr docData, const std::wstring& boundID,
			ElfRelationArg_ptr arg);

	bool _same_sentence_only;
	bool _displace_xdoc;
	bool _even_if_already_found_one;
};

class AddTitleXDocIDsFilter : public EIFilter {
public:
	AddTitleXDocIDsFilter() : EIFilter(L"AddTitleXDocIDsFilter",false,false,true) {}
	~AddTitleXDocIDsFilter()  {}
	void processAll(EIDocData_ptr docData);
};
