// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/LocatedString.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/InputStream.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/StringTransliterator.h"

#include <iostream>
#include <cstdarg>
#include <wchar.h>
#include <math.h>
#include <stdio.h>
#include <cassert>
#include <boost/foreach.hpp>

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLStrings.h"
#include <boost/scoped_ptr.hpp>

#include <xercesc/util/XMLChar.hpp>

#define MARK_INVALID_EDT_OFFSETS false
#define MARK_INVALID_ASR_TIMES true

using namespace std;

LocatedString::LocatedString(const wchar_t *source): _text(source) {
	initialize_offsets();
	initialize_bounds();
	assert(checkOffsetInvariants());
}

LocatedString::LocatedString(const wchar_t *source, OffsetGroup start)
: _text(source)
{
	initialize_offsets(start);
	initialize_bounds();
	assert(checkOffsetInvariants());
}

LocatedString::LocatedString(InputStream &stream) {
	initFromStream(stream);
	assert(checkOffsetInvariants());
}

void LocatedString::readTextFromStream(InputStream &stream) {
	const size_t BUFFER_SIZE = 1024;
	wchar_t buffer[BUFFER_SIZE];
	while (stream.good()) {
		stream.read(buffer, BUFFER_SIZE);
		_text.append(buffer, buffer+stream.gcount());
	}
	if (stream.bad()) {
		throw UnexpectedInputException("LocatedString::LocatedString",
			"Error while reading from stream -- does it contain invalid UTF-8?"); 
	}
}

void LocatedString::initFromStream(InputStream &stream) {
	readTextFromStream(stream);
	initialize_offsets();
	initialize_bounds();
	assert(checkOffsetInvariants());
}

LocatedString::LocatedString(InputStream &stream, wchar_t delim) {
	while( wchar_t c=stream.get() ) {
		if (c == delim) break;
		_text.push_back(c);
	}
	if (stream.bad()) {
		throw UnexpectedInputException("LocatedString::LocatedString",
			"Error while reading from stream -- does it contain invalid UTF-8?"); 
	}
	initialize_offsets();
	initialize_bounds();
	assert(checkOffsetInvariants());
}

LocatedString::LocatedString(const LocatedString &source, std::list<OffsetEntry> offsets) : _text(source._text) {
	_offsets.reserve(offsets.size());
	BOOST_FOREACH(OffsetEntry offset, offsets) {
		_offsets.push_back(offset);
	}
	initialize_bounds();
	assert(checkOffsetInvariants());
}

/** Create a substring.  (Private constructor)
 * 
 * @param s the source string.
 * @param begin_index the index of the first character to copy, inclusive.
 * @param end_index the index of the last character to copy, *exclusive*. 
 *                  Note that this contrasts with most other places in the
 *                  code, where end offsets and end positions are inclusive.
 *
 */
LocatedString::LocatedString(const LocatedString &s, int begin_index, int end_index)
	: _text(s._text.begin()+begin_index, s._text.begin()+end_index)
{
/*	if (end_index == begin_index) {
		throw InternalInconsistencyException("LocatedString::LocatedString", 
			"Error constructing substring: zero-length LocatedStrings are not allowed.");
	}*/
	if (end_index < begin_index) {
		throw InternalInconsistencyException("LocatedString::LocatedString", 
			"Error constructing substring: end_offset is less than begin_offset");
	}
	// Copy the offsets from s to _offsets.
	s.copyOffsetsOfSubstring(begin_index, end_index, _offsets);

	// Initialize the bounds from the substring.  We do *not* copy them from the
	// source string.
	initialize_bounds();
	assert(checkOffsetInvariants());
}

// dst is an output parameter.
void LocatedString::copyOffsetsOfSubstring(int begin_index, int end_index, std::vector<LocatedString::OffsetEntry> &dst) const {
	if (begin_index == end_index) return;
	for (size_t entry_num = findOffsetEntryBefore(begin_index); entry_num < _offsets.size(); ++entry_num) {
		dst.push_back(_offsets[entry_num]);
		OffsetEntry& entry = dst.back();
		assert(!(entry.startPos > end_index));
		if (entry.startPos < begin_index) {
			entry.startPos = begin_index;
			entry.startOffset = startOffsetGroup(begin_index);
		}
		if (entry.endPos > end_index) {
			entry.endPos = end_index;
			entry.endOffset = endOffsetGroup(end_index-1);
		}
		// The new string's positions start at begin_index.
		entry.startPos -= begin_index;
		entry.endPos -= begin_index;
		// Check if we're done.
		if (entry.endPos >= (end_index-begin_index))
			break;
	}
}

/** Initialize the offsets for this LocatedString, based on its _text string. */
void LocatedString::initialize_offsets(OffsetGroup start) {
	int byte_offset = start.byteOffset.value();
	int char_offset = start.charOffset.value();
	int edt_offset = start.edtOffset.value();

	int pos = 0;
	for ( ; pos < static_cast<int>(_text.length()); ++pos) {
		// Update the offsets to consume the new character.
		++char_offset;
		byte_offset += static_cast<int>(UnicodeUtil::countUTF8Bytes(_text[pos]));
		++edt_offset;
	}
	if (pos > 0) {
		OffsetGroup end(ByteOffset(byte_offset-1), CharOffset(char_offset-1), EDTOffset(edt_offset-1), ASRTime());
		_offsets.push_back(OffsetEntry(0, pos, start, end, false));
	}
}

void LocatedString::initialize_bounds(bool empty_is_undefined) {
	int len = length();
	if (len > 0) {
		_bounds = OffsetRange(_offsets.front().startOffset, _offsets.back().endOffset);
	} else if (empty_is_undefined) {
		_bounds = OffsetRange(); // all offsets undefined.
	} else {
		OffsetGroup zeroStart(ByteOffset(0), CharOffset(0), EDTOffset(0), ASRTime(0.0));
		OffsetGroup zeroEnd(ByteOffset(0), CharOffset(0), EDTOffset(0), ASRTime(0.0));
		_bounds = OffsetRange(zeroStart, zeroEnd); // all offsets zero.
	}
}

