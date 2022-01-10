// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef PROFILE_H
#define PROFILE_H

#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include "boost/enable_shared_from_this.hpp"

#include <map>
#include <set>

#include "Generic/common/bsp_declare.h"
#include "Generic/common/UTF8OutputStream.h"
BSP_DECLARE(Profile);
BSP_DECLARE(PGDatabaseManager);
BSP_DECLARE(NameHypothesis);
BSP_DECLARE(ProfileConfidence);

#include "ProfileGenerator/ProfileSlot.h"

typedef std::map<std::string, ProfileSlot_ptr> ProfileSlotMap;

/*! \brief Holds data for one profile.  It maintains a collection of 
    ProfileSlot objects. 
*/
class Profile : public boost::enable_shared_from_this<Profile>
{
public:
	// Profile type management
	typedef enum { PER, ORG, GPE, PSN, UNKNOWN } profile_type_t;
	static std::string getStringForProfileType(profile_type_t profileType);
	static profile_type_t getProfileTypeForString(std::string str);

	// Constructor
	friend Profile_ptr boost::make_shared<Profile>(int const&, std::wstring const&, profile_type_t const&, PGDatabaseManager_ptr const&);
	~Profile();

	// Basic getters
	int getActorId() { return _actor_id; }
	std::vector<int>& getSpokespeopleIds() { return _spokespeopleActorIds; }
	std::wstring getName() {return _canonical_actor_name;}
	profile_type_t getProfileType () { return _profile_type;}
	NameHypothesis_ptr getProfileNameHypothesis() { return _profileNameHypothesis;}
		
	// Slot getter/creators
	ProfileSlot_ptr getExistingSlot(std::string slot_name);
	ProfileSlot_ptr addSlot(xercesc::DOMElement* slot, PGDatabaseManager_ptr pgdm);
	ProfileSlotMap& getSlots() { return _slots;}
	
	// Helper function for use with evidence ratios
	int countRawFacts(std::string slot_name = "");

	// Prepare profile for upload (generate teaser bio, etc.)
	void prepareForUpload(ProfileConfidence_ptr confidenceModel, bool print_to_screen = false);

	// Strip resolved strings of the resolutions to this actor
	std::wstring stripOfResolutionsToThisActor(std::wstring display_value);

	void printDebugInfoToFile(std::string str) { if (_DEBUG_ON) _debugStream << str; }
	void printDebugInfoToFile(std::wstring str) { if (_DEBUG_ON) _debugStream << str; }
	bool debugIsOn() { return _DEBUG_ON; }

private:
	Profile(int actor_id, std::wstring canonicalActorName, profile_type_t profileType, PGDatabaseManager_ptr pgdm);

	int _actor_id;
	std::wstring _canonical_actor_name;
	profile_type_t _profile_type;
	PGDatabaseManager_ptr _pgdm;

	std::vector<std::wstring> _document_canonical_names; // for use in pruning display strings
	boost::wregex _document_canonical_name_regex;
		
	ProfileSlotMap _slots;
	std::vector<int> _spokespeopleActorIds;
	NameHypothesis_ptr _profileNameHypothesis;

	bool _DEBUG_ON;
	UTF8OutputStream _debugStream;

};

#endif
