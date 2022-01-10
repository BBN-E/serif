// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <boost/algorithm/string.hpp>

#include "Generic/values/ValueRuleRepository.h"
#include "Generic/theories/TokenSequence.h"

ValueRuleRepository::ValueRuleRepository() {

	// GENERIC
	_sports_words = boost::wregex(L"scoreboard|scores|baseball|football|hockey|basketball|tennis|bowling|cricket|rugby|soccer|swimming|sports|wickets|team gp|standings|league|wild card|runner-up|seeded", boost::regex::icase);
			
	// PHONE NUMBERS
	_phone_bad_hyphen_then_space = boost::wregex(L".*-.* .*");
	_phone_bad_range = boost::wregex(L"[012][0-9][0-9][0-9]\\s*-?-?\\s*[012][0-9][0-9][0-9]"); // 1872-1913 or 0800-1200
	_phone_bad_date_spec1 = boost::wregex(L"[12][0-9][0-9][0-9]\\s*--?\\s*[01][0-9]\\s*--?\\s*[0123][0-9]"); // 2002-03-14
	_phone_bad_date_spec2 = boost::wregex(L"[01][0-9]\\s*--?\\s*[0123][0-9]\\s*--?\\s*[12][0-9][0-9][0-9]"); // 03-14-2002
	_phone_bad_zip_code = boost::wregex(L"[0-9]{5}\\s*-\\s*[0-9]{4}"); // 33915-2167
	_phone_bad_year_in_parens = boost::wregex(L".*\\([012][0-9]{3}\\).*"); // anything with (2002) in it
	_phone_bad_too_long_straight = boost::wregex(L"\\d{10}\\d+"); // only allow phone numbers with 10 or fewer digits to be straight ########## without dashes
	_phone_bad_singletons = boost::wregex(L".*[ -]+\\d[ -]+\\d[ -]+\\d[ -]+.*"); // three singletons in a row is a bad sign
	_phone_bad_too_many_groups = boost::wregex(L".*\\d+[ -]+\\d+[ -]+\\d+[ -]+\\d+[ -]+\\d+[ -]+\\d+[ -]+.*"); // six groups of numbers is too many
	_phone_eight_digit_date = boost::wregex(L"\\(?(19|20)\\d{6}\\)?");
	_phone_bad_context1 = boost::wregex(L"isbn|serial|license|licence|patent|lottery|ebay| sn |number of|n. of|ref:|id:|total|=|uin:|pin:", boost::regex::icase); // sn is serial number, = is usually a URL
	_phone_bad_context2 = boost::wregex(L"(serial|lucky|ticket|inmate|case|item|smr|id|patent|license|licence)\\s*-?(number|no.|#)", boost::regex::icase);
	_phone_good_american_standard = boost::wregex(L"\\+? ?1? ?-?-? ?\\(?[0-9]{3}\\)?[ \\-][0-9]{3}[ \\-][0-9]{4}");
	_phone_good_american_seven = boost::wregex(L"[0-9]{3}[ \\-][0-9]{4}");
	_phone_good_american_standard_ext = boost::wregex(L"\\+? ?1? ?-?-? ?\\(?[0-9]{3}\\)?[ \\-][0-9]{3}[ \\-][0-9]{4}/\\d{3,4}");
	_phone_good_american_seven_ext = boost::wregex(L"[0-9]{3}[ \\-][0-9]{4}/\\d{3,4}");
	_phone_good_american_intl = boost::wregex(L"001 \\d{3} \\d{7}");
	_phone_good_american_periods = boost::wregex(L"1?\\.?[0-9]{3}\\.[0-9]{3}\\.[0-9]{4}");
	_phone_good_country_code = boost::wregex(L"\\+\\d.*");
	_phone_good_context = boost::wregex(L".*(fax|tel\\.?|telephone|telefon|info:|phone|voice|mobile|cell|pager|contact|call|dial|gsm|sms|home|work| w\\.? | h\\.? )\\.?\\s*(number)?\\s*:?\\s*", boost::regex::icase);
	_phone_common1 = boost::wregex(L"\\d{2,5}[ \\-]\\d{3,4}[ \\-]\\d{3,4}");
	_phone_common2 = boost::wregex(L"\\d{2} \\d{2} \\d{2} \\d{2} \\d{2}"); //france
	_phone_common3 = boost::wregex(L"\\d{4} \\d{1} \\d{3} \\d{3}"); //germany
	_phone_common4 = boost::wregex(L"\\d{4} \\d{1} \\d{6}"); // germany
	_phone_common5 = boost::wregex(L"\\d{7}\\d? ?(\\-\\d\\d+)?"); // iran

	//URLS
	_url_http = boost::wregex(L"https?\\:", boost::regex::icase);
	_url_www = boost::wregex(L"www\\.", boost::regex::icase);
	_url_domain = boost::wregex(L"\\.(com|gov|edu|org|mil|net)(\\W|$)", boost::regex::icase);

	//EMAILS
	_email_at_domain = boost::wregex(L"@.*\\.(com|gov|edu|org|mil|net)", boost::regex::icase);
	_email_at_country_code = boost::wregex(L"@.*\\.[A-Za-z][A-Za-z](\\W|$)", boost::regex::icase);
}