/**
 * The characters are appended to the end of this string's source text,
 * and all characters are marked with the offset of the last character
 * in this string before appending.
 *
 * For example:
 *
 * <pre>    ["book",{100,101,102,103}].append("shelf")
 *    =>
 *    ["bookshelf",{100,101,102,103,103,103,103,103,103}]</pre>
 *
 * @param str the string to append to this LocatedString.
 */
void LocatedString::append(const wchar_t *str) {
	insert(str, length());
}

void LocatedString::append(const LocatedString &str) {
	_bounds.end = str._bounds.end;
	_offsets.insert(_offsets.end(), str._offsets.begin(), str._offsets.end());
	for (size_t i=_offsets.size()-str._offsets.size(); i<_offsets.size(); ++i) {
		_offsets[i].startPos += static_cast<int>(_text.length());
		_offsets[i].endPos += static_cast<int>(_text.length());
	}

	_text.insert(_text.end(), str._text.begin(), str._text.end());

	assert(checkOffsetInvariants());
}

bool LocatedString::checkOffsetInvariants() const {
	if (_offsets.empty()) {
		assert(_text.empty());
	} else {
		assert (_offsets.front().startPos == 0);
		assert (_offsets.back().endPos == static_cast<int>(_text.length()));
		for (size_t i=0; i<_offsets.size(); ++i)
			assert (_offsets[i].endPos > _offsets[i].startPos);
		for (size_t i=1; i<_offsets.size(); ++i)
			assert (_offsets[i-1].endPos == _offsets[i].startPos);
		/*for (size_t i=0; i<_offsets.size(); ++i)
			assert (_offsets[i].endOffset.edtOffset >= _offsets[i].startOffset.edtOffset);*/
	}
	return true;
}

/**
 * Each character's offset is set to the offset of the character immediately
 * to the left of <code>pos</code>, or immediately to the right of the
 * inserted text if it is inserted at the beginning of the string (i.e., if
 * <code>pos == 0</code>).
 *
 * For example:
 *
 * <pre>    ["smiles", {100,101,102,103,104,105}].insert("ix ", 1)
 *    =>
 *    ["six miles", {100,100,100,100,101,102,103,104,105}]</pre>
 *
 * <pre>    ["hazard", {100,101,102,103,104,105}].insert("hap", 0)
 *    =>
 *    ["haphazard", {100,100,100,100,101,102,103,104,105}]</pre>
 *
 * @param str the array of characters to insert into this string.
 * @param pos the position at which to insert the text.
 */
void LocatedString::insert(const wchar_t *str, int pos) {
	OffsetGroup new_offset;
	if (length() == 0) {
		SessionLogger::warn("located_string") << "inserting into an empty string.  Offsets will be undefined." << std::endl;
	} else if (pos == 0) {
		new_offset = _offsets.front().startOffset;
	} else {
		new_offset = endOffsetGroup(pos-1);
	}
	insert(str, pos, OffsetRange(new_offset, new_offset));
}

void LocatedString::insertAtPOSWithSameOffsets(const wchar_t *str, int pos){
	if(pos >= static_cast<int>(_text.length()))
		pos = static_cast<int>(_text.length()) - 1;
	OffsetGroup new_offset = endOffsetGroup(pos);
	insert(str, pos, OffsetRange(new_offset, new_offset));
}

void LocatedString::insert(const wchar_t *str, int pos, 
						   const OffsetRange &new_offset) {
	if (pos > length()) {
		throw InternalInconsistencyException::arrayIndexException(
			"LocatedString::insert()", length(), pos);
	}
	size_t old_length = _text.size();
	_text.insert(pos, str);
	size_t count = _text.size() - old_length;

	// Decide where to insert the new offset entries; and split an offset entry
	// if necessary.
	size_t insertion_point;
	size_t entry_num = findOffsetEntryBefore(pos);
	bool is_edt_skip_region = !_offsets.empty() && _offsets[entry_num].is_edt_skip_region;
	if (_offsets.empty()) {
		insertion_point = 0;
	} else if (_offsets[entry_num].startPos == pos) {
		insertion_point = entry_num;
	} else if (_offsets[entry_num].endPos == pos) {
		insertion_point = entry_num+1;
	} else {
		splitOffsetsAtPos(pos, entry_num);
		insertion_point = entry_num+1;
	}

	// We add one OffsetEntry for each character added.
	OffsetEntry new_entry(-1, -1, new_offset.start, new_offset.end, is_edt_skip_region);
	_offsets.insert(_offsets.begin()+insertion_point, count, new_entry);
	for (size_t i=0; i<count; ++i) {
		_offsets[insertion_point+i].startPos = pos+static_cast<int>(i);
		_offsets[insertion_point+i].endPos = pos+static_cast<int>(i)+1; // exclusive
	}

	// Any following entries must have their positions updated.
	for (entry_num=insertion_point+count;entry_num < _offsets.size(); ++entry_num) {
		_offsets[entry_num].startPos += static_cast<int>(count);
		_offsets[entry_num].endPos += static_cast<int>(count);
	}
	assert(checkOffsetInvariants());
}

void LocatedString::splitOffsetsAtPos(int pos, size_t entry_num) {
	assert(pos>0);
	OffsetGroup splitStart = startOffsetGroup(pos);
	OffsetGroup splitEnd = endOffsetGroup(pos-1);
	_offsets.insert(_offsets.begin()+entry_num+1, OffsetEntry(
		pos, _offsets[entry_num].endPos, splitStart, 
		_offsets[entry_num].endOffset, _offsets[entry_num].is_edt_skip_region));
	_offsets[entry_num].endPos = pos;
	_offsets[entry_num].endOffset = splitEnd;
}

/**
 * The offset buffer is adjusted accordingly.
 *
 * For example:
 *
 * <pre>    ["six miles", {100,101,102,103,104,105,106,107,108}].remove(1, 4)
 *    =>
 *    ["smiles", {100,104,105,106,107,108}]</pre>
 *
 * @param start the beginning index, inclusive.
 * @param end the ending index, exclusive.
 */
