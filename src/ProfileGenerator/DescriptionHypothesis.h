// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef DESCRIPTION_HYPOTHESIS_H
#define DESCRIPTION_HYPOTHESIS_H

#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include "boost/enable_shared_from_this.hpp"

#include "ProfileGenerator/GenericHypothesis.h"
#include "ProfileGenerator/NameHypothesis.h"
#include "ProfileGenerator/SimpleStringHypothesis.h"

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(DescriptionHypothesis);

class DescriptionHypothesis :
	public GenericHypothesis, public boost::enable_shared_from_this<DescriptionHypothesis>
{
public:
	friend DescriptionHypothesis_ptr boost::make_shared<DescriptionHypothesis>(PGFact_ptr const&, PGDatabaseManager_ptr const&);
	~DescriptionHypothesis(void) { }

	// Static class-specific functions
	static void loadGenderDescLists();
	static void gatherExistingProfileInformation(Profile_ptr existing_profile,
		std::vector<NameHypothesis_ptr>& namesAlreadyDisplayed,
		std::vector<std::wstring>& titlesAlreadyDisplayed,
		std::vector<NameHypothesis_ptr>& nationalitiesAlreadyDisplayed);
	
	// Virtual parent class functions, implemented here
	void addSupportingHypothesis(GenericHypothesis_ptr hypo);
	bool isRiskyHypothesis(ProfileSlot_ptr slot, std::string& rationale);
	int rankAgainst(GenericHypothesis_ptr hypo);	
	bool isEquiv(GenericHypothesis_ptr hypoth);
	bool isSimilar(GenericHypothesis_ptr hypoth);
	std::wstring getDisplayValue();
	bool isIllegalHypothesis(ProfileSlot_ptr slot, std::string& rationale);

	// Class-specific functions
	bool isSpecialIllegalHypothesis(std::string& rationale,
		std::vector<NameHypothesis_ptr>& namesAlreadyDisplayed,
		std::vector<std::wstring>& titlesAlreadyDisplayed,
		std::vector<NameHypothesis_ptr>& nationalitiesAlreadyDisplayed,
		NameHypothesis::GenderType gender);
	void dump_to_screen();

	// Class-specific getters & simple booleans
	std::wstring getHeadword() { return _headword; }
	std::vector<NameHypothesis_ptr>& getMentionArguments() { return _mentionArguments; }
	std::vector<SimpleStringHypothesis_ptr>& getNonMentionArguments() { return _nonMentionArguments; }
	int getNModifiers() { return static_cast<int>(_mentionArguments.size() + _nonMentionArguments.size()); }
	PGFact_ptr getCoreFact() { return _coreFact; }
	bool isTitle() { return _is_title; }
	bool isCopula() { return _is_copula; }
	bool isAppositive() { return _is_appositive; }
	bool hasMentionModifier(NameHypothesis_ptr hypo);
	bool hasNonMentionModifier(SimpleStringHypothesis_ptr hypo);

private:
	DescriptionHypothesis(PGFact_ptr fact, PGDatabaseManager_ptr pgdm);
	
	// Class-specific member variables
	PGFact_ptr _coreFact;
	void setCoreFact(PGFact_ptr fact);
	std::wstring _display_value;
	bool _is_title;
	bool _is_copula;
	bool _is_appositive;
	int _max_equivalence_distance;
	std::wstring _headword;
	std::vector<NameHypothesis_ptr> _mentionArguments;
	std::vector<SimpleStringHypothesis_ptr> _nonMentionArguments;

	// Helper functions
	bool isEquivHeadword(std::wstring otherHeadword);
	bool isGenericORGHeadword(std::wstring headword);
	bool isGenericPERHeadword(std::wstring headword);
	void updateCountsForNewFact(PGFact_ptr fact);
	bool hasOverlappingModifiers(DescriptionHypothesis_ptr otherDesc);
	bool isSubsumedBy(DescriptionHypothesis_ptr otherDesc);	

	// Static member variables
	static std::set<std::wstring> _maleDescriptions;
	static std::set<std::wstring> _femaleDescriptions;

};

#endif