void ValueRuleRepository::getEmails(TokenSequence * tokSeq, std::list<index_pair_t>& emails) {
	boost::wcmatch matchResult;
	for (int i = 0; i < tokSeq->getNTokens(); i++) {
		std::wstring targetString = tokSeq->toString(i, i); // NB: this function takes inclusive indices
		boost::trim(targetString);
		if (boost::regex_search(targetString.c_str(), matchResult, _email_at_domain) ||
		    boost::regex_search(targetString.c_str(), matchResult, _email_at_country_code)) 
		{
			index_pair_t my_pair(i, i);
			emails.push_back(my_pair);
		} 
	}
}

void ValueRuleRepository::getURLs(TokenSequence * tokSeq, std::list<index_pair_t>& urls) { 
	boost::wcmatch matchResult;
	for (int i = 0; i < tokSeq->getNTokens(); i++) {
		std::wstring targetString = tokSeq->toString(i, i); // NB: this function takes inclusive indices
		boost::trim(targetString);
		if (boost::regex_search(targetString.c_str(), matchResult, _url_http) ||
			boost::regex_search(targetString.c_str(), matchResult, _url_www) ||
			boost::regex_search(targetString.c_str(), matchResult, _url_domain))
		{
			index_pair_t my_pair(i, i);
			urls.push_back(my_pair);
		}
	}
}