void LocatedString::remove(int start, int end) {
	size_t num_chars_removed = (end-start);
	if (start == end)
		return;
	if (start > end) {
		throw InternalInconsistencyException("LocatedString::remove()",
			"start > end");
	}
	if (end > length()) {
		throw InternalInconsistencyException::arrayIndexException(
			"LocatedString::remove()", length(), end);
	}

	OffsetGroup endOffsetAtStartOfDeletedRange;
	OffsetGroup startOffsetAtEndOfDeletedRange;
	if (start > 0)
		endOffsetAtStartOfDeletedRange = endOffsetGroup(start-1);
	if (end < length())
		startOffsetAtEndOfDeletedRange = startOffsetGroup(end);
	int first_to_delete = -1;
	int last_to_delete = -1;
	size_t entry_num = findOffsetEntryBefore(start);
	while ((entry_num < _offsets.size()) && (_offsets[entry_num].startPos < end)) {
		if (_offsets[entry_num].startPos >= start && _offsets[entry_num].endPos <= end) {
			// We can delete this entire entry.
			if (first_to_delete == -1) 
				first_to_delete = static_cast<int>(entry_num);
			last_to_delete = static_cast<int>(entry_num);
			++entry_num;
		} else if (_offsets[entry_num].startPos < start && _offsets[entry_num].endPos > end) {
			// We must split this entry.
			_offsets.insert(_offsets.begin()+entry_num+1, OffsetEntry(
				start, _offsets[entry_num].endPos-static_cast<int>(num_chars_removed),	
				startOffsetAtEndOfDeletedRange, _offsets[entry_num].endOffset, 
				_offsets[entry_num].is_edt_skip_region));
			_offsets[entry_num].endPos = start;
			_offsets[entry_num].endOffset = endOffsetAtStartOfDeletedRange;
			assert(start > 0);
			assert(end < length());
			entry_num += 2; 
			break;
		} else if (end >= _offsets[entry_num].endPos) {
			// We can just adjust the end of this entry.
			_offsets[entry_num].endPos = start;
			_offsets[entry_num].endOffset = endOffsetAtStartOfDeletedRange;
			assert(start > 0);
			++entry_num;
		} else if (start <= _offsets[entry_num].startPos) {
			// We can just adjust the start of this entry.
			_offsets[entry_num].startPos = start;
			_offsets[entry_num].endPos -= static_cast<int>(num_chars_removed);
			assert(end < length());
			_offsets[entry_num].startOffset = startOffsetAtEndOfDeletedRange;
			++entry_num; 
			break;
		} else {
			assert(0); // All cases should already be covered!
		}
	}

	// Any following entries must have their positions updated.
	for (;entry_num < _offsets.size(); ++entry_num) {
		_offsets[entry_num].startPos -= static_cast<int>(num_chars_removed);
		_offsets[entry_num].endPos -= static_cast<int>(num_chars_removed);
	}

	// If there are any entries that should be deleted, then delete them now.
	if (first_to_delete >= 0)
		_offsets.erase(_offsets.begin()+first_to_delete, _offsets.begin()+last_to_delete+1);

	_text.erase(start, end-start);
	assert(checkOffsetInvariants());
}

void LocatedString::remove(const wchar_t *find) {
	replace(find, L"");
}

/**
 * All the offsets for the new text are set to the offset of the
 * character immediately to the left, or immediately to the right
 * if the text is replaced at the beginning of the string (i.e.,
 * if <code>pos == 0</code>).
 *
 * For example:
 *
 * <pre>    ["six miles", {100,101,102,103,104,105,106,107,108}].replace(1, 2, "even")
 *    =>
 *    ["seven miles", {100,100,100,100,100,103,104,105,106,107,108}]</pre>
 *
 * <pre>    ["six miles", {100,101,102,103,104,105,106,107,108}].replace(0, 3, "ten")
 *    =>
 *    ["ten miles", {103,103,103,103,104,105,106,107,108}]</pre>
 *
 * @param pos the position at which to replace text.
 * @param len the number of characters to remove.
 * @param repl the text to replace characters with.
 */
void LocatedString::replace(int pos, int len, const wchar_t *repl) {
	if (len <= 0)
		throw InternalInconsistencyException("LocatedString::replace()",
									   "negative length parameter");
	if ((pos + len) > length())
		throw InternalInconsistencyException::arrayIndexException(
			"LocatedString::replace()", length(), (pos + len));
	
	OffsetRange new_offset;
	if (length() > 0)
		new_offset = OffsetRange(startOffsetGroup(pos), endOffsetGroup(pos+len-1));

	remove(pos, pos+len);
	insert(repl, pos, new_offset);
	/*
	if (len <= 0)
		throw InternalInconsistencyException("LocatedString::replace()",
									   "negative length parameter");
	if ((pos + len) > length())
		throw InternalInconsistencyException::arrayIndexException(
			"LocatedString::replace()", length(), (pos + len));
	
	OffsetRange new_offset;
	if (length() > 0)
		new_offset = OffsetRange(_offsets[pos].start, _offsets[pos+len-1].end);

	// Replace the original text
	_text.replace(pos, len, repl);

	// Grow or shrink the offsets, if necessary.
	int length_change = static_cast<int>(_text.size()) - static_cast<int>(_offsets.size());
	if (length_change > 0) {
		_offsets.insert(_offsets.begin()+pos+len, length_change, new_offset);
	} else if (length_change < 0) {
		_offsets.erase(_offsets.begin()+pos, _offsets.begin()+pos-length_change);
	}

	// Set the offsets for the newly replaced text.
	for (int i=pos; i<pos+(len+length_change); ++i)
		_offsets[i] = new_offset;
	*/
}

/**
 * All the offsets for any new text are set to the offset of the
 * character immediately to the left of the replaced text, or
 * immediately to the right if the text is replaced at the
 * beginning of the string.
 *
 * For example:
 *
 * <pre>    ["_one_two_", {100,101,102,103,104,105,106,107,108}].replace("_", " ")
 *    =>
 *    [" one two ", {101,101,102,103,103,105,106,107,107}]</pre>
 *
 * @param find the text to replace.
 * @param repl the text to replace it with.
 */
