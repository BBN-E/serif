// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LOCATED_STRING_H
#define LOCATED_STRING_H

#include "Generic/theories/Theory.h"
#include "Generic/common/Offset.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/InputStream.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UnicodeUtil.h"

#include <cstdarg>
#include <wchar.h>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <assert.h>

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

// If this flag is enabled, then characters inside xml-like tags
// will be given an "undefined" EDT offset.  It would be good to
// enable this eventually, but currently there is some code that
// breaks if it is enabled.
//#define USE_UNDEFINED_OFFSETS_FOR_SKIP_EDT_REGIONS

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;

/**
 * Class for storing and manipulating strings that have been read in 
 * from a file, without losing the relationship between each character
 * and its origin in the file from which it was read.  In particular,
 * for each character in the located string, we record a start offset
 * and an end offset of each offset type (ByteOffset, CharOffset,
 * EDTOffset, and ASRTime).  Start offsets and end offsets are zero-
 * indexed, and both are inclusive.  E.g., if a character in the string 
 * came from a single byte at position 12, then that character's start 
 * ByteOffset and end ByteOffset will both be 12.  For a character
 * that was encoded using three bytes at positions 14, 15, and 16, the
 * start ByteOffset will be 14, and the end ByteOffset will be 16.
 *
 * In unmodified LocatedStrings, the start CharOffset for each character
 * will be equal to its end CharOffset.  However, modifications that
 * replace substrings can result in individual characters whose start 
 * and end offsets are not equal, since the offsets of the replacement
 * characters are set based on the entire range of characters in the 
 * replaced substring.
 *
 * The four offset types that are currently stored for each character are:
 * 
 *   - CharOffset.  More accurately, this is a unicode code point offset.
 *
 *   - ByteOffset.  Currently, we assume that the source string was UTF-8, 
 *     and calculate byte offsets by checking how many bytes it would take
 *     to encode each character.  In the future, if we did our own unicode
 *     encoding, we could directly read byte offsets for other encodings.
 *
 *   - EDTOffset.  EDT offsets are similra to character offsets, except that
 *       (i) any substrings starting with "<" and extending to the 
 *           matching ">" are skipped when counting offsets; and 
 *       (ii) the character "\r" is skipped when counting offsets.  
 *     Note that condition (i) is *not* always identical to skipping 
 *     XML/SGML tags and comments.
 *
 *   - ASRTime.  The start and end time of the speech signal that corresponds 
 *     to a character.  ASRTime must be set after a LocatedString is 
 *     constructed, using setAsrStartTime() and setAsrEndTime().
 *
 * @author originally by David A. Herman, refactored by Edward Loper
 */
class SERIF_EXPORTED LocatedString : public Theory {

public: /* ================ constructors & destructor ================= */

	/** Construct a new LocatedString from the given source string.  All offsets
	  * will begin at zero. */
	LocatedString(const wchar_t *source);

	/** Construct a new LocatedString by reading from a given stream.  All 
	  * offsets will begin at zero. */
	LocatedString(InputStream &stream);

	/** Construct a new LocatedString by reading from a given stream until a 
	  * specified delimiter character is encountered, or the end of the stream 
	  * is reached. All offsets will begin at zero. */
	LocatedString(InputStream &stream, wchar_t delim);

	/** Construct a new LocatedString from the given source string.  Offsets 
	  * will begin with the specified values. */
	LocatedString(const wchar_t *source, OffsetGroup start);

	// Destructor
	~LocatedString() {}

public: /* ================ factory methods ================= */

	/** Returns a substring of this string from start_index to end_index.
	  * The caller is responsible for deleting the returned LocatedString. */
	LocatedString* substring(int start_index, int end_index) const;

	/** Returns a substring of this string from start_index to the end of the string.
	  * The caller is responsible for deleting the returned LocatedString. */
	LocatedString* substring(int start_index) const;

	/** Returns a substring of this string as a Symbol */
	Symbol substringAsSymbol(int start_index, int end_index) const;

	/** Returns a substring of this string as a std::wstring */
	std::wstring substringAsWString(int start_index, int end_index) const;

public: /* ================ non-const (modifying) methods ================= */

