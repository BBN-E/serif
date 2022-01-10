// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef TEASER_BIOGRAPHY_H
#define TEASER_BIOGRAPHY_H

#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include <boost/regex.hpp> 
#include "ProfileGenerator/NameHypothesis.h"
#include "ProfileGenerator/EmploymentHypothesis.h"

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(Profile);
BSP_DECLARE(TeaserBiography);
BSP_DECLARE(GenericHypothesis);
BSP_DECLARE(ProfileSlot);

class TeaserBiography {
public:
	friend TeaserBiography_ptr boost::make_shared<TeaserBiography>(Profile_ptr const&);
	std::wstring getBiography() { return _biography; }
	static std::wstring monthFromIntWString(std::wstring monthInt);
	
private:
	TeaserBiography(Profile_ptr profile);
	Profile_ptr _profile;
	NameHypothesis_ptr _name_hypothesis;
	std::wstring _biography;
	int _bio_mentions;

	std::wstring getLeadPersonSentence(Profile_ptr profile);
	std::wstring getFamilySentence(Profile_ptr profile);
	std::wstring getEducationSentence(Profile_ptr profile);
	
	std::wstring getLeadOrganizationSentence(Profile_ptr profile);
	std::wstring getSimpleFoundingHQSentence(std::wstring founder, std::wstring founding_date, std::wstring hq);
	
	std::wstring getDescriptionSentence(Profile_ptr profile);
	std::wstring getName(Profile_ptr profile);

	std::wstring getPrintableList(ProfileSlot_ptr slot, int max_to_print, bool include_titles);
	std::wstring getPrintableList(std::list<GenericHypothesis_ptr>& hypotheses, int max_to_print, bool include_titles, bool is_description);

	std::wstring getDisplayValue(GenericHypothesis_ptr hypo, bool include_titles, bool use_quotes);

	std::wstring getDateDisplayValue(ProfileSlot_ptr slot);
	bool isSingularJobTitle(std::wstring title);
	std::wstring getJobTitleArticle(std::wstring title);
	std::wstring modifyEmployerName(std::wstring employer);
	EmploymentHypothesis::status_tense_t getEmploymentTense(GenericHypothesis_ptr hypo);
	void initReferences() { _bio_mentions = 0;}
	std::wstring repeatReference(NameHypothesis::PronounRole, bool capitalize);  



};



#endif
