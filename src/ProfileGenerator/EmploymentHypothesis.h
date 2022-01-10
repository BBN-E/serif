// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef EMPLOYMENT_HYPOTHESIS_H
#define EMPLOYMENT_HYPOTHESIS_H

#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include <set>

#include "ProfileGenerator/GenericHypothesis.h"
#include "ProfileGenerator/NameHypothesis.h"

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(EmploymentHypothesis);

/*! \brief An implementation of GenericHypothesis for slot types that expect an
    entity name (employee or employer) and a job title. 
*/
class EmploymentHypothesis : public GenericHypothesis
{
public:
	typedef enum { EMPLOYER, EMPLOYEE } employment_argument_type_t;	
	friend EmploymentHypothesis_ptr boost::make_shared<EmploymentHypothesis>(PGFact_ptr const&, 
		employment_argument_type_t const&, PGDatabaseManager_ptr const&);
	~EmploymentHypothesis(void) { }

	// Static class-specific functions
	static void loadJobTitles();

	// Virtual parent class functions, implemented here
	void addSupportingHypothesis(GenericHypothesis_ptr hypo);
	void addHypothesisSpecificFact(PGFact_ptr fact);
	bool isEquiv(GenericHypothesis_ptr hypoth);
	bool isSimilar(GenericHypothesis_ptr hypoth);
	int rankAgainst(GenericHypothesis_ptr hypoth);
	std::wstring getDisplayValue();
	bool isIllegalHypothesis(ProfileSlot_ptr slot, std::string& rationale);
	bool excludeQuestionableHoldDates();
	std::vector<kb_arg_t> getKBArguments(int actor_id, ProfileSlot_ptr slot);

	// Employer vs. Employee
	bool isEmployerHypothesis() { return _employment_argument_type == EMPLOYER; }
	bool isEmployeeHypothesis() { return _employment_argument_type == EMPLOYEE; }

	// Tense of job status
	typedef enum { UNSET, PAST, CURRENT, FUTURE } status_tense_t;
	status_tense_t getTense(bool print_debug = false);	
	static bool isPastStatusWord(std::wstring word);
	static bool isPresentStatusWord(std::wstring word);
	static bool isFutureStatusWord(std::wstring word);

	// Getters
	NameHypothesis_ptr getNamedArgument() { return _namedArgument; }
	std::wstring& getJobTitle();
	bool isTopBrass();
	bool isSpokesman();
	bool hasSecondaryTitleWord();
	bool employerIsPoliticalParty();

	// For knowing about reliability from the employee's point of view
	bool isReliableFromEmployeePOV() {return _reliable_from_employee_pov; }
	void setReliableFromEmployeePOV(bool reliable) { _reliable_from_employee_pov = reliable; }

	
private:
	EmploymentHypothesis(PGFact_ptr fact, employment_argument_type_t arg_type, PGDatabaseManager_ptr pgdm);

	// Class-specific member variables
	PGDatabaseManager_ptr _pgdm;
	employment_argument_type_t _employment_argument_type;
	NameHypothesis_ptr _namedArgument;	
	bool _reliable_from_employee_pov;
	int _num_reliable_titles_seen;
	bool _is_spokesman;
	std::wstring _job_title;
	bool _job_title_up_to_date;
	bool _is_top_brass;
	std::map<std::wstring, int> _jobTitleCounts;	
	std::map<std::wstring, int> _statusCounts;	
	
	// Class-specific helpers
	bool isEquivJobTitle(std::wstring& otherJobTitle);
	bool isEquivNamedArgument(NameHypothesis_ptr& otherNamedArgument);
	bool isSimilarNamedArgument(NameHypothesis_ptr& otherNamedArgument);
	void updateJobTitleInfo();

	static const std::string TITLE_ROLE;
	static const std::string STATUS_ROLE;

	// Static info about job titles (for display purposes and top brass vs. employee)
	static std::map<std::wstring, int> _rankedJobTitles;
	static int rankOfJobTitle(std::wstring jt);
	static int rankJobTitles(std::wstring jt1, std::wstring jt2);
	static std::wstring capitalizeTitle(std::wstring title); 
	static bool jobTitlesEquiv(const std::wstring job_title, const std::wstring otherJobTitle);
	
};

#endif