	/** Insert a given string immediately before the character at position pos.
	  * The newly added characters will all have the same offsets.  If the new
	  * string is inserted at the beginning of the string, then the start and
	  * end offset will both be set to the start offset of the first character.
	  * Otherwise, thhe start and end offset will both be set to the end offset
	  * of the character at position pos. */
	void insert(const wchar_t *str, int pos);

	/** Insert a given string with EDT ranges that match pos.  
	  * The newly added characters will all have the same as pos.  */
	void insertAtPOSWithSameOffsets(const wchar_t *str, int pos);

	/** Append a string to the end of this LocatedString.  New characters'
	  * start and end offsets will be set to the end offset of the last 
	  * character in the string. */
	void append(const wchar_t *str);

	/** Append a located string to the end of this LocatedString.  This appends
	  * each character in locstr to this string, keeping whatever offset information
	  * it originally contained.  In addition, the 'originalEnd' of this 
	  * string is set to locstr.originalEnd().*/
	void append(const LocatedString& locstr);

	/** Remove the given range of characters from the LocatedString.  The offsets
	  * of the remaining characters are not modified in any way. */
	void remove(int start, int end);
	
	/** Remove all (non-overlapping) occurences of the given string. */
	void remove(const wchar_t *find);

	/** Replace the substring starting at character pos (inclusive) and ending
	  * to character pos+len (inclusive) with the given replacement text.  The
	  * replaced characters' start offsets are set to the start offset of 
	  * the replaced substring (i.e., the start offset of the character at 
	  * position pos); and their end offsets are set to the end offset of the 
	  * replaced substring (i.e., the end offset of the character at position 
	  * pos+len-1). */
	void replace(int pos, int len, const wchar_t *repl);

	/** Find any occurences of the string `find`, and replace them with `repl`.
	  * See above for a description of how the offsets of the replacement
	  * characters are set. */
	void replace(const wchar_t *find, const wchar_t *repl);

	/** Find any occurences of the string `find`, and replace them with `repl`.
	  * See above for a description of how the offsets of the replacement
	  * characters are set. */
	void replace( const std::wstring & find, const std::wstring & repl);

		/** Find any occurance of the string 'find' and replace with 'repl'.
	*	See above description of how the offsets of the replacement characters are set.
	*	This replace continues to look where it last found an occurance of 'find'
	*	So if 'find'="&amp;" and 'repl'="&", then "&amap;amp;amp" will be replaced with "&"
	*/
	void replaceRecursive(const std::wstring & find, const std::wstring & repl);

	/** Replace all substrings found between 'findStart' and 'findEnd' with repl, including 'findStart' and 'findEnd'.
	*/
	void replaceBetween(const std::wstring & findStart, const std::wstring & findEnd, const std::wstring & repl);

	/** Remove leading and trailing whitespace. */
	void trim();

	/** Convert the characters to lowercase.  Offsets are not modified. */
	void toLowerCase();

	/** Convert the characters to uppercase.  Offsets are not modified. */
	void toUpperCase();

	/** Replace white space in unicode that is not standard ascii-type with more standard forms */
	void replaceNonstandardUnicodeWhitespace();

	/** Helper to check if the token at the specified position is a valid tag name */
	bool isValidTagName(int pos) const;

	/** Set the ASR start time for the specified character. */
	void setAsrStartTime(int pos, ASRTime time) { _setStartOffset(pos, time); }

	/** Set the ASR end time for the specified character. */
	void setAsrEndTime(int pos, ASRTime time) { _setEndOffset(pos, time); }

	///** Assign begin and end ASR times to all characters that currently
	//  * have undefined times, by interpolating between characters that
	//  * have been assigned times with setAsrStartTime() and setAsrEndTime(). */
	//void interpolateAsrTimes(void); // not implemented.

public: /* ================ const (non-modifying) methods ================= */
	/** Return the character at position <code>pos</code> in the string. */
	wchar_t charAt(int pos) const {
		if ((pos < 0) || (pos >= static_cast<int>(_text.size()))) 
			throw InternalInconsistencyException::arrayIndexException(
				"LocatedString::charAt", length(), pos);
		return _text[pos];
	}