void LocatedString::replace(const wchar_t *find, const wchar_t *repl) {
	int find_len = static_cast<int>(wcslen(find));
	int repl_len = static_cast<int>(wcslen(repl));
	int start = 0;
	while (start < length()) {
		int pos = indexOf(find, start);
		if (pos == -1)
			return;
		replace(pos, find_len, repl);
		start = pos + repl_len;
	}
}

void LocatedString::replace(const std::wstring & find, const std::wstring & repl) {
	replace(find.c_str(), repl.c_str());
}

/** Find any occurance of the string 'find' and replace with 'repl'.
	*	See above description of how the offsets of the replacement characters are set.
	*	This replace continues to look where it last found an occurance of 'find'
	*	So if 'find'="&amp;" and 'repl'="&", then "&amap;amp;amp" will be replaced with "&"
	*/
void LocatedString::replaceRecursive(const std::wstring & find, const std::wstring & repl) {
	int find_len = static_cast<int>(find.length());
	int start = 0;
	while(start < length()) {
		int pos = indexOf(find.c_str(), start);
		if(pos == -1)
			return;
		replace(pos, find_len, repl.c_str());
		start = pos;
	}
}

/** Replace all substrings found between 'findStart' and 'findEnd' with repl, including 'findStart' and 'findEnd'.
	*/
void LocatedString::replaceBetween(const std::wstring &findStart, const std::wstring &findEnd, const std::wstring & repl) {
	int findStartLen = static_cast<int>(findStart.length());
	int findEndLen = static_cast<int>(findEnd.length());

	int searchStart = 0;
	while(searchStart < length()) {
		int segStart = indexOf(findStart.c_str(), searchStart);
		int segEnd = indexOf(findEnd.c_str(), segStart + findStartLen);

		if(segStart == -1 || segEnd == -1)
			return;

		replace(segStart, segEnd + findEndLen - segStart, repl.c_str());
	}

}

/**
 * For any characters removed, the behavior is the same as in remove().
 */
void LocatedString::trim() {
	if (length() > 0) {
		// Remove whitespace from the end.
		int end = length()-1;
		while (end >= 0 && iswspace(_text[end])) --end;
		if (length() != end+1)
			remove(end+1, length());
		// Remove whitespace from the beginning.
		int start = 0;
		while (start < length() && iswspace(_text[start])) ++start;
		if (start != 0)
			remove(0, start);
	}
}

void LocatedString::toLowerCase() {
	for (int i = 0; i < length(); i++) {
		_text[i] = towlower(_text[i]);
	}
}

void LocatedString::toUpperCase() {
	for (int i = 0; i < length(); i++) {
		_text[i] = towupper(_text[i]);
	}
}

// Basic character & offset accessor functions.

/**
 * @return a null-terminated, wide-character string containing this
 *         string's character buffer.
 */
const wchar_t* LocatedString::toString() const {
	return _text.c_str();
}

/**
 * @return a Symbol of this located string's text.
 */
Symbol LocatedString::toSymbol() const {
	return Symbol(_text);
}

/**
 * Returns <code>-1</code> if the string is not found.
 *
 * @param str the string to search for.
 * @param start the first index to start searching at.
 * @return the first index of the given string in this LocatedString,
 *         or <code>-1</code> if the string is not found.
 */
int LocatedString::indexOf(const wchar_t *str, int start) const {
	// If we're searching beyond the end of the string, then an
	// empty string is found exactly at the end, and any other
	// string is not found at all.
	if (start >= length()) {
		return (str[0] == L'\0') ? length() : -1;
	}

	// We can't start before the string.
	if (start < 0) {
		start = 0;
	}

	// An empty string is always found immediately.
	if (str[0] == L'\0') {
		return start;
	}

	size_t pos = _text.find(str, start);
	return (pos == std::wstring::npos) ? -1 : static_cast<int>(pos);
}

/**
 * From the specified position, skip characters forwards until
 * we find a line break. If index is a line break, index is returned.
 *
 * @param A position in this string.
 * @return The end of this line, or the end of string if none found.
 **/
int LocatedString::endOfLine(int index) const {
	int end = index - 1;
	do {
		end++;
	} while (end < length() && charAt(end) != L'\n' && charAt(end) != L'\r');
	if (end >= length())
		end = length() - 1;
	return end;
}

/**
 * From the specified position, skip empty lines backwards
 * until we find the index of the line break at the end of
 * a line with text content.
 *
 * @param A position in this string.
 * @return The end of previous line, or the start of string
 * if none found.
 **/
int LocatedString::endOfPreviousNonEmptyLine(int index) const {
	int start = startOfLine(index);
	do {
		start--;
	} while (start >= 0 && (charAt(start) == L'\n' || charAt(start) == L'\r'));
	start++;
	if (start < 0)
		start = 0;
	return start;
}

/**
 * From the specified position, skip characters backwards until
 * we find a line break. If index is a line break, index is returned.
 *
 * @param A position in this string.
 * @return The start of this line, or the start of string if none found.
 **/
int LocatedString::startOfLine(int index) const {
	int start = index;
	do {
		start--;
	} while (start >= 0 && charAt(start) != L'\n' && charAt(start) != L'\r');
	start++;
	if (start < 0)
		start = 0;
	return start;
}


/**
 * From the specified position, skip empty lines forwards
 * until we find the index of the first character of
 * a line with text content.
 *
 * @param A position in this string.
 * @return The start of the line, or the end of string
 * if none found.
 **/
int LocatedString::startOfNextNonEmptyLine(int index) const {
	int end = endOfLine(index);
	do {
		end++;
	} while (end < length() && (charAt(end) == L'\n' || charAt(end) == L'\r'));
	if (end >= length())
		end = length() - 1;
	return end;
}

#ifdef NOBODY_USES_THIS

/**
 * Returns <code>-1</code> if the string is not found.
 *
 * @param str the string to search for.
 * @return the last index of the given string in this LocatedString,
 *         or <code>-1</code> if the string is not found.
 */
int LocatedString::lastIndexOf(const wchar_t *str) const {
	return lastIndexOf(str, length() - 1);
}

/**
 * Searching starts from the right at position <code>start</code>.
 * Returns <code>-1</code> if the string is not found.
 *
 * @param str the string to search for.
 * @param start the first index to start searching at.
 * @return the last index of the given string in this LocatedString,
 *         or <code>-1</code> if the string is not found.
 */
