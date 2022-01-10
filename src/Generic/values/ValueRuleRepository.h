// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef VALUE_RULE_REPOSITORY_H
#define VALUE_RULE_REPOSITORY_H

#include <boost/regex.hpp>
#include <list>
#include <utility>

class TokenSequence;
class Symbol;

class ValueRuleRepository {
public:
	ValueRuleRepository();

	typedef std::pair<int,int> index_pair_t; // using a typedef makes BOOST_FOREACH compile correctly

	void getPhoneNumbers(TokenSequence * tokSeq, std::list<index_pair_t>& phoneNumbers);
	void getEmails(TokenSequence * tokSeq, std::list<index_pair_t>& emails);
	void getURLs(TokenSequence * tokSeq, std::list<index_pair_t>& urls);

private:

	boost::wregex _sports_words;
		
	// PHONE NUMBER REGEXES
	boost::wregex _phone_bad_hyphen_then_space;
	boost::wregex _phone_bad_range;
	boost::wregex _phone_bad_date_spec1;
	boost::wregex _phone_bad_date_spec2;
	boost::wregex _phone_bad_zip_code;
	boost::wregex _phone_bad_year_in_parens;
	boost::wregex _phone_bad_too_long_straight;
	boost::wregex _phone_bad_singletons;
	boost::wregex _phone_bad_too_many_groups;
	boost::wregex _phone_bad_context1;
	boost::wregex _phone_bad_context2;
	boost::wregex _phone_eight_digit_date;
	boost::wregex _phone_good_american_standard;
	boost::wregex _phone_good_american_seven;
	boost::wregex _phone_good_american_standard_ext;
	boost::wregex _phone_good_american_seven_ext;
	boost::wregex _phone_good_american_intl;
	boost::wregex _phone_good_american_periods;
	boost::wregex _phone_good_country_code;
	boost::wregex _phone_good_context;	
	boost::wregex _phone_common1;	
	boost::wregex _phone_common2;	
	boost::wregex _phone_common3;	
	boost::wregex _phone_common4;	
	boost::wregex _phone_common5;

	// URL REGEXES
	boost::wregex _url_http;
	boost::wregex _url_www;
	boost::wregex _url_domain;

	// EMAIL REGEXES
	boost::wregex _email_at_domain;
	boost::wregex _email_at_country_code;
};


#endif