	/** Return the character at position <code>pos</code> in the string, without
	    performing any bounds checks.  This should only be used if/when profiling
		shows that it is helpful. */
	wchar_t charAtNoBoundsCheck(int pos) const { return _text[pos]; }

	/** Return the length of this located string (i.e., the number of
	  * characters it contains). */
	int length() const { return static_cast<int>(_text.size()); }

	/** Return a null-terminated string containing this string's text.
	  * This string is owned by the LocatedString.  It is invalidated if the
	  * LocatedString is modified or destroyed. */
	const wchar_t* toString() const;
	
	/** Return a std::wstring containing this string's text. */
	const std::wstring& toWString() const { return _text; }
	
	/** Return a Symbol containing this located string's text. */
	Symbol toSymbol() const;

	/** Return the index of the first occurence of the given 
	  * substring ('str') in this string that occurs at or after the
	  * given position ('start').  If it is not found, return -1.  */
	int indexOf(const wchar_t *str, int start=0) const;

	/** Methods to skip empty lines */
	int endOfLine(int index) const;
	int endOfPreviousNonEmptyLine(int index) const;
	int startOfLine(int index) const;
	int startOfNextNonEmptyLine(int index) const;

	/** Print a representation of this string to the given output stream for debugging. */
	void dump(std::ostream &out, int indent = 0) const;

	/** Return an OffsetGroup containing the start offsets of the character at the specified position. */
	OffsetGroup startOffsetGroup(int pos=0) const;

	/** Return an OffsetGroup containing the end offsets of the character at the specified position. */
	OffsetGroup endOffsetGroup(int pos) const;
	OffsetGroup endOffsetGroup() const {return endOffsetGroup(length()-1); }

	/** Return the start offset with the given OffsetType at the specified position 
	  * in the LocatedString.  If no position is specified, return the start offset 
	  * of the first character.  This method requires a template argument to specify 
	  * the offset type that should be returned.  E.g.: locstr.start<CharOffset>(pos) */
	template<typename OffsetType>
	OffsetType start(int pos=0) const {
		OffsetType result;
		getStartOffset(pos, result);
		return result;
	}

	/** Return the end offset with the given OffsetType at the specified position 
	  * in the LocatedString.  If no position is specified, return the end offset 
	  * of the last character.  This method requires a template argument to specify 
	  * the offset type that should be returned.  E.g.: locstr.end<CharOffset>(pos) */
	template<typename OffsetType>
	OffsetType end(int pos) const {
		OffsetType result;
		getEndOffset(pos, result);
		return result;
	}
	template<typename OffsetType>
	OffsetType end() const { return end<OffsetType>(length()-1); }

	/** Return the start offset that the first character of this LocatedString
	  * had before any modifications were made.  This may be lower than the 
	  * start offset of the first character of the current LocatedString if any
	  * of the starting characters were removed from the string using trim() or
	  * remove().  This method requires a template argument to specify the offset 
	  * type that should be returned.  E.g.: locstr.originalStart<CharOffset>() */
	template<typename OffsetType>
	OffsetType originalStart() const { return _bounds.start.value<OffsetType>(); }

	/** Return the end offset that the last character of this LocatedString
	  * had before any modifications were made.  This may be higher than the 
	  * end offset of the last character of the current LocatedString if any
	  * of the starting characters were removed from the string using trim() or
	  * remove().  This method requires a template argument to specify the offset 
	  * type that should be returned.  E.g.: locstr.originalEnd<CharOffset>() */
	template<typename OffsetType>
	OffsetType originalEnd() const { return _bounds.end.value<OffsetType>(); }

public: // I'm not entirely happy with these names:
	/** Starting at the specified position (inclusive), search forward for the 
	  * first character which has a defined start offset with the given OffsetType; 
	  * and return that offset.  If no characters at or after pos have defined 
	  * offsets, throw InternalInconsistencyException.  This method requires a 
	  * template argument to specify the offset type that should be returned.  
	  * E.g.: locstr.firstDefinedStart<CharOffset>(pos) */
	template<typename OffsetType>
	OffsetType firstStartOffsetStartingAt(int pos) const;