int LocatedString::lastIndexOf(const wchar_t *str, int start) const {
	// Nothing can be found left of the string, not even
	// an empty string.
	if (start < 0) {
		return -1;
	}

	const int str_len = (int)wcslen(str);

	// We can't start searching beyond the point where the target
	// string would go beyond the end of the source string.
	int max_start = _length - str_len;
	if (start > max_start) {
		start = max_start;
	}

	// An empty string is always found immediately.
	if (str[0] == L'\0') {
		return start;
	}

	wchar_t c = str[0];

	// Look for a match of the first character.
	for (int i = start; i >= 0; i--) {
		// If the first character was matched, try to match the
		// whole string. If that succeeds, return the first index.
		if (_source[i] == c) {
			int source_index = i;
			int match_index = 0;
			while ((c != L'\0') &&
				   ((c = str[match_index]) == _source[source_index])) {
				source_index++;
				match_index++;
			}
			if (c == L'\0') {
				return i;
			}
		}
		c = str[0];
	}

	return -1;
}

#endif

/**
 * The characters and offsets are copied from
 * <code>begin_index</code> to the end of this string. As a result,
 * the length of the returned LocatedString is
 * <code>this->length() - begin_index</code>.
 *
 * The low and high offsets are based on the offsets of the
 * substring, not on the low and high offsets of the base string.
 *
 * The caller is responsible for deleting the returned LocatedString
 * when it is no longer needed.
 *
 * This method does not appear to correctly copy some of the EDT offsets of
 * the internal elements (characters) of the string.  It is unclear why this
 * is not currently causing an issue.
 *
 * @param begin_index the begin index of the substring, inclusive.
 * @return a new LocatedString representing a substring of this one.
 */
LocatedString* LocatedString::substring(int begin_index) const {
	return _new LocatedString(*this, begin_index, length());
}

/**
 * The characters and offsets are copied from
 * <code>begin_index</code> to <code>end_index - 1</code>. As a
 * result, the length of the returned LocatedString is
 * <code>end_index - begin_index</code>.
 *
 * The low and high offsets are based on the offsets of the
 * substring, not on the low and high offsets of the base string.
 *
 * The caller is responsible for deleting the returned LocatedString
 * when it is no longer needed.
 *
 * This method does not appear to correctly copy some of the EDT offsets of
 * the internal elements (characters) of the string.  It is unclear why this
 * is not currently causing an issue.
 *
 * @param begin_index the begin index of the substring, inclusive.
 * @param end_index the end index of the substring, exclusive.
 * @return a new LocatedString representing a substring of this one.
 */
LocatedString* LocatedString::substring(int begin_index, int end_index) const {
	return _new LocatedString(*this, begin_index, end_index);
}

// Return true if this LocatedString is a proper substring of the given string.
bool LocatedString::isSubstringOf(const LocatedString *superstring) const {
	int superstring_start_pos = superstring->positionOfStartOffset(start<CharOffset>());
	if (superstring_start_pos < 0) 
		return false;
	if (superstring_start_pos+length() > superstring->length())
		return false;
	if (start<CharOffset>() != superstring->start<CharOffset>(superstring_start_pos))
		return false;
	if (end<CharOffset>() != superstring->end<CharOffset>(superstring_start_pos+length()-1))
		return false;
	if (superstring->_text.compare(superstring_start_pos, length(), _text)!=0)
		return false;
	return true;
}

std::wstring LocatedString::substringAsWString(int start_index, int end_index) const {
	return _text.substr(start_index, end_index-start_index);
}

Symbol LocatedString::substringAsSymbol(int start_index, int end_index) const {
	return Symbol(_text.substr(start_index, end_index-start_index));
}

/* Changes located string to make nonstandard whitespace into more standard chars  */
void LocatedString::replaceNonstandardUnicodeWhitespace(){
	// this code is a portion of Tokenizer::replaceNonstandardUnicodes
	// it belongs in a utility directory but is being tested locally where first used
	int len = static_cast<int>(_text.length());
	char msg[200];
	for (int index = 0; index < len; index++) {
		wchar_t wch = charAt(index);
		msg[0] = 0;
		if (// "horizontal tab"   -- keep this one unchanged for segmenting -- 
			// (wch == 0x0009) ||  
			(wch == 0x00a0) ||  // "no-break space"
			((wch >= 0x2000) && (wch <= 0x200f)) || // 2000-200b are fancy white spaces
		  // 0x200b is "no break space" used for some languages
		  // 200c-200f are typography (include ltr and rtl)
		    ((wch >= 0x202a) && (wch <= 0x202e)) || // embedding direction to print
		// these typography codes fall between tokens so we replace with whitespace
			(wch == 0x202f) ||  // "narrow no-break space"
		  // 0x205f is mathematical space
		  // 0x2060 is "word joiner" 0-width no-break space
			((wch >= 0x205f) && (wch <= 0x2063))){ // 0x2061-0x2063 math separators
				// replace with vanilla space
				sprintf_s(msg, "U+%x with blank at index %d\n",
				  wch, index);
				replace(index,1,L" ");
		}else if ((wch == 0x0085) ||  // new Unicode "next line"
				  (wch == 0x2028))  { // "line separator"	
			// replace with old-fashioned line feed (Unix new line)
				sprintf_s(msg, "U+%x with line feed at index %d\n",
						wch, index);
				replace(index,1,L"\x0a");
		}else if (wch == 0x2029){ // "paragraph separator"
				sprintf_s(msg, "U+%x with double line feed at index %d\n",
						wch, index);
				replace(index,1,L"\x0a\x0a"); // two new lines
				len++;
		}
		if (msg[0] != 0){
			//std::cerr<<msg;
			SessionLogger::dbg("unicode") <<"LocatedString::replaceNonstandardUnicodeWhitespace replacing char " <<msg;
			//std::cerr<<"debug tokenizer: replaceNonstandardUnicodeWhitespace replacing char " <<msg;
		}
	}
}

