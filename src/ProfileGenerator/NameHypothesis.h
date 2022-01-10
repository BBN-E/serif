// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef NAME_HYPOTHESIS_H
#define NAME_HYPOTHESIS_H

#include "ProfileGenerator/GenericHypothesis.h"
#include "ProfileGenerator/PGFact.h"
#include "ProfileGenerator/Profile.h"
#include "ProfileGenerator/ProfileSlot.h"
#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include "boost/enable_shared_from_this.hpp"
#include <set>
#include <vector>

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(NameHypothesis);
BSP_DECLARE(PGDatabaseManager);

/*! \brief An implementation of GenericHypothesis for slot types that expect an
    entity name for a value. It makes use of XDoc lite information when 
    checking facts for equivalency.
*/
class NameHypothesis : public GenericHypothesis, public boost::enable_shared_from_this<NameHypothesis>
{
public:
	friend NameHypothesis_ptr boost::make_shared<NameHypothesis>(PGFact_ptr const&, ProfileSlot_ptr const&, PGDatabaseManager_ptr const&);
	friend NameHypothesis_ptr boost::make_shared<NameHypothesis>(int const &, std::wstring const&, bool const&, PGDatabaseManager_ptr const&);
	
	~NameHypothesis(void) { }

	typedef std::map<std::wstring, double> ActorStringConfidenceMap;

	void addSupportingHypothesis(GenericHypothesis_ptr hypo);

	// TODO: Not sure that "equiv and better" is possible in new actor-id world
	bool isEquivAndBetter(GenericHypothesis_ptr hypoth) {
		NameHypothesis_ptr nameHypoth = boost::dynamic_pointer_cast<NameHypothesis>(hypoth);
		if (nameHypoth == NameHypothesis_ptr()) // cast failed
			return false;
		ComparisonType comp = compareValues(nameHypoth);
		return (comp == BETTER);
	}
	bool isEquiv(GenericHypothesis_ptr hypoth) {
		NameHypothesis_ptr nameHypoth = boost::dynamic_pointer_cast<NameHypothesis>(hypoth);
		if (nameHypoth == NameHypothesis_ptr()) // cast failed
			return false;
		ComparisonType comp = compareValues(nameHypoth);
		return (comp == EQUAL || comp == BETTER);
	}
	bool isSimilar(GenericHypothesis_ptr hypoth) {
		// NOTE: This function is NOT reciprocal, and should be called each way if you want that behavior
		NameHypothesis_ptr nameHypoth = boost::dynamic_pointer_cast<NameHypothesis>(hypoth);
		if (nameHypoth == NameHypothesis_ptr()) // cast failed
			return false;		
		ComparisonType comp = compareValues(nameHypoth);
		return (comp == EQUAL || comp == BETTER || comp == SIMILAR || comp == VERY_SIMILAR);
	}	
	std::vector<kb_arg_t> getKBArguments(int actor_id, ProfileSlot_ptr slot);
	
	typedef enum { SUBJECT, OBJECT, POSSESSIVE_MOD, POSSESSIVE, PRONOUN_ROLES_SIZE } PronounRole;
	typedef enum { UNKNOWN, INANIMATE, SECOND, MALE, FEMALE, PLURAL_ORG, GENDER_TYPES_SIZE } GenderType;
	GenderType getGender() { return _gender; }

	int getActorId() { return _actor_id; }
	std::wstring getDisplayValue();
	std::wstring getNormalizedValue() { return _normalizedValue; }
	std::wstring getShortFormalName() { return _shortFormalName; }
	std::map<std::wstring, int>& getPossibleDisplayValues() { return _possibleDisplayValues; }
	ActorStringConfidenceMap& getActorStrings() { return _actorStrings; }
	std::wstring getRepeatReference(int& counter, PronounRole role);

	bool isIllegalHypothesis(ProfileSlot_ptr slot, std::string& rationale);
	bool isRiskyHypothesis(ProfileSlot_ptr slot, std::string& rationale);
	int rankAgainst(GenericHypothesis_ptr hypo);
	bool matchesProfileName(Profile_ptr profile);
	bool matchesName(std::wstring test_name);
	bool isFamily() { return _slot != ProfileSlot_ptr() && _slot->isFamily(); }
	bool isPersonSlot() { return _is_person_slot; }
	
	static void loadGenderNameLists();

private:	
	NameHypothesis(PGFact_ptr fact, ProfileSlot_ptr slot, PGDatabaseManager_ptr pgdm);
	NameHypothesis(int actor_id, std::wstring canonical_name, bool is_person_slot, PGDatabaseManager_ptr pgdm);
	
	PGDatabaseManager_ptr _pgdm;
	int _actor_id;
	std::wstring _canonical_name;
	bool _is_person_slot;
	ActorStringConfidenceMap _actorStrings;

	bool _display_value_up_to_date;
	std::map<std::wstring, int> _possibleDisplayValues;
	std::wstring _displayValue;
	std::wstring _normalizedValue;
	std::wstring _shortFormalName; 
	GenderType _gender;
	ProfileSlot_ptr _slot;
	
	// Initialization functions
	void initialize();
	void initalizeStaticVectors();	
	void addNameString(std::wstring name, double confidence);
	void addNameVariants(std::wstring name, double confidence);
	void setPersonShortFormalName(std::wstring candidate);

	// Name comparison driver
	typedef enum { BETTER, EQUAL, VERY_SIMILAR, SIMILAR, NONE } ComparisonType;
	ComparisonType compareValues(NameHypothesis_ptr otherNameHypothesis);
	
	// Name manipulation functions & resources
	static std::set<std::wstring> _maleFirstNames;
	static std::set<std::wstring> _femaleFirstNames;
	static std::vector<std::wstring> _honorifics;
	static std::vector<std::wstring> _prefixHonorifics;
	static std::set<std::wstring> _lowercaseNameWords;
	std::wstring getLastTwoNames(std::vector<std::wstring>& tokens) const;
	std::wstring stripHonorifics(std::wstring name);
	std::wstring stripAlPrefix(std::wstring name);

	// Gender
	static const std::wstring pro_unknown[], pro_inanimate[], pro_second[], pro_male[], pro_female[], pro_plural[];
	std::wstring getPronounString(PronounRole role, GenderType gender_type);
	GenderType guessGenderFromFirstName(std::wstring personName);
	
	std::wstring getSimpleCapitalization(std::wstring phrase);
	std::wstring fixLowercaseNameWords(std::wstring phrase, bool is_short_name);
	void fixDisplayCapitalization();
	void updateDisplayValue();

	// last name except when preceded by "marker" that goes with last; "Johnson" or "bin Laden"
	// for ORGs can be "Apple" for "Apple Corporation" and "SRI" for "SRI International"

};

#endif