	/** Starting at the specified position (inclusive), search backward for the first character
	  * which has a defined end offset with the given OffsetType; and return that offset.  If
	  * no characters at or before pos have defined offsets, throw InternalInconsistencyException. */
	/** Starting at the specified position (inclusive), search backward for the 
	  * first character which has a defined end offset with the given OffsetType; 
	  * and return that offset.  If no characters at or before pos have defined 
	  * offsets, throw InternalInconsistencyException.  This method requires a 
	  * template argument to specify the offset type that should be returned.  
	  * E.g.: locstr.firstDefinedEnd<CharOffset>(pos) */
	template<typename OffsetType>
	OffsetType lastEndOffsetEndingAt(int pos) const;

public:
	/** Return the position of the first character in this LocatedString whose start offset 
	  * (with the given type) is equal to the specified offset.  If no character with the
	  * requested offset is found, then return -1. */
	template<typename OffsetType>
	int positionOfStartOffset(OffsetType offset) const;

	/** Return the position of the first character in this LocatedString whose end offset 
	  * (with the given type) is equal to the specified offset.  If no character with the
	  * requested offset is found, then return -1. */
	template<typename OffsetType>
	int positionOfEndOffset(OffsetType offset) const;

	/** Search for a character with a given start offset value of one type 
	  * (SourceOffsetType), and return the corresponding start offset value
	  * of a different type (ReturnOffsetType).  E.g., this can be used to
	  * convert a CharOffset to a ByteOffset or an EDTOffset.  This method 
	  * requires a template argument to specify the offset type that should be 
	  * returned.  E.g.: locstr.convertStartOffsetTo<ByteOffset>(myCharOffset) */
	template<typename ReturnOffsetType, typename SourceOffsetType>
	ReturnOffsetType convertStartOffsetTo(const SourceOffsetType &src) const {
		return start<ReturnOffsetType>(positionOfStartOffset(src)); }

	/** Search for a character with a given end offset value of one type 
	  * (SourceOffsetType), and return the corresponding end offset value
	  * of a different type (ReturnOffsetType).  E.g., this can be used to
	  * convert a CharOffset to a ByteOffset or an EDTOffset.  This method 
	  * requires a template argument to specify the offset type that should be 
	  * returned.  E.g.: locstr.convertEndOffsetTo<ByteOffset>(myCharOffset) */
	template<typename ReturnOffsetType, typename SourceOffsetType>
	ReturnOffsetType convertEndOffsetTo(const SourceOffsetType &src) const {
		return end<ReturnOffsetType>(positionOfEndOffset(src)); }

	/** Return true if this LocatedString occurs verbatim as a substring of the given
	  * LocatedString.  In particular, return true if (for some i and j) 
	  * superstring.substring(i,j) has the same characters and offsets as this
	  * LocatedString.  (The offset bounds are not checked.) */
	bool isSubstringOf(const LocatedString *superstring) const;

private: /* ================ streaming output ================= */
	friend std::ostream &operator <<(std::ostream &out, LocatedString &s) {
		s.dump(out, 0);
		return out;
	}
public:

	/// Print a representation of this string and its offsets to the given output stream for debugging.
	void dumpDetails(std::ostream &out, int indent = 0) const;

	// A mapping from a range of positions to a range of offsets.
	struct OffsetEntry { 
		int startPos;
		int endPos; // exclusive
		OffsetGroup startOffset;
		OffsetGroup endOffset; // inclusive
		bool is_edt_skip_region;
		OffsetEntry(): startPos(0), endPos(0), startOffset(), endOffset(), is_edt_skip_region(0) {}
		OffsetEntry(int startPos, int endPos, 
			const OffsetGroup &startOffset, const OffsetGroup &endOffset, bool is_edt_skip_region)
			: startPos(startPos), endPos(endPos), 
			startOffset(startOffset), endOffset(endOffset), 
			is_edt_skip_region(is_edt_skip_region) {}
	};

private: /* ================ member variables ================= */

