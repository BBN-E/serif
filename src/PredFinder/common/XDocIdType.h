/**
 * Defines XDocIdType, which handles conversion between int and string representations of cross-doc IDs.
 *
 * @file XDocIdType.h
 * @author afrankel@bbn.com
 * @date 2011.05.31
 **/

#pragma once

#include <string>

const std::wstring BBN_XDOC_PREFIX = L"bbn:xdoc-";

class XDocIdType {
public:
	typedef int AsIntType;
	typedef std::wstring AsStrType;
	// INVALID_XDOC_ID is used to indicate an uninitialized, invalid, or missing XDoc ID.
	static const int INVALID_XDOC_ID = -1;
	static bool is_valid(int val) {return (val != INVALID_XDOC_ID);}
	// Returns true for, e.g., "bbn:xdoc-913".
	// Will return false if the string is empty or invalid (i.e., not of the form "dd" or "bbn:xdoc-dd", where
	// dd is a string of one or more digits).
	static bool is_valid_string(const std::wstring & str);
	static void test(); // for test purposes only
	XDocIdType(): _val(INVALID_XDOC_ID) {}
	XDocIdType(int val): _val(val) {}
	~XDocIdType() {}
	XDocIdType(const XDocIdType & other) :_val(other._val) {}
	XDocIdType & operator=(const XDocIdType &other) {
		if (this != &other) {
			_val = other._val;
		}
		return *this;
	}

	/**
	 * Constructs an XDocIdType given a string with or without a "bbn:xdoc-" prefix.
	 * @param str_w_or_wo_prefix A string of the form "dd" or "bbn:xdoc-dd", where dd is a string of one or more digits.
	 * @param throw_on_exception Indicates whether to throw an exception, or simply construct an instance with val = INVALID_XDOC_ID,
	 * if the string is of the wrong form.
	 * @example XDocIdType xid(L"bbn:xdoc-913") constructs a valid instance with _val = 913.
	 * @example XDocIdType xid(L"822") constructs an instance with _val = 822.
	 * @example XDocIdType xid(L"", false) constructs an instance with val = INVALID_XDOC_ID.
	 * @example XDocIdType xid(L"", true) throws an exception.
	 * @example XDocIdType xid(L"bbn:xdoc-NONE", true) throws an exception.
	 **/
	XDocIdType(const std::wstring & str_w_or_wo_prefix, bool throw_on_exception=true);
	int as_int() const {return _val;}
	operator int() const {return _val;}
	// Returns a string in "bbn:xdoc-dd" form (e.g., "bbn:xdoc-913") if this->is_valid() is true, an empty string otherwise.
	std::wstring as_string() const;
	bool is_valid() const {return (_val != INVALID_XDOC_ID);}
protected:
	int _val;
};