bool LocatedString::isValidTagName(int pos) const {
	// Get tag name (up to next whitespace/end of tag)
	int end_pos = pos;
	while (end_pos < length() && _text[end_pos] != L'>' && !iswspace(_text[end_pos]))
		end_pos++;
	if (end_pos == pos)
		return false;

	// Check tag name
	SerifXML::xstring name = SerifXML::transcodeToXString(_text.substr(pos, end_pos - pos).c_str());
	return xercesc::XMLChar1_1::isValidName(name.c_str(), name.length());
}

/*
 * Debugging functions
 */

/**
 * @param out the output stream to print to.
 * @param indent the number of spaces to indent each line.
 */
void LocatedString::dump(std::ostream &out, int indent) const {
	char buf[100000];
	StringTransliterator::transliterateToEnglish(buf, _text.c_str(), 99999);
	out << buf;
}

/**
 * @param out the output stream to print to.
 * @param indent the number of spaces to indent each line.
 */
void LocatedString::dumpDetails(std::ostream &out, int indent) const {
	char num_fmt[32];    // a dynamically-created printf-style format string for offset numbers
	char num_str[32];    // a right-aligned string containing an offset number
	char spc_fmt[32];    // a dynamically-created printf-style format string for space characters
	char spc_str[32];    // a string of (width - 1) spaces
	char spc_fmt2[32];   // a dynamically-created printf-style format string for space characters
	char spc_str2[32];   // a string of (width - 2) spaces
	int i = 0;

	// Based on the string width of the largest offset number, figure out
	// the string width of each character to print out.
	int field_width = (int)log10((double)end<CharOffset>().value()) + 2;

	// Create the printf format string for printing out offset numbers
	// in exactly field_width characters.
	sprintf(num_fmt, "%%%dd", field_width);

	// Create a string containing (field_width - 1) adjacent space characters.
	sprintf(spc_fmt, "%%%dc", (field_width - 1));
	sprintf(spc_str, spc_fmt, ' ');

	// Create a string containing (field_width - 2) adjacent space characters.
	sprintf(spc_fmt2, "%%%dc", (field_width - 2));
	sprintf(spc_str2, spc_fmt2, ' ');

	while (i < length()) {
		int j = i;
		int width = 0;

		while ((j < length()) && (width < 70)) {
			char c = (char)_text[j];
			switch (c) {
				case '\t':
					out << spc_str2 << "\\t";
					break;
				case '\r':
					out << spc_str2 << "\\r";
					break;
				case '\n':
					out << spc_str2 << "\\n";
					break;
				default:
					wchar_t wstr[2];
					char str[100];
					wstr[0] = _text[j];
					wstr[1] = L'\0';
					wchar_t* strptr = wstr;
					StringTransliterator::transliterateToEnglish(str, strptr, 99);
					out << spc_str << str[0];
			}
			j++;
			width += field_width;
		}
		out << '\n';

		// Print the absolute offsets.
		j = i;
		width = 0;
		while ((j < length()) && (width < 70)) {
			// Create a string with the offset number right-aligned to the correct width.
			sprintf(num_str, num_fmt, start<CharOffset>(j).value());
			out << num_str;
			j++;
			width += field_width;
		}

		// Print the EDT begin offsets.
		out << "\n";
		j = i;
		width = 0;
		while ((j < length()) && (width < 70)) {
			// Create a string with the offset number right-aligned to the correct width.
			sprintf(num_str, num_fmt, start<EDTOffset>(j).value());
			out << num_str;
			j++;
			width += field_width;
		}

		// Print the EDT end offsets.
		out << "\n";
		j = i;
		width = 0;
		while ((j < length()) && (width < 70)) {
			// Create a string with the offset number right-aligned to the correct width.
			sprintf(num_str, num_fmt, end<EDTOffset>(j).value());
			out << num_str;
			j++;
			width += field_width;
		}

		out << "\n\n";

		i = j;
	}

	out << '\n';
}


void LocatedString::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);
}
void LocatedString::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"LocatedString", this);

	stateSaver->saveInteger(length());
	stateSaver->saveString(_text.c_str());
	stateSaver->beginList(L"LocatedString::_offsets");
	for (int i = 0; i < length(); ++i) {
		stateSaver->saveInteger(start<ByteOffset>(i).value());
		stateSaver->saveInteger(end<ByteOffset>(i).value());
		stateSaver->saveInteger(start<CharOffset>(i).value());
		stateSaver->saveInteger(end<CharOffset>(i).value());
		stateSaver->saveInteger(start<EDTOffset>(i).value());
		stateSaver->saveInteger(end<EDTOffset>(i).value());
		stateSaver->saveReal(start<ASRTime>(i).value());
		stateSaver->saveReal(end<ASRTime>(i).value());
	}
	stateSaver->endList();
	// [XX] do something about this:
	//stateSaver->saveInteger(_min_offset);
	//stateSaver->saveInteger(_max_offset);

	stateSaver->endList();
}

namespace {
	// Helper for LocatedString::LocatedString(StateLoader *stateLoader)
	void loadOffsets(StateLoader *stateLoader, OffsetGroup start, OffsetGroup end) {
		start.byteOffset = ByteOffset(stateLoader->loadInteger());
		end.byteOffset   = ByteOffset(stateLoader->loadInteger());
		start.charOffset = CharOffset(stateLoader->loadInteger());
		end.charOffset   = CharOffset(stateLoader->loadInteger());
		start.edtOffset  = EDTOffset(stateLoader->loadInteger());
		end.edtOffset    = EDTOffset(stateLoader->loadInteger());
		start.asrTime    = ASRTime(stateLoader->loadReal());
		end.asrTime      = ASRTime(stateLoader->loadReal());
	}
}