	// The underlying string.  Note that this string may not match the
	// original text string if any non-const methods have been used.
	std::wstring _text;

	// A pair of offsets forms an offset range.
	struct OffsetRange { 
		OffsetGroup start;
		OffsetGroup end;
		OffsetRange() {} // All offsets initialized to Offset::undefined_value().
		OffsetRange(const OffsetGroup &start, const OffsetGroup &end): start(start), end(end) {}
	};

	// A compact representation of the offsets in this located string.
	std::vector<OffsetEntry> _offsets;

	// Original start/end of the entire string.  This will be only be different
	// from the start/end offsets of the first/last character if text has been
	// deleted from the beginning or end of the located string.
	OffsetRange _bounds;

public:

	/** Construct a new LocatedString from the given source string. The
	  * specified offsets will be used. */
	LocatedString(const LocatedString &source, std::list<OffsetEntry> offsets);

private: /* ================ helper methods ================= */

	// Loading from a stream.
	void initFromStream(InputStream &stream);

	// Loading from a stream -- does not set offsets.
	void readTextFromStream(InputStream &stream);

	// Substring constructor
	LocatedString(const LocatedString &s, int start_index, int end_index);

	// Method used by constructors to initialize offsets.
	void initialize_offsets(OffsetGroup start=OffsetGroup(ByteOffset(0),CharOffset(0),EDTOffset(0),ASRTime()));

	// Method used by constructors to initialize bounds.
	void initialize_bounds(bool empty_is_undefined = true);

	// Raise an exception if the given position is not within the bounds of this string.
	inline void check_bounds(int pos, const char* method_name) const {
		if ((pos < 0) || (pos >= length())) 
			throw InternalInconsistencyException::arrayIndexException(method_name, length(), pos);
	}

	// Modify a character's start offset
	template<typename OffsetType>
	inline void _setStartOffset(int pos, OffsetType new_value);

	// Modify a character's end offset
	template<typename OffsetType>
	inline void _setEndOffset(int pos, OffsetType new_value);

	// Return the last OffsetEntry whose position is less than or equal to pos.
	size_t findOffsetEntryBefore(int pos) const;

	// Copy the offsets for a substring of this string to the destination offset
	// array dst.
	void copyOffsetsOfSubstring(int begin_index, int end_index, std::vector<OffsetEntry> &dst) const;

	void splitOffsetsAtPos(int pos, size_t entry_num);

	bool checkOffsetInvariants() const;

