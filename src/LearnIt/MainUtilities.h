#pragma once

#include "Generic/common/Offset.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/ValueMention.h"
#include <vector>
#include <string>
#include "boost/shared_ptr.hpp"

class MainUtilities {
public:

	/** Return a normalized version of the given string.  In particular, remove
	  * accents, convert to lower case, remove select punctuation, normalize
	  * whitespace, and (English only) strip articles. */
	static std::wstring normalizeString(const std::wstring& input_string);

	/** Return a normalized version of the given string consistent with wstring version of normalizeString. 
	  * To maintain consistency:
	  *   (1) Convert input_string to a wstring, 
	  *   (2) Call normalizeString() on the wstring
	  *   (3) Convert the returned wstring back to a string
	  *   (4) Return the string 	
	 */
	static std::string normalizeString(const std::string& input_string);

	/** Return a copy of the given string with special XML characters (such
	  * as '&' and '<' appropriately escaped. */
	static std::wstring encodeForXML(std::wstring const &s);

	/* try to align serif objects to offsets */
	/** Try to find the token span that matches the start and end offsets given.
	 *  Returns true if a span is found
	 *  Returns false (and prints a warning message if no span is found (or if the span would cross senten boundaries
	 */
	static bool getSerifStartEndTokenFromCharacterOffsets(const DocTheory* docTheory, EDTOffset c_start, EDTOffset c_end, 
		int& sent_num, int& tok_start, int& tok_end);
	/**
	 * Try to find a SynNode that aligns to the token span (returns 0 if there is not an exact match)
	 * Note:  If we wanted to any node that covered the the token-span, use SynNode::getCoveringNodeFromTokenSpan()
	 */	
	static const SynNode* getParseNodeFromTokenOffsets(const SentenceTheory* sentTheory, int start_token, int end_token);
	/** 
	* Try to find a Mention that aligns to the token span (returns 0 if no match can be found).
	* Prefers to return Mentions with Mention Type != NONE, but will return any mention.
	* Will back-off to matching heads if an extent match cannot be found
	* May return 'headless' mentions.
	*/
	static const Mention* getMentionFromTokenOffsets(const SentenceTheory* sentTheory, int start_token, int end_token);
	/** 
	* Try to find a ValueMention that aligns to the token span (returns 0 if no match can be found).
	* Requires exact token match
	*/
	static const ValueMention* getValueMentionFromTokenOffsets(const SentenceTheory* sentTheory, int start_token, int end_token);
	/**
	 * Retrieve LocatedString substring by using edt offsets.
	 *
	 * @param begin_edt_index the begin edt index of the substring, inclusive.
	 * @param end_edt_index the end edt index of the substring, inclusive.
	 * @return a new LocatedString representing a substring of this one.  User is responsible for deleting the new LocatedString after use.
	 */
	static class LocatedString* substringFromEdtOffsets(const class LocatedString* origStr, const EDTOffset begin_edt_index, EDTOffset end_edt_index);
};