void ValueRuleRepository::getPhoneNumbers(TokenSequence * tokSeq, std::list<index_pair_t>& phoneNumbers) {

	// Seems like most moderately sized strings of numbers are probably phone numbers.
	// However, we need to avoid certain things, e.g. 0800-0500 is likely to be hours rather
	// than a phone number. We really actually need a full-on module for this, and I am sure
	// others have worked on this and done much better. But for now this will have to do.

	// It would also be much better if we could do this at the document level, but we can't.
	// We miss things in preceding or following context. :(
	// For example: 
	//
	// or you can call us:
	// 086-013152697369
	//
	// OR 
	//
	// 108 jackson
	// j c, OH 45334
	// US
	// 5965263
	
	// This also doesn't currently deal well with zip code / phone number combos
	// e.g. Northridge, California 91326 (818)363-9304
	// To do this right, we'd need to first identify zip codes, then use that to skip over them here.

	int start = 0;
	int end = 0;
	boost::wcmatch matchResult;	
	for ( ; start < tokSeq->getNTokens() ; start = end + 1) {

		// create string of numeric/dash/paren/period/whitespace tokens (allow + to start, for country codes)
		int n_right_parens = 0;
		for (end = start ; end < tokSeq->getNTokens(); end++) {
			Symbol tokSym = tokSeq->getToken(end)->getSymbol();
			if (tokSym == Symbol(L"-LRB-")) {
				continue;
			} else if (tokSym == Symbol(L"-RRB-")) {
				n_right_parens++;
				// we don't allow phone numbers to start with a ')'
				if (end == start) 
					break;
				else continue;
			} else {
				std::wstring tokenStr = tokSym.to_string();
				bool invalid_token = false;
				for (size_t i = 0; i < tokenStr.length(); i++) {
					// first character: only digits and '+' are allowed
					// other characters: only digits, '-', '/', and '.' are allowed
					wchar_t ch = tokenStr.at(i);
					if (end == start && i == 0) {
						if (ch != L'+' && !iswdigit(ch)) {
							invalid_token = true;
							break;
						}
					} else if (!iswdigit(ch) && ch != L'-' && ch != L'.' && ch != L'/') {
						invalid_token = true;
						break;
					}
				}
				if (invalid_token)
					break;
			}
		}

		// if start == end, we didn't even make it through a single token (end is an exclusive index at this point)
		if (start == end)
			continue;

		// make end an inclusive index
		end--;

		// get rid of closing parentheses (likely to be something like "Call me (617-873-3200).")
		if (tokSeq->getToken(end)->getSymbol() == Symbol(L"-RRB-")) {
			if (start == end)
				continue;
			end--;
			n_right_parens--;
		}

		if (tokSeq->getToken(end)->getSymbol() == Symbol(L"-LRB-")) {
			if (start == end)
				continue;
			end--;
		}

		if (tokSeq->getToken(end)->getSymbol() == Symbol(L".")) {
			if (start == end)
				continue;
			end--;
		}

		// get rid of opening parentheses if string has no closing parenthesis (likely to be "Call me (617-873-3200 is my number)")
		if (tokSeq->getToken(start)->getSymbol() == Symbol(L"-LRB-") && n_right_parens == 0) {
			if (start == end)
				continue;
			start++;
		}

		// this is the potential phone number string, which we modify to make regex matching simpler
		std::wstring targetString = tokSeq->toString(start, end); // NB: this function takes inclusive indices
		boost::trim(targetString);
		boost::replace_all(targetString, "-LRB- ", "(");
		boost::replace_all(targetString, " -RRB-", ")");
		boost::replace_all(targetString, " - ", "-");
		boost::replace_all(targetString, " -", "-");
		boost::replace_all(targetString, "- ", "-");

		// count actual digits in string
		int n_digits = 0;
		for (size_t i = 0; i < targetString.length(); i++) {
			if (iswdigit(targetString.at(i)))
				n_digits++;
		}

		// This will miss some Iraqi phone numbers, but I think 6-digit codes are too risky
		if (n_digits < 7)
			continue;

		// Too long to be a likely phone number
		// changed from 20 to 25 to accomodate iranian numbers
		if (targetString.length() > 25)
			continue;

		// context is the seven preceding tokens (picked for no particular reason, feel free to change it)
		int context_start = std::max(0, start - 7);
		std::wstring precedingContext = tokSeq->toString(context_start, start - 1); // NB: this function takes inclusive indices
		precedingContext += L" "; // to make word boundary matching easier
		
		// REMOVE THINGS ALMOST DEFINITELY NOT PHONE NUMBERS
		if (boost::regex_match(targetString.c_str(), matchResult, _phone_bad_hyphen_then_space) ||
			boost::regex_match(targetString.c_str(), matchResult, _phone_bad_range) ||
			boost::regex_match(targetString.c_str(), matchResult, _phone_bad_date_spec1) ||
			boost::regex_match(targetString.c_str(), matchResult, _phone_bad_date_spec2) ||
			boost::regex_match(targetString.c_str(), matchResult, _phone_bad_zip_code) ||
			boost::regex_match(targetString.c_str(), matchResult, _phone_bad_year_in_parens) ||
			boost::regex_match(targetString.c_str(), matchResult, _phone_bad_too_long_straight) ||
			boost::regex_match(targetString.c_str(), matchResult, _phone_bad_singletons)||
			boost::regex_match(targetString.c_str(), matchResult, _phone_bad_too_many_groups) ||
			boost::regex_match(targetString.c_str(), matchResult, _phone_eight_digit_date) ||
			boost::regex_search(precedingContext.c_str(), matchResult, _phone_bad_context1) ||
			boost::regex_search(precedingContext.c_str(), matchResult, _phone_bad_context2))
			
		{
			//std::wcout << "Discarding phone number: " << printableTestStr << "--" << printableContextStr << "\n";
			continue;
		}

		// TAG THINGS THAT ARE DEFINITELY PHONE NUMBERS (mostly American)
		if (boost::regex_match(targetString.c_str(), matchResult, _phone_good_american_standard) ||
			boost::regex_match(targetString.c_str(), matchResult, _phone_good_american_seven) ||
			boost::regex_match(targetString.c_str(), matchResult, _phone_good_american_standard_ext) ||
			boost::regex_match(targetString.c_str(), matchResult, _phone_good_american_seven_ext) ||
			boost::regex_match(targetString.c_str(), matchResult, _phone_good_american_intl) ||
			boost::regex_match(targetString.c_str(), matchResult, _phone_good_american_periods) ||
			boost::regex_match(targetString.c_str(), matchResult, _phone_good_country_code) ||
			boost::regex_search(precedingContext.c_str(), matchResult, _phone_good_context))
		{
			//std::wcout << "Printing conservative phone number: " << targetString << "--" << precedingContext << "\n";
			index_pair_t my_pair(start, end);
			phoneNumbers.push_back(my_pair);
			continue;
		}

		// SKIP ANYTHING THAT HAS A PERIOD
		if (targetString.find(L'.') != std::wstring::npos)
			continue;

		// SKIP SPORTS SENTENCES (scores are evil phone-number-like things)
		std::wstring fullSentence = tokSeq->toString();
		if (boost::regex_search(fullSentence.c_str(), matchResult, _sports_words))
			continue;

		// TAG THINGS THAT ARE MORE LIKELY THAN NOT TO BE PHONE NUMBERS (non-American)
		boost::replace_all(targetString, L"(","");
		boost::replace_all(targetString, L")","");
		if (boost::regex_match(targetString.c_str(), matchResult, _phone_common1) ||
			boost::regex_match(targetString.c_str(), matchResult, _phone_common2) ||
			boost::regex_match(targetString.c_str(), matchResult, _phone_common3) ||
			boost::regex_search(targetString.c_str(), matchResult, _phone_common4) ||
			boost::regex_search(targetString.c_str(), matchResult, _phone_common5))
		{
			//std::wcout << "Printing moderate phone number: " << targetString << "--" << precedingContext << "\n";
			index_pair_t my_pair(start, end);
			phoneNumbers.push_back(my_pair);
			continue;
		}

		//std::wcout << "Discarding possible phone number: " << targetString << "--" << precedingContext << "\n";
		continue;
	}
}