	void insert(const wchar_t *str, int pos, const OffsetRange &new_offset);

public: /* ================ Serialization ================= */
	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	LocatedString(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	const wchar_t* XMLIdentifierPrefix() const;
	LocatedString(SerifXML::XMLTheoryElement elem);

public: /* ================ Serialization ================= */
	class RepStruct {
	public:
		int length;
		const wchar_t * source;
		CharOffset * offsets;
		EDTOffset * edt_begin;
		EDTOffset * edt_end;
		ASRTime * asr_start;
		ASRTime * asr_end;

		int minoffset;
		int maxoffset;
		RepStruct() {
			length=0;
			source=NULL;
			offsets=NULL;
			edt_begin=NULL;
			edt_end=NULL;
			asr_start=NULL;
			asr_end=NULL;
		}
		~RepStruct() {
			if (source!=NULL) {delete [] source;}
			if (offsets!=NULL) {delete [] offsets;}
			if (edt_begin!=NULL) {delete [] edt_begin;}
			if (edt_end!=NULL) {delete [] edt_end;}
			if (asr_start!=NULL) {delete [] asr_start;}
			if (asr_end!=NULL) {delete [] asr_end;}
		}
	};
	RepStruct * getStateRep() const; //This method allows non-statesaver objects to get a representation of this LocatedString
	void loadStateRep(const RepStruct * rep);//This method allows the creation of a LocateString from non-statesaver
	LocatedString(const RepStruct *rep);

private: /* ================ DEPRECATED METHODS ================= */
	int offsetAt(...) const;                        // DEPRECATED: use start<CharOffset>() instead.
	int edtBeginOffsetAt(...) const;                // DEPRECATED: use start<EDTOffset>() instead.
	int edtEndOffsetAt(...) const;                  // DEPRECATED: use end<EDTOffset>() instead.
	float asrStartAt(...) const;                    // DEPRECATED: use start<ASRTime>() instead.
	float asrEndAt(...) const;                      // DEPRECATED: use end<ASRTime>() instead.
	int positionOfOriginalOffset(...) const;        // DEPRECATED: use positionOfStartOffset() instead.
	int positionOfBeginEDTOffset(...) const;        // DEPRECATED: use positionOfStartOffset() instead.
	int positionOfEndEDTOffset(...) const;          // DEPRECATED: use positionOfEndOffset() instead.
	int firstEdtOffsetStartingAt(...) const;        // DEPRECATED: use firstStartOffsetStartingAt<EDTOffset>() instead.
	int lastEdtOffsetEndingAt(...) const;           // DEPRECATED: use lastEndOffsetEndingAt<EDTOffset>() instead.
	int getFirstOffset(...) const;                  // DEPRECATED: use start<CharOffset>() instead.
	int getLastOffset(...) const;                   // DEPRECATED: use end<CharOffset>() instead.
	int getOriginalMinOffset(...) const;            // DEPRECATED: use originalStart<CharOffset>() instead.
	int getOriginalMaxOffset(...) const;            // DEPRECATED: use originalEnd<CharOffset>() instead.
	void destroyAll(...);                           // DEPRECATED: use remove() instead.
	void replaceRight(...);                         // DEPRECATED: use remove() instead.

public:
	void getStartOffset(int pos, CharOffset& result) const {
		const OffsetEntry& entry = _offsets[findOffsetEntryBefore(pos)];
		assert(pos >= entry.startPos && pos <= (entry.endPos-1));
		if (pos == entry.startPos)
			result = entry.startOffset.charOffset;
		else
			result = CharOffset(entry.startOffset.charOffset.value() + (pos-entry.startPos));
	}

	void getEndOffset(int pos, CharOffset& result) const {
		const OffsetEntry& entry = _offsets[findOffsetEntryBefore(pos)];
		assert(pos >= entry.startPos && pos <= (entry.endPos-1));
		if (pos == (entry.endPos-1))
			result = entry.endOffset.charOffset;
		else
			result = CharOffset(entry.startOffset.charOffset.value() + (pos-entry.startPos));
	}

	void getStartOffset(int pos, EDTOffset& result) const {
		const OffsetEntry& entry = _offsets[findOffsetEntryBefore(pos)];
		assert(pos >= entry.startPos && pos <= (entry.endPos-1));
		if (entry.is_edt_skip_region)
			#ifdef USE_UNDEFINED_OFFSETS_FOR_SKIP_EDT_REGIONS
				result = EDTOffset();
			#else
				result = entry.startOffset.edtOffset;
			#endif
		else if (pos == entry.startPos)
			result = entry.startOffset.edtOffset;
		else
			result = EDTOffset(entry.startOffset.edtOffset.value() + (pos-entry.startPos));
	}

	void getEndOffset(int pos, EDTOffset& result) const {
		const OffsetEntry& entry = _offsets[findOffsetEntryBefore(pos)];
		assert(pos >= entry.startPos && pos <= (entry.endPos-1));
		if (entry.is_edt_skip_region)
			#ifdef USE_UNDEFINED_OFFSETS_FOR_SKIP_EDT_REGIONS
				result = EDTOffset();
			#else
				result = entry.startOffset.edtOffset;
			#endif
		else if (pos == (entry.endPos-1))
			result = entry.endOffset.edtOffset;
		else
			result = EDTOffset(entry.startOffset.edtOffset.value() + (pos-entry.startPos));
	}

	void getStartOffset(int pos, ASRTime& result) const {
		const OffsetEntry& entry = _offsets[findOffsetEntryBefore(pos)];
		assert(pos >= entry.startPos && pos <= (entry.endPos-1));
		if (pos == entry.startPos)
			result = entry.startOffset.asrTime;
		else
			result = ASRTime();
	}

	void getEndOffset(int pos, ASRTime& result) const {
		const OffsetEntry& entry = _offsets[findOffsetEntryBefore(pos)];
		assert(pos >= entry.startPos && pos <= (entry.endPos-1));
		if (pos == (entry.endPos-1))
			result = entry.startOffset.asrTime;
		else
			result = ASRTime();
	}

	void getStartOffset(int pos, ByteOffset& result) const {
		const OffsetEntry& entry = _offsets[findOffsetEntryBefore(pos)];
		assert(pos >= entry.startPos && pos <= (entry.endPos-1));
		int byteOffset = entry.startOffset.byteOffset.value();
		for (int i=entry.startPos; i<pos; i++)
			byteOffset += UnicodeUtil::countUTF8Bytes(_text[i]);
		result = ByteOffset(byteOffset);
	}

	void getEndOffset(int pos, ByteOffset& result) const {
		const OffsetEntry& entry = _offsets[findOffsetEntryBefore(pos)];
		assert(pos >= entry.startPos && pos <= (entry.endPos-1));
		if (pos == (entry.endPos-1))
			result = entry.endOffset.byteOffset;
		else {
			int byteOffset = entry.startOffset.byteOffset.value();
			for (int i=entry.startPos; i<=pos; i++)
				byteOffset += UnicodeUtil::countUTF8Bytes(_text[i]);
			result = ByteOffset(byteOffset-1); // Byte offsets are inclusive, so subtract one.
		}
	}
};

inline OffsetGroup LocatedString::startOffsetGroup(int pos) const {
	check_bounds(pos, "LocatedString::startOffsetGroupAt");
	return OffsetGroup(start<ByteOffset>(pos), start<CharOffset>(pos), start<EDTOffset>(pos), start<ASRTime>(pos));
}

inline OffsetGroup LocatedString::endOffsetGroup(int pos) const {
	check_bounds(pos, "LocatedString::endOffsetGroupAt");
	return OffsetGroup(end<ByteOffset>(pos), end<CharOffset>(pos), end<EDTOffset>(pos), end<ASRTime>(pos));
}

template<> CharOffset LocatedString::convertStartOffsetTo<CharOffset,CharOffset>(const CharOffset &src) const;
template<> EDTOffset LocatedString::convertStartOffsetTo<EDTOffset,EDTOffset>(const EDTOffset &src) const;

template<> CharOffset LocatedString::convertEndOffsetTo<CharOffset,CharOffset>(const CharOffset &src) const;
template<> EDTOffset LocatedString::convertEndOffsetTo<EDTOffset,EDTOffset>(const EDTOffset &src) const;

inline size_t LocatedString::findOffsetEntryBefore(int pos) const {
	size_t i = 1;
	while (i<_offsets.size() && _offsets[i].startPos<=pos)
		++i;
	return i-1;
}

/*
template<typename OffsetType>
inline OffsetType LocatedString::start(int pos) const {
	check_bounds(pos, "LocatedString::startOffsetAt");
	return _offsets[pos].start.value<OffsetType>();
}

template<typename OffsetType>
inline OffsetType LocatedString::end(int pos) const {
	check_bounds(pos, "LocatedString::endOffsetAt");
	return _offsets[pos].end.value<OffsetType>();
}
*/

template<typename OffsetType>
inline OffsetType LocatedString::firstStartOffsetStartingAt(int pos) const {
	check_bounds(pos, "LocatedString::firstOffsetStartingAt");
	while ((pos < length()) && (!(start<OffsetType>(pos).is_defined()))) ++pos;
	if (pos >= length())
		throw InternalInconsistencyException("LocatedString::firstOffsetStartingAt", 
			"No valid offsets after the specified position.");
	return start<OffsetType>(pos);
}

template<typename OffsetType>
inline OffsetType LocatedString::lastEndOffsetEndingAt(int pos) const {
	if ((pos < 0) || (pos >= length())) 
		throw InternalInconsistencyException::arrayIndexException(
			"LocatedString::lastOffsetEndingAt", length(), pos);
	while ((pos > 0) && (!(end<OffsetType>(pos).is_defined()))) --pos;
	if (pos < 0)
		throw InternalInconsistencyException("LocatedString::lastOffsetEndingAt", 
			"No valid offsets after the specified position.");
	return end<OffsetType>(pos);
}

template<typename OffsetType>
inline int LocatedString::positionOfStartOffset(OffsetType offset) const {
	for (int i=0; i<length(); ++i)
		if (start<OffsetType>(i) == offset)
			return i;
	return -1;
}

template<>
inline int LocatedString::positionOfStartOffset(CharOffset offset) const {
	for (std::vector<OffsetEntry>::const_iterator it=_offsets.begin(); it!=_offsets.end(); ++it) {
		if (it->startOffset.charOffset > offset)
			return -1;
		if (offset <= it->endOffset.charOffset)
			return it->startPos + (offset.value() - it->startOffset.charOffset.value());
	}
	return -1;
}

template<>
inline int LocatedString::positionOfStartOffset(EDTOffset offset) const {
	for (std::vector<OffsetEntry>::const_iterator it=_offsets.begin(); it!=_offsets.end(); ++it) {
		if (it->startOffset.edtOffset > offset)
			return -1;
		if ((offset <= it->endOffset.edtOffset) && !it->is_edt_skip_region)
			return it->startPos + (offset.value() - it->startOffset.edtOffset.value());
	}
	return -1;
}

template<typename OffsetType>
inline int LocatedString::positionOfEndOffset(OffsetType offset) const {
	for (int i=0; i<length(); ++i)
		if (end<OffsetType>(i) == offset)
			return i;
	return -1;
}

template<>
inline int LocatedString::positionOfEndOffset(CharOffset offset) const {
	for (std::vector<OffsetEntry>::const_iterator it=_offsets.begin(); it!=_offsets.end(); ++it) {
		if (it->startOffset.charOffset > offset)
			return -1;
		if (offset == it->endOffset.charOffset)
			return it->endPos-1;
		if (offset < it->endOffset.charOffset)
			return it->startPos + (offset.value() - it->startOffset.charOffset.value());
	}
	return -1;
}

template<>
inline int LocatedString::positionOfEndOffset(EDTOffset offset) const {
	for (std::vector<OffsetEntry>::const_iterator it=_offsets.begin(); it!=_offsets.end(); ++it) {
		if (it->startOffset.edtOffset > offset)
			return -1;
		if (!it->is_edt_skip_region) {
			if (offset == it->endOffset.edtOffset)
				return it->endPos-1;
			if (offset < it->endOffset.edtOffset)
				return it->startPos + (offset.value() - it->startOffset.edtOffset.value());
		}
	}
	return -1;
}

template<typename OffsetType>
inline void LocatedString::_setStartOffset(int pos, OffsetType new_value) {
	if ((pos < 0) || (pos >= length())) 
		throw InternalInconsistencyException::arrayIndexException(
			"LocatedString::setOffsetAt", length(), pos);
	size_t entry_num = findOffsetEntryBefore(pos);
	OffsetEntry& entry = _offsets[entry_num];
	if (entry.startPos == pos) {
		entry.startOffset.setOffset(new_value);
	} else {
		splitOffsetsAtPos(pos, entry_num);
		_offsets[entry_num+1].startOffset.setOffset(new_value);
	}
}

template<typename OffsetType>
inline void LocatedString::_setEndOffset(int pos, OffsetType new_value) {
	if ((pos < 0) || (pos >= length())) 
		throw InternalInconsistencyException::arrayIndexException(
			"LocatedString::setOffsetAt", length(), pos);
	size_t entry_num = findOffsetEntryBefore(pos);
	OffsetEntry& entry = _offsets[entry_num];
	if (entry.endPos == pos) {
		entry.endOffset.setOffset(new_value);
	} else {
		splitOffsetsAtPos(pos+1, entry_num);
		_offsets[entry_num].endOffset.setOffset(new_value);
	}
}



#endif