LocatedString::LocatedString(StateLoader *stateLoader) {
	int id = stateLoader->beginList(L"LocatedString");
	stateLoader->getObjectPointerTable().addPointer(id, this);
	int length = stateLoader->loadInteger();
	_text.append(stateLoader->loadString(), length);
	bool char_offsets_only = (stateLoader->getVersion() <= std::make_pair(1,5));
	if (length > 0) {
		if (char_offsets_only) {
			SessionLogger::warn("located_string") << "loading old LocatedString.  Some offsets will not be available." << std::endl;
			stateLoader->beginList(L"LocatedString::_offsets");
			int prev_offset = stateLoader->loadInteger();
			OffsetGroup offsetGroup = OffsetGroup(CharOffset(prev_offset), EDTOffset(prev_offset));
			_offsets.push_back(OffsetEntry(0, 1, offsetGroup, offsetGroup, false));
			for (int i = 1; i < length; ++i) {
				int offset = stateLoader->loadInteger();
				if (offset == prev_offset + 1) {
					_offsets.back().endOffset = OffsetGroup(CharOffset(prev_offset), EDTOffset(prev_offset));
					_offsets.back().endPos = i+1;
				} else {
					OffsetGroup offsetGroup = OffsetGroup(CharOffset(prev_offset), EDTOffset(prev_offset));
					_offsets.push_back(OffsetEntry(i, i+1, offsetGroup, offsetGroup, false));
				}
				prev_offset = offset;
			}
			stateLoader->endList();
		} else {
			stateLoader->beginList(L"LocatedString::_offsets");

			OffsetGroup prev_start, prev_end;
			loadOffsets(stateLoader, prev_start, prev_end);
			_offsets.push_back(OffsetEntry(0, 1, prev_start, prev_end, false));
			for (int i = 1; i < length; ++i) {
				OffsetGroup start, end;
				loadOffsets(stateLoader, start, end);
				if ((start.byteOffset.value() == prev_end.byteOffset.value()+1) && 
					(start.charOffset.value() == prev_end.charOffset.value()+1) &&
					(start.edtOffset.value() == prev_end.edtOffset.value()+1) &&
					(end.asrTime.value() == prev_end.asrTime.value())) {
					_offsets.back().endOffset = end;
					_offsets.back().endPos = i+1;
				} else {
					_offsets.push_back(OffsetEntry(i, i+1, start, end, false));
				}
				prev_end = end;
				// We don't bother to update prev_start, since we don't use it.
			}
			stateLoader->endList();
		}
	}
	// [XX] Do something with these!
	int min_offset = stateLoader->loadInteger();
	int max_offset = stateLoader->loadInteger();
	stateLoader->endList();
}

void LocatedString::resolvePointers(StateLoader * stateLoader) {
}

//for serializing sofa
LocatedString::RepStruct * LocatedString::getStateRep() const {
	RepStruct * rep = _new RepStruct();
	int len = length();
	rep->length=len;
	rep->source=_text.c_str();
	rep->offsets = _new CharOffset[len];
	rep->asr_start = _new ASRTime[len];
	rep->asr_end = _new ASRTime[len];
	rep->edt_begin = _new EDTOffset[len];
	rep->edt_end = _new EDTOffset[len];

	rep->minoffset=_bounds.start.charOffset.value();
	rep->maxoffset=_bounds.end.charOffset.value();
	for (int i = 0; i < len; ++i) {
		rep->offsets[i]=start<CharOffset>(i);
		rep->edt_begin[i]=start<EDTOffset>(i);
		rep->edt_end[i]=end<EDTOffset>(i);
		rep->asr_start[i]=start<ASRTime>(i);
		rep->asr_end[i]=end<ASRTime>(i);
	}
	return rep;
}

LocatedString::LocatedString(const RepStruct * rep) {
	loadStateRep(rep);
}

//for deserializing sofa
void LocatedString::loadStateRep(const RepStruct * rep) {
	_text = rep->source;
	size_t len = _text.size();

	_bounds = OffsetRange();
	_bounds.start.charOffset = CharOffset(rep->minoffset);
	_bounds.end.charOffset = CharOffset(rep->maxoffset);
	_offsets.clear();

	OffsetGroup prev_start(ByteOffset(), rep->offsets[0], rep->edt_begin[0], rep->asr_start[0]);
	OffsetGroup prev_end(ByteOffset(), rep->offsets[0], rep->edt_begin[0], rep->asr_end[0]);
	_offsets.push_back(OffsetEntry(0, 1, prev_start, prev_end, false));
	for (int i = 1; i < (int)len; ++i) {
		OffsetGroup start(ByteOffset(), rep->offsets[i], rep->edt_begin[i], rep->asr_start[i]);
		OffsetGroup end(ByteOffset(), rep->offsets[i], rep->edt_begin[i], rep->asr_end[i]);
		if ((start.charOffset.value() == prev_end.charOffset.value()+1) &&
			(start.edtOffset.value() == prev_end.edtOffset.value()+1) &&
			(end.asrTime.value() == prev_end.asrTime.value())) {
			_offsets.back().endOffset = end;
			_offsets.back().endPos = i+1;
		} else {
			_offsets.push_back(OffsetEntry(i, i+1, start, end, false));
		}
		prev_end = end;
		// We don't bother to update prev_start, since we don't use it.
	}
}

template<> CharOffset LocatedString::convertStartOffsetTo<CharOffset,CharOffset>(const CharOffset &src) const { return src; }
template<> EDTOffset LocatedString::convertStartOffsetTo<EDTOffset,EDTOffset>(const EDTOffset &src) const { return src; }

template<> CharOffset LocatedString::convertEndOffsetTo<CharOffset,CharOffset>(const CharOffset &src) const { return src; }
template<> EDTOffset LocatedString::convertEndOffsetTo<EDTOffset,EDTOffset>(const EDTOffset &src) const { return src; }

const wchar_t* LocatedString::XMLIdentifierPrefix() const {
	return L"ls";
}

