/**
 * Implements XDocIdType, which handles conversion between int and string representations of cross-doc IDs.
 *
 * @file XDocIdType.cpp
 * @author afrankel@bbn.com
 * @date 2011.06.13
 **/
#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "boost/lexical_cast.hpp"
#include "boost/algorithm/string/predicate.hpp"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/ASCIIUtil.h"
#include "XDocIdType.h"
#include <sstream>
#include <string>
#include <stdexcept>
const int BBN_XDOC_PREFIX_LEN = BBN_XDOC_PREFIX.length();

/**
 * Initialize from either, e.g., "913" or "bbn:xdoc-913".
 * Will throw if the string is empty or invalid (i.e., not of the form "dd" or "bbn:xdoc-dd", where
 * dd is a string of one or more digits).
 **/
XDocIdType::XDocIdType(const std::wstring & str_w_or_wo_prefix, bool throw_on_exception/*=true*/) :_val(-1) {
	int ret(-1);
	if (boost::starts_with(str_w_or_wo_prefix, BBN_XDOC_PREFIX)) {
		try {    
			ret = boost::lexical_cast<int>(str_w_or_wo_prefix.substr(BBN_XDOC_PREFIX_LEN));
		}
		catch (boost::bad_lexical_cast) {
			std::wostringstream ostr;
			ostr << "Invalid XDoc ID <" << str_w_or_wo_prefix << ">; prefix '" << BBN_XDOC_PREFIX
				<< "' is not followed by an int." << std::endl;
			SessionLogger::err("LEARNIT") << ostr.str();
			if (throw_on_exception) {
				std::string str = UnicodeUtil::toUTF8StdString(ostr.str());
				std::runtime_error e(str.c_str());
				throw e;
			} else {
                ret = -1;
			}
		}
	} else {
		try {    
			ret = boost::lexical_cast<int>(str_w_or_wo_prefix);
		}
		catch (boost::bad_lexical_cast) {
			std::wostringstream ostr;
			ostr << "Invalid XDoc ID <" << str_w_or_wo_prefix << ">; must be an int optionally preceded by " 
				<< BBN_XDOC_PREFIX << std::endl;
			SessionLogger::err("LEARNIT") << ostr.str();
			if (throw_on_exception) {
				std::string str = UnicodeUtil::toUTF8StdString(ostr.str());
				std::runtime_error e(str.c_str());
				throw e;
			} else {
                ret = -1;
			}
		}
	}
	_val = ret;
}

/**
 * Returns true for, e.g., "bbn:xdoc-913".
 * Will return false if the string is empty or invalid (i.e., not of the form "bbn:xdoc-dd", where
 * dd is a string of one or more digits).
 **/
bool XDocIdType::is_valid_string(const std::wstring & str) {
	if (str.empty()) {
		return false;
	} else {
		size_t start_pos(0);
		if (boost::starts_with(str, BBN_XDOC_PREFIX)) {
			start_pos = BBN_XDOC_PREFIX_LEN;
		}
		if (start_pos >= str.length() || ASCIIUtil::containsNonDigits(str.substr(start_pos))) {
			SessionLogger::info("LEARNIT") << L"<" << str << L"> is not a valid XDoc ID." << std::endl;
			return false;
		}
		try {
			// Ignore value assigned to xid; we just want to see whether a string that passed the tests 
			// earlier in the method causes a failure in the constructor (though this is unlikely).
			XDocIdType xid(str); 
		}
		catch(std::exception & e) {
			std::string temp(e.what()); // to circumvent C4101 (unreferenced local variable)
			SessionLogger::info("LEARNIT") << L"<" << str << L"> is not a valid XDoc ID." << std::endl;
			return false;
		}
		return true;
	}
}

/**
 * Returns, e.g., "bbn:xdoc-913" if this->is_valid() is true, an empty string otherwise.
 **/
std::wstring XDocIdType::as_string() const {
	if (is_valid()) {
		std::wostringstream out;
		out << BBN_XDOC_PREFIX << _val;
		return out.str();
	} else {
		return L"";
	}
}

// for testing purposes only
void XDocIdType::test() {
	XDocIdType xid10(500);
	std::wstring str10 = xid10.as_string();
	SessionLogger::info("LEARNIT") << L"str10: " << str10 << std::endl;
	int xid11 = xid10.as_int();
	SessionLogger::info("LEARNIT") << "xid11: " << xid11 << " (int) xid10: " << (int) xid10 << " xid10: " << xid10 << std::endl;
	XDocIdType xid12(L"bbn:xdoc-200");
	XDocIdType xid13(L"200");
	SessionLogger::info("LEARNIT") << "xid12: " << xid12 << " xid13: " << xid13 << std::endl;
	// Tests that should cause exception
	XDocIdType xid20;
	//xid3.from_str_w_prefix(BBN_XDOC_PREFIX); // should throw
	std::wstring str20(BBN_XDOC_PREFIX);
	str20 += L"abc";
	XDocIdType xid24(str20); // should also throw
	SessionLogger::info("LEARNIT") << "xid24: " << xid24 << std::endl;
}

