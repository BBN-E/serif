// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef PROFILE_SLOT_H
#define PROFILE_SLOT_H

#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include <boost/enable_shared_from_this.hpp>
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#include <boost/regex.hpp>
#pragma warning(pop)
#include "Generic/common/XMLUtil.h"
#include <xercesc/dom/DOM.hpp>

#include <list>
#include <vector>
#include <sstream>
#include <string>

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(Profile);
BSP_DECLARE(PGDatabaseManager);
BSP_DECLARE(PGFact);
BSP_DECLARE(PGFactDate);
BSP_DECLARE(ProfileSlot);
BSP_DECLARE(GenericHypothesis);
BSP_DECLARE(NameHypothesis);

/*! \brief An enum indicating ProfileGenerator's confidence in a row of data.
*/
enum Confidence {LOW, MED, HIGH};

/*! \brief Responsible for maintaining, evaluating, and merging SlotHypotheses.
    It defines the criteria for uploading data for the various slot type and
    also maps slot types to the appropriate GenericHypothesis subclasses.
*/
class ProfileSlot : public boost::enable_shared_from_this<ProfileSlot>
{
public:
	friend ProfileSlot_ptr boost::make_shared<ProfileSlot>(xercesc::DOMElement* const&, PGDatabaseManager_ptr const&);
	void addFact(PGFact_ptr fact, PGDatabaseManager_ptr pgdm);

	typedef enum { DOCUMENT, EXTERNAL, FAKE } db_fact_type_t;

	typedef struct {
		// The fact type in the fact DB for this slot
		int fact_type;
		
		// The id for the extractor or external kb who generated this fact type
		std::pair<db_fact_type_t, int> source_info;
		
		// The role that the focus of this fact should play in this relation, e.g. "Actor" for Description facts
		std::string focus_role;

		// The role that represents the "answer" for this fact type
		std::string answer_role;

		// Mappings of secondary roles to primary roles
		std::map<std::string, std::string> kb_role_map;
	} DatabaseFactInfo;
	
	bool isSimilar(ProfileSlot_ptr that);
	bool isInfoboxSlot() { return _is_infobox; }	
	bool isBinaryRelation() { return _is_binary_relation; }
	std::string getUniqueId() { return _unique_id; }
	int getKBFactTypeId() { return _kb_fact_type_id; }
	std::string getDisplayName() { return _displayName; }
	std::string getDisplayType() { return _displayType; }
	std::string getSortOrder() { return _sortOrder;}
	int getRank() { return _rank; }
	static const unsigned int hyp_ordinal_size = 10;
	static char *hyp_ordinals[];
	std::list<GenericHypothesis_ptr>& getOutputHypotheses() { return _outputHypotheses; }
	const std::list<GenericHypothesis_ptr>& getRawHypotheses() { return _rawHypotheses; }
	void generateOutputHypotheses(Profile_ptr existingProfile, bool print_to_screen);
	void makeOutOfDate() { _output_up_to_date = false; }
	void generateSlotDates();
	bool isPerson() { return _is_person; }
	bool isFamily() { return _is_family; }
	bool isLocation() { return _is_location; }
	bool useSpokespeople() { return _use_spokespeople; }
	std::list<DatabaseFactInfo>& getDatabaseFactInfoList() { return _databaseFactInfoList; }
	std::string getFocusRole() { return _focus_role; }
	std::string getAnswerRole() { return _answer_role; }

	// These are technically set in the XML file, but they are common enough that 
	//   we need to be able to handle them in the code, and we'd rather define them here
	//   then check for the actual string in the code.
	static const std::string ACTIONS_UNIQUE_ID;
	static const std::string ASSOCIATE_UNIQUE_ID;
	static const std::string BIRTHDATE_UNIQUE_ID;
	static const std::string DEATHDATE_UNIQUE_ID;
	static const std::string DESCRIPTION_UNIQUE_ID;
	static const std::string EDUCATION_UNIQUE_ID;
	static const std::string EMPLOYEE_UNIQUE_ID;
	static const std::string EMPLOYER_UNIQUE_ID;
	static const std::string FAMILY_UNIQUE_ID;
	static const std::string FOUNDER_UNIQUE_ID;
	static const std::string FOUNDINGDATE_UNIQUE_ID;
	static const std::string HEADQUARTERS_UNIQUE_ID;
	static const std::string LEADER_UNIQUE_ID;
	static const std::string NATIONALITY_UNIQUE_ID;
	static const std::string QUOTES_ABOUT_UNIQUE_ID;
	static const std::string QUOTES_BY_UNIQUE_ID;
	static const std::string SPOUSE_UNIQUE_ID;
	static const std::string TEASER_BIO_UNIQUE_ID;
	static const std::string VISIT_UNIQUE_ID;