void LocatedString::saveXML(SerifXML::XMLTheoryElement lsElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("LocatedString::saveXML", "Expected context to be NULL");

	// Special handling for empty strings:
	if (length() == 0) {
		lsElem.saveOffsets(_bounds.start, _bounds.end);
		lsElem.addChild(X_Contents); // empty contents.
		return;
	}

	// Save the start and end offsets.
	lsElem.saveOffsets(startOffsetGroup(), endOffsetGroup());

	// If the original text has been serialized, and this LocatedString is
	// a proper substring of the original text, then we're done -- we can
	// reconstruct the string from just the offsets.  Only serialize the 
	// text if an include_spans_as... option is explicitly set. Never 
	// serialize the text content of Regions that are proper substrings, 
	// since they tend to be long.
	const LocatedString *originalText = lsElem.getXMLSerializedDocTheory()->getOriginalText();
	if (originalText && (originalText != this) && isSubstringOf(originalText)) {
		// Contents (optional):
		if (lsElem.getOptions().include_spans_as_elements && !lsElem.hasTag(X_Region)) 
			lsElem.addChild(X_Contents).addText(lsElem.getOriginalTextSubstring(startOffsetGroup(), endOffsetGroup()));
		if (lsElem.getOptions().include_spans_as_comments && !lsElem.hasTag(X_Region))
			lsElem.addComment(lsElem.getOriginalTextSubstring(startOffsetGroup(), endOffsetGroup()));
		return;
	}

	// Otherwise, serialize the text contents.
	lsElem.addChild(X_Contents).addText(toString());

	// If this located string isn't a strict substring, and has multiple offset groups,
	// explicitly output them, even if they're adjacent, because jserif needs them.
	if (_offsets.size() > 1) {
		BOOST_FOREACH(OffsetEntry offsetEntry, _offsets) {
			XMLTheoryElement offsetElem = lsElem.addChild(X_OffsetSpan);
			offsetElem.setAttribute(X_start_pos, offsetEntry.startPos);
			offsetElem.setAttribute(X_end_pos, offsetEntry.endPos);
			offsetElem.saveOffsets(offsetEntry.startOffset, offsetEntry.endOffset);
		}
	}

	// [XX] TODO: deal with ASR offsets.
	// [XX] TODO: deal with byte offsets if the input wasn't utf-8
	// [XX] TODO: this doesn't save bounds -- do we need to?
}

LocatedString::LocatedString(SerifXML::XMLTheoryElement lsElem)
{
	using namespace SerifXML;

	// Load the start and end offsets.
	OffsetGroup startOffset, endOffset;
	lsElem.loadOffsets(startOffset, endOffset);
	_bounds.start = startOffset;
	_bounds.end = endOffset;

	// Get the contents element (if it's available).
	XMLTheoryElement contentsElem = lsElem.getOptionalUniqueChildElementByTagName(X_Contents);

	// If the text is explicitly specified, then store it.  Otherwise,
	// if there's a file URL, then read the text from that file.
	_text = L"";
	if (!contentsElem.isNull()) {
		_text = contentsElem.getText<std::wstring>();
	} else if (lsElem.hasAttribute(X_href)) {
		std::wstring href = lsElem.getAttribute<std::wstring>(X_href);
		if ((href.size() < 8) || href.substr(0,7) != L"file://")
			throw lsElem.reportLoadError(L"Only \"file://path\" URLs are supported; got href=\""+href+L"\""); 
		boost::scoped_ptr<UTF8InputStream> strm_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& strm(*strm_scoped_ptr);
		strm.open(href.substr(7).c_str());
		readTextFromStream(strm);
	}

	// Do we have local text or just offsets into the document string?
	if (_text.length() == 0) {
		// Get the original text
		const LocatedString* originalText = lsElem.getXMLSerializedDocTheory()->getOriginalText();

		// If we have the original text, then just take the appropriate substring.
		// If the original text is not available yet, then we're deserializing it
		// now, so check for an href attribute or an explicit contents element.
		if (originalText) {  
			if (!startOffset.charOffset.is_defined() || !endOffset.charOffset.is_defined())
				throw lsElem.reportLoadError("Expected character offsets");
			// Find the range of positions to copy from the original text.  We assume
			// here that the original text has not been modified (so character offsets 
			// are in a one-to-one relationship with string positions).
			int start_index = startOffset.charOffset.value() - originalText->start<CharOffset>().value();
			int end_index = endOffset.charOffset.value() - originalText->start<CharOffset>().value() + 1;
			// Copy the text from the original text.
			_text.append(originalText->_text.begin()+start_index, originalText->_text.begin()+end_index);
			// Copy the offsets from the original text.
			originalText->copyOffsetsOfSubstring(start_index, end_index, _offsets);
		} else {
			throw lsElem.reportLoadError("Expected document originalText, href attribute, or <Contents> child element");
		}
	} else {
		// Read any offset information that is stored with the located string.
		// If not offset information is available, then reconstruct it from
		// scratch.
		XMLTheoryElementList offsetElems = lsElem.getChildElementsByTagName(X_OffsetSpan, false);
		if (offsetElems.empty()) {

			// Supply default values for offsets that were not supplied. 
			// Should these defaults be more intelligent?  E.g., what if
			// the user supplies a start char but no start edt?  should we
			// use the start char as the start edt?
			if (!startOffset.byteOffset.is_defined())
				startOffset.byteOffset = ByteOffset(0);
			if (!startOffset.charOffset.is_defined())
				startOffset.charOffset = CharOffset(0);
			if (!startOffset.edtOffset.is_defined())
				startOffset.edtOffset = EDTOffset(0);

			// Calculate per-character offsets.
			initialize_offsets(startOffset);
		} else {
			BOOST_FOREACH(XMLTheoryElement offsetElem, offsetElems) {
				_offsets.push_back(OffsetEntry());
				OffsetEntry &entry = _offsets.back();
				offsetElem.loadOffsets(entry.startOffset, entry.endOffset);
				entry.startPos = offsetElem.getAttribute<int>(X_start_pos);
				entry.endPos = offsetElem.getAttribute<int>(X_end_pos);
				entry.is_edt_skip_region = ((entry.endPos-entry.startPos > 2) &&
											(entry.startOffset.edtOffset == entry.endOffset.edtOffset));
			}
		}

		initialize_bounds(false);

		// Perform a sanity check vs end offsets.
		if (endOffset.byteOffset.is_defined() && endOffset.byteOffset != end<ByteOffset>())
			lsElem.reportLoadWarning("End byte offset inconsistent with text content and start offset");
		if (endOffset.charOffset.is_defined() && endOffset.charOffset != end<CharOffset>())
			lsElem.reportLoadWarning("End char offset inconsistent with text content and start offset");
		if (endOffset.edtOffset.is_defined() && endOffset.edtOffset != end<EDTOffset>())
			lsElem.reportLoadWarning("End EDT offset inconsistent with text content and start offset");
	}
}
