// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef PROFILE_GENERATOR_H
#define PROFILE_GENERATOR_H

#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include "ProfileGenerator/Profile.h"
#include "ProfileGenerator/PGDatabaseManager.h"

#include <vector>
#include <list>
#include <ostream>
#include <set>

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(ProfileGenerator);
BSP_DECLARE(PGDatabaseManager);
BSP_DECLARE(Fact);
BSP_DECLARE(NameHypothesis);
BSP_DECLARE(ProfileConfidence);

/*! \brief This class is responsible for creating and managing new profiles
     It contains methods for generating profiles and uploading them to an 
     external profile database.
*/
class ProfileGenerator
{
public:

	~ProfileGenerator();

	static const int MIN_GOOD_QUOTE_LEN = 60;
	static const float FREQUENCY_CUTOFF_PERCENT;

	friend ProfileGenerator_ptr boost::make_shared<ProfileGenerator>(PGDatabaseManager_ptr const&, ProfileConfidence_ptr const&);

	Profile_ptr generateFullProfile(std::wstring entityName, Profile::profile_type_t profileType = Profile::UNKNOWN);	
	Profile_ptr generateFullProfile(PGActorInfo_ptr target_actor);
	
	void populateProfile(Profile_ptr prof);	

	bool uploadProfile(Profile_ptr prof);

	static std::wstring normalizeName(std::wstring name);
	static std::wstring tokenizeName(std::wstring name);
	
private:
	ProfileGenerator(PGDatabaseManager_ptr pgdm, ProfileConfidence_ptr pc);
	PGDatabaseManager_ptr _pgDatabaseManager;
	ProfileConfidence_ptr _confModel;

	xercesc::DOMDocument* _profileSlotDocument;
	std::map<std::string, xercesc::DOMElement*> _profileSlotElements;

	void generateSlot(Profile_ptr prof, ProfileSlot_ptr slot, std::vector<PGFact_ptr>& allFacts);

	// A cache of normalized names, since otherwise we waste a ton of time here for big profiles
	static std::map<std::wstring, std::wstring> _normalizedNameCache;

	// the number of times to retry a profile before giving up
	int _num_retries;	

	// for debugging, primarily
	bool _write_profile_to_html;
	
};

#endif