	static const std::string KB_EMPLOYMENT_FACT_TYPE;
	static const std::string KB_EMPLOYEE_ROLE;
	static const std::string KB_EMPLOYER_ROLE;
	
	bool isQuotation() { return _unique_id == QUOTES_BY_UNIQUE_ID || _unique_id == QUOTES_ABOUT_UNIQUE_ID; }
	bool isDistantPastDate() { return _unique_id == BIRTHDATE_UNIQUE_ID || _unique_id == QUOTES_ABOUT_UNIQUE_ID; }		
	bool isSentiment() { return _unique_id.find("sentiment") != std::string::npos; }

private:
	ProfileSlot(xercesc::DOMElement* slot, PGDatabaseManager_ptr pgdm);
	void addDatabaseFactInfo(xercesc::DOMElement* elt, PGDatabaseManager_ptr pgdm);
	void addRoleMapping(DatabaseFactInfo& dfi, std::string spec);

	static const int SMALL_PROFILE_FACTS_COUNT;

	GenericHypothesis_ptr makeNewHypothesis(PGFact_ptr fact, PGDatabaseManager_ptr pgdm);
	
	std::list<GenericHypothesis_ptr>  _rawHypotheses;

	// Functions for generating the set of output hypotheses
	bool _output_up_to_date;
	std::list<GenericHypothesis_ptr> _outputHypotheses;
	GenericHypothesis_ptr getBestHypothesis(std::list<GenericHypothesis_ptr>& unusedHypotheses);
	GenericHypothesis_ptr getBestEmploymentHypothesis(std::list<GenericHypothesis_ptr>& unusedHypotheses, int status);
	void printHypothesisToScreen(Profile_ptr existingProfile, std::string status, GenericHypothesis_ptr hypo);
	void removeHypothesis(GenericHypothesis_ptr hypo, std::list<GenericHypothesis_ptr>& hypotheses);
	bool reachesCutoff(GenericHypothesis_ptr hypo);
	bool isIllegalHypothesis(GenericHypothesis_ptr hypo, std::string& rationale);
	bool isValidHypothesis(GenericHypothesis_ptr bestHypothesis, Profile_ptr existingProfile, int n_foreign_hypotheses_uploaded, bool secondary_hypothesis, bool print_to_screen);

	// For ST_DESCRIPTION
	std::vector<NameHypothesis_ptr> _namesAlreadyDisplayed;
	std::vector<std::wstring> _titlesAlreadyDisplayed;
	std::vector<NameHypothesis_ptr> _nationalitiesAlreadyDisplayed;

	// The name that distinguishes this slot from all others
	std::string _unique_id;

	// Sort order
	std::string _sortOrder;

	// whether this slot belongs in an infoboxin a UI display
	bool _is_infobox;

	// whether this slot should show up in the UI graph view
	bool _is_binary_relation;

	// whether this slot is always a person / family member / location
	bool _is_person;
	bool _is_family;
	bool _is_location;

	// whether to include spokespeople as agents for this slot
	bool _use_spokespeople;

	// minimum facts ratio
	float _min_facts_ratio;

	// The type of hypothesis to be generated for this type of slot
	std::string _hypothesisType;

	// The ways facts for this slot might be stored in the DB (document-level or external)
	std::list<DatabaseFactInfo> _databaseFactInfoList;

	// The way this fact should be stored as a KBFact
	int _kb_fact_type_id;
	std::string _focus_role;
	std::string _answer_role;

	// The display name for this slot type
	std::string _displayName;
	std::string _displayType;

	// The rank of this slot relative to other slots
	int _rank;

	// The maximum number of hypotheses (data) rows to upload for this slot type (-1 if no cutoff applied)
	int _max_uploaded_hypoth;

	// For debugging
	bool _print_all_hypotheses_to_html;

	void printDate(std::ostringstream& ostr, std::string header, PGFactDate_ptr date);
	void printDates(std::ostringstream& ostr, std::string header, std::vector<PGFactDate_ptr>& dates);
};

#endif
