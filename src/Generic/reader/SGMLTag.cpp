// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/reader/SGMLTag.h"
#include "Generic/common/SessionLogger.h"
#include <boost/shared_ptr.hpp>

SGMLTag::SGMLTag() : _source(0), _start(0), _end(0), _close(false), _tagAttributes() {
	_name = SymbolConstants::nullSymbol;
	_attributes.type = SymbolConstants::nullSymbol;
	_attributes.coref_id = NO_ID;
	_open = true;
}

SGMLTag::SGMLTag(const SGMLTag& other) {
	_source = other._source;
	_name = other._name;
	_attributes.type = other._attributes.type;
	_attributes.coref_id = other._attributes.coref_id;
	_start = other._start;
	_end = other._end;
	_close = other._close;
	_open = other._open;
	_attributes.docID = other._attributes.docID;
    _tagAttributes = other._tagAttributes;
}

SGMLTag::SGMLTag(const LocatedString& source, Symbol name, int start, int end, bool close) {
	_source = &source;
	_name = name;
	_attributes.type = SymbolConstants::nullSymbol;
	_attributes.coref_id = NO_ID;
	_start = start;
	_end = end;
	_close = close;
	_open = ! close;
	computeTagAttributes();
}


SGMLTag::SGMLTag(const LocatedString& source, Symbol name, Symbol type, 
				 int start, int end, bool close)
{
	_source = &source;
	_name = name;
	_attributes.type = type;
	_attributes.coref_id = NO_ID;
	_start = start;
	_end = end;
	_close = close;
	_open = ! close;
	computeTagAttributes();
}


SGMLTag::SGMLTag(const LocatedString& source, Symbol name, int id, 
				 int start, int end, bool close)
{
	_source = &source;
	_name = name;
	_attributes.type = SymbolConstants::nullSymbol;
	_attributes.coref_id = id;
	_start = start;
	_end = end;
	_close = close;
	_open = ! close;
	computeTagAttributes();
}

SGMLTag::SGMLTag(const LocatedString& source) {
	_source = &source;
	_name = SymbolConstants::nullSymbol;
	_attributes.type = SymbolConstants::nullSymbol;
	_attributes.coref_id = NO_ID;
	_start = _source->length();
	_end = _source->length();
	_close = false;
	_open = true;
	computeTagAttributes();
}

SGMLTag::SGMLTag(const LocatedString& source, Symbol name, Symbol type, 
				 int id, int start, int end, bool close)
{
	_source = &source;
	_name = name;
	_attributes.type = type;
	_attributes.coref_id = id;
	_start = start;
	_end = end;
	_close = close;
	_open = ! close;
	computeTagAttributes();
}

Symbol SGMLTag::toSymbol() {
	LocatedString *sub = _source->substring(_start, _end);
	Symbol result = sub->toSymbol();
	delete sub;
	return result;
}

bool SGMLTag::notFound() { 
	return _start >= _source->length(); 
}


bool SGMLTag::foundWithinBounds(int pos) const {
    return pos !=-1 && pos < _end;
}

Symbol SGMLTag::getAttributeValue(const wchar_t* attr) const {
	return getAttributeValue(std::wstring(attr));
}


Symbol SGMLTag::getAttributeValue(const std::wstring& attr) const {

    if (_tagAttributes.find(attr)!=_tagAttributes.end()) {
        return _tagAttributes.find(attr)->second->toSymbol();
    } else {
        return Symbol();
    }
}


const AttributeMap& SGMLTag::getTagAttributes() const {
    return _tagAttributes;
}

// see for example, XML standard section 2.3
// http://www.w3.org/TR/REC-xml/#NT-S
bool SGMLTag::isSGMLTagWhiteSpace(wchar_t c) const {
    return c == L' ' || c == L'\t'
        || c == L'\n' || c == L'\r';
}

// method called by constructor to compute the mappping of SGML attributes to their values
// should not be called elsewhere
void SGMLTag::computeTagAttributes() {
    // recall that start is the position of the opening <
    // and end is the position of the closing >
    int start=_start+1;

	while (start < _end) {
	    // locate attributes by searching for equals signs
		const int eqLoc = _source->indexOf(L"=", start);
		if (!foundWithinBounds(eqLoc)) {
		    // no more equals signs means no more attributes
		    return;
		}

        // search backwards from the equals sign to find the attribute name
		int searchForAttrPos = eqLoc - 1;
		bool foundAttribute=false;
		for (; searchForAttrPos > start; --searchForAttrPos) {
		    wchar_t probeChar = _source->charAt(searchForAttrPos);

			if(!isSGMLTagWhiteSpace(probeChar)){
			    // there can be any amount of whitespace between the attribute
			    // name and the equals sign. When we've found non-whitespace
			    // when going backwards, then we have found the end of the attribute
			    // name
				foundAttribute=true;
			} else if (foundAttribute) {
			    // if we are within the attribute name going backwards and we
			    // encounter another space, then we are at the beginning of the
			    // attribute name
			    break;
		    }
		}

	    if (foundAttribute) {
    	    const int attributeStart = searchForAttrPos + 1;

            // end of substring is exclusive, beginning is inclusive
			//BUG:: should it be eqLoc-1??
            LocatedString* attributeNameLS = _source->substring(attributeStart,eqLoc);
            attributeNameLS->trim();
            std::wstring attributeName = attributeNameLS->toWString();
            delete attributeNameLS;

	        start = locateAndSetAttributeValue(attributeName, eqLoc);
        } else {
            // if we didn't find an attribute name for an equals sign, then
        	// our tag is somehow corrupted
			// SessionLogger TODO: I can't seem to get this one to fire
            SessionLogger::warn_user("sgml_tag_problem") << "Invalid attribute while parsing SGML tag "
				<< _source->substring(_start, _end)->toString() << "\n";
            start = eqLoc + 1;
        }
	}
}

// This is a helper method to computeTagAttributes and should not otherwise
// be called.
// Given an attribute name and the position of the attribute's equals sign,
// will attempt to find the attribute value. If it does, it will store
// the name-->value mapping in _tagAttributes.  In any case, it will
// return the position the next attribute search should begin from
int SGMLTag::locateAndSetAttributeValue( std::wstring& attributeName, int eqLoc) {
    // we handle several situations when trying to find the attribute value
    // (a) standard SGML attributes are surrounded by " or ' and may be
    //        separated from the attribute name by any number of spaces
    //       <foo attr="bar" attr2='meep' />
    // (b) if there is a span of non-whitespace immediately following
    //        the attribute name but without quotes, we will use explicit
    //        as the value.
    //        <foo attr=bar attr2=meep />
    // (c) if there is a space following = and the first non-whitespace
    //      character after = is not a quote (or not present at all),
    //      we return en empty string
    wchar_t delimiter = L' ';
    int valueStartPos = -1;

    for (int pos = eqLoc +1; pos < _end; ++pos) {
        wchar_t charAtPos = _source->charAt(pos);

        if (charAtPos == L'\"') {
            delimiter = L'"';
            valueStartPos = pos+1;
            break;
        } else if (charAtPos == L'\'') {
            delimiter = L'\'';
            valueStartPos = pos+1;
            break;
        } else if (isSGMLTagWhiteSpace(charAtPos)) {
            // continue - we can have any number of spaces
            // before the attribute value
        } else {
            // we encountered a non-space character before
            // encountering any quote

            // are we immediately adjacent to the =?
            if (pos == eqLoc + 1) {
                // if so, use the attribute value
                delimiter = L' ';
                // not pos + 1 because we want to include
                // the character we just found
                valueStartPos = pos;
                break;
            } else {
                // we do not parse an attribute value in this case
                // so we break leaving valueStartPos at -1
                break;
            }
        }
    }

    if (foundWithinBounds(valueStartPos)) {
        int closingDelimiterPos;
        if (L' ' == delimiter) {
            closingDelimiterPos = findNextSGMLWhiteSpace(valueStartPos+1);
         } else {
			std::wstring str(1, delimiter);
			closingDelimiterPos = _source->indexOf( str.c_str(),valueStartPos+1);
        }
        if (!foundWithinBounds(closingDelimiterPos)) {
            // if we don't find a closing delimiter, we use
            // the rest of the tag contents as the value
            // RMG: there is a small bug here - if we have a tag like
            // <foo attr="meep />.
            // We probably don't want to include the closing /.
            // But we are already handling malformed input here, so
            // we won't complicate things to deal with it at the moment.
            closingDelimiterPos = _end-1;
        }

		LSPtr attributeValue;
		attributeValue.reset(_source->substring(valueStartPos, closingDelimiterPos));
        _tagAttributes.insert(std::make_pair(attributeName, attributeValue));
        return closingDelimiterPos + 1;
    } else {
        return eqLoc + 1;
    }
}

int SGMLTag:: findNextSGMLWhiteSpace(int pos){
	for(int i=pos;i<_end;i++){
		wchar_t charAtPos = _source->charAt(pos);
		if(isSGMLTagWhiteSpace(charAtPos)){
			return i;
		}
	}
	return -1;

}
SGMLTag SGMLTag::findNextSGMLTag(const LocatedString& input, int start) {
	const int len = input.length();
	int end_tag = 0;
	bool close = false;
	bool openCloseTag = false;

	// Find the next open tag.
	int start_tag = start;
	while ((start_tag < len) && (input.charAtNoBoundsCheck(start_tag) != L'<')) {
		start_tag++;
	}

	// If there wasn't one, return a null tag.
	if (start_tag >= len) {
		return SGMLTag::notFound(input);
	}

	// Find the beginning of the tag name.
	int start_name = start_tag + 1;
	int slashes = 0;
	while ((start_name < len) && 
		   (iswspace(input.charAtNoBoundsCheck(start_name)) || 
		   (input.charAtNoBoundsCheck(start_name) == L'/'))) {
		if (input.charAtNoBoundsCheck(start_name) == L'/') { 	// Is it a close tag?
			close = true;
			if (++slashes > 1) {
				throw UnexpectedInputException("SGMLTag::findNextSGMLTag()",
						"multiple slashes found in tag name");
			}
		}
		start_name++;
	}

	// If there wasn't a name, return a null tag.
	if (start_name >= len) {
		return SGMLTag::notFound(input);
	}

	if (!input.isValidTagName(start_name)) {
		//Not a valid tag name (nested bracket, number, something else)
		//This isn't really a tag then.
		//find the next tag after this one
		return findNextSGMLTag(input, start_name+1);
	}

	// Find the end of the tag name.
	int end_name = start_name;
	while ((end_name < len) &&
		   !iswspace(input.charAtNoBoundsCheck(end_name)) &&
		   (input.charAtNoBoundsCheck(end_name) != L'>') &&
		   (input.charAtNoBoundsCheck(end_name) != L'<')) {
		end_name++;
	}

	// Get the tag name.
    LocatedString *nameString = input.substring(start_name, end_name);
	nameString->toUpperCase();
	Symbol name = nameString->toSymbol();
	delete nameString;

	// To handle embedded a pseudo-SGML tag (which is tolerated
	// but clearly incorrect SGML), check out the embedded tag.
	// but allow only an immediate close of the same tag name that is open....
	if (input.charAtNoBoundsCheck(end_name) == L'<') {
		if (mustBeCloseTag(input, name, end_name)) {
			// we found the end of this whole tag early
			end_tag = end_name; // this is an overlapping ficticious end
		}else{
			//nested open bracket after name in tag.
			//This isn't really a tag then.
			//find the next tag after this one
			return findNextSGMLTag(input, end_name+1);
		}
	}

	// Find the end of the tag (unless already found)
	if (end_tag == 0){
		end_tag = end_name;
		while ((end_tag < len) && (input.charAtNoBoundsCheck(end_tag) != L'>')) {
			// To handle embedded a pseudo-SGML tag (which of course
			// is not correct SGML), recurse on the embedded tag.
			if (input.charAt(end_tag) == L'<') {
				if (mustBeCloseTag(input, name, end_tag)) {
					// we found the end of this whole tag early
					end_tag = end_tag -1; // this is an overlapping ficticious end
					break;
				}else{
					//nested open bracket after name in tag.
					//This isn't really a tag then.
					//find the next tag after this one
					return findNextSGMLTag(input, end_tag+1);
				}
			}
			end_tag++;
		}
	}

	// If there was no closing bracket, this wasn't really a tag.
	if (end_tag >= len) {
		return SGMLTag::notFound(input);
	}

	if (!close && (input.charAt(end_tag -1 ) == L'/') ){
		openCloseTag = true;
	}

	// Search for a TYPE="..." clause.
	int end_type = end_name;
	LocatedString *typeAttribute = getTagType(input, end_name, end_type, end_tag);

	// Search for a COREFID="..." clause.
	LocatedString *idAttribute = getTagCorefID(input, end_type, end_tag);

	// Search for a ID="..." clause.
	LocatedString *id_str = getTagDocID(input, end_type, end_type, end_tag);

	// Put end_tag at one beyond the end.
	end_tag++;





	int coref_id = SGMLTag::NO_ID;
	if (idAttribute != NULL) {
		coref_id = _wtoi(idAttribute->toString());
		delete idAttribute;
	}

	SGMLTag tag;
	if (typeAttribute == NULL) {
		tag = SGMLTag(input, name, coref_id, start_tag, end_tag, close);
	}
	else {
		Symbol type = typeAttribute->toSymbol();
		delete typeAttribute;
		tag = SGMLTag(input, name, type, coref_id, start_tag, end_tag, close);
	}
	if (openCloseTag){
		tag._open = true;
		tag._close = true;
	}
	if(id_str != NULL){
		tag.setDocumentID(id_str->toSymbol());
		delete id_str;
	}
	return tag;
}

SGMLTag SGMLTag::findOpenTag(const LocatedString& input, Symbol name, int start) {
	SGMLTag tag;
	do {
		tag = findNextSGMLTag(input, start);
		start = tag.getEnd();
	} while (!tag.notFound() && ((tag.getName() != name) || !(tag.isOpenTag())));
	return tag;
}

SGMLTag SGMLTag::findCloseTag(const LocatedString& input, Symbol name, int start) {
	SGMLTag tag;
	int stack = 1;
	do {
		tag = findNextSGMLTag(input, start);
		start = tag.getEnd();
		if (tag.getName() == name) {
			if (tag.isOpenTag())
				stack++;
			else
				stack--;
		}
	} while (!tag.notFound() && (stack != 0));
	return tag;
}
bool SGMLTag::mustBeCloseTag(const LocatedString& input, Symbol name, int start) {
	SGMLTag tag;
	tag = findNextSGMLTag(input, start);
	return (!tag.notFound() && (start == tag.getStart()) &&
			(tag.getName() == name) && (tag.isCloseTag()));
}

void SGMLTag::removeSGMLTags(LocatedString& source) {
	SGMLTag tag;
	int offset = 0;
	do {
		tag = findNextSGMLTag(source, offset);
		if (!tag.notFound()) {
			source.remove(tag.getStart(), tag.getEnd());
			offset = tag.getStart();
		}
	} while (!tag.notFound());
}

/**
 * @param source the input text.
 * @param start the position just after the tag name.
 * @param max the position of the closing angle bracket.
 */
LocatedString* SGMLTag::getTagType(const LocatedString& source, int start, int& end, int max) {
	// Skip past whitespace.
	while ((start < max) && iswspace(source.charAt(start))) {
		start++;
	}

	// Must be at least long enough for "TYPE=X".
	if ((start + 6) >= max) {
		return NULL;
	}

	// Match the word TYPE.
	if ((towupper(source.charAt(start)) == L'T') &&
		(towupper(source.charAt(start + 1)) == L'Y') &&
		(towupper(source.charAt(start + 2)) == L'P') &&
		(towupper(source.charAt(start + 3)) == L'E') &&
		(!iswalnum(source.charAt(start + 4))))
	{
		// Skip past "TYPE".
		start += 4;

		// Skip past whitespace.
		while ((start < max) && iswspace(source.charAt(start))) {
			start++;
		}

		// Skip past the '=' sign.
		if ((start >= max) || (source.charAt(start) != L'=')) {
			return NULL;
		}
		start++;

		// Skip past whitespace.
		while ((start < max) && iswspace(source.charAt(start))) {
			start++;
		}
		if (start >= max) {
			return NULL;
		}

		// Read the attribute value.
		wchar_t first_char = source.charAt(start);
		if ((first_char == L'\'') || (first_char == L'"')) {
			// Don't include the quote character in the type string.
			start++;

			// Search for the closing quote character.
			end = start;
			while ((end < max) && (source.charAt(end) != first_char)) {
				end++;
			}

			// If there wasn't a closing quote character before the
			// closing angle bracket, throw an exception.
			if (end >= max) {
				throw UnexpectedInputException(
					"SGMLTag::getTagType()",
					"unterminated TYPE attribute");
			}

			// HACK: need to get rid of the extra coref info in Arabic (ex: GPE:CITY:3)
			int temp_end = end - 1;
			while (iswdigit(source.charAt(temp_end))) {
				temp_end--;
			}
			if (towupper(source.charAt(temp_end)) == L':') 
			{ 
				end = temp_end;
			}
				
			// Return the substring between the quote characters.
			return source.substring(start, end);
		}
		else {
			end = start;

			// HACK: this allows all kinds of punctuation that it shouldn't

			// Return the word terminated by whitespace or the closing angle bracket.
			while ((end < max) && !iswspace(source.charAt(end))) {
				end++;
			}

			// HACK: need to get rid of the extra coref info in Arabic (ex: GPE:CITY:3)
			int temp_end = end - 1;
			while (iswdigit(source.charAt(temp_end))) {
				temp_end--;
			}
			if (towupper(source.charAt(temp_end)) == L':') 
			{ 
				end = temp_end;
			}
			return source.substring(start, end);
		}
	}
	else {
		return NULL;
	}
}

/**
 * @param source the input text.
 * @param start the position just after the tag name.
 * @param max the position of the closing angle bracket.
 */
LocatedString* SGMLTag::getTagDocID(const LocatedString& source, int start, int& end, int max) {
	// Skip past whitespace.
	while ((start < max) && iswspace(source.charAt(start))) {
		start++;
	}

	// Must be at least long enough for "TYPE=X".
	if ((start + 4) >= max) {
		return NULL;
	}

	// Match the word TYPE.
	if ((towupper(source.charAt(start)) == L'I') &&
		(towupper(source.charAt(start + 1)) == L'D') &&
		(!iswalnum(source.charAt(start + 2))))
	{
		// Skip past "TYPE".
		start += 2;

		// Skip past whitespace.
		while ((start < max) && iswspace(source.charAt(start))) {
			start++;
		}

		// Skip past the '=' sign.
		if ((start >= max) || (source.charAt(start) != L'=')) {
			return NULL;
		}
		start++;

		// Skip past whitespace.
		while ((start < max) && iswspace(source.charAt(start))) {
			start++;
		}
		if (start >= max) {
			return NULL;
		}

		// Read the attribute value.
		wchar_t first_char = source.charAt(start);
		if ((first_char == L'\'') || (first_char == L'"')) {
			// Don't include the quote character in the type string.
			start++;

			// Search for the closing quote character.
			end = start;
			while ((end < max) && (source.charAt(end) != first_char)) {
				end++;
			}

			// If there wasn't a closing quote character before the
			// closing angle bracket, throw an exception.
			if (end >= max) {
				throw UnexpectedInputException(
					"SGMLTag::getTagDocID()",
					"unterminated ID attribute");
			}

			// Return the substring between the quote characters.
			return source.substring(start, end);
		}
		else {
			end = start;

			// HACK: this allows all kinds of punctuation that it shouldn't

			// Return the word terminated by whitespace or the closing angle bracket.
			while ((end < max) && !iswspace(source.charAt(end))) {
				end++;
			}

			return source.substring(start, end);
		}
	}
	else {
		return NULL;
	}
}

/**
 * @param source the input text.
 * @param start the position just after the tag name.
 * @param max the position of the closing angle bracket.
 */
LocatedString* SGMLTag::getTagCorefID(const LocatedString& source, int start, int max) {

	// check for Arabic style coref (GPE:CITY:3)
	if (source.charAt(start) == L':') {
		start++;
		int end = start;
		while (end < max && iswdigit(source.charAt(end)))
			end++;
		// Return the substring between the quote characters.
		return source.substring(start, end);
	} 
	// check for Chinese style coref ("GPE:CITY" COREFID=3)
	else {
		start++;
		// Skip past whitespace.
		while ((start < max) && iswspace(source.charAt(start))) {
			start++;
		}

		// Must be at least long enough for "COREFID=X".
		if ((start + 8) >= max) {
			return NULL;
		}

		// Match the word COREFID.
		if ((towupper(source.charAt(start)) == L'C') &&
			(towupper(source.charAt(start + 1)) == L'O') &&
			(towupper(source.charAt(start + 2)) == L'R') &&
			(towupper(source.charAt(start + 3)) == L'E') &&
			(towupper(source.charAt(start + 4)) == L'F') &&
			(towupper(source.charAt(start + 5)) == L'I') &&
			(towupper(source.charAt(start + 6)) == L'D') &&
			(!iswalnum(source.charAt(start + 7))))
		{
			// Skip past "COREFID".
			start += 7;

			// Skip past whitespace.
			while ((start < max) && iswspace(source.charAt(start))) {
				start++;
			}

			// Skip past the '=' sign.
			if ((start >= max) || (source.charAt(start) != L'=')) {
				return NULL;
			}
			start++;

			// Skip past whitespace.
			while ((start < max) && iswspace(source.charAt(start))) {
				start++;
			}
			if (start >= max) {
				return NULL;
			}

			// Read the attribute value.
			wchar_t first_char = source.charAt(start);
			if ((first_char == L'\'') || (first_char == L'"')) {
				// Don't include the quote character in the type string.
				start++;

				// Search for the closing quote character.
				int end = start;
				while ((end < max) && (source.charAt(end) != first_char)) {
					if (!iswdigit(source.charAt(end)))	
					{
						throw UnexpectedInputException("SGMLTag::getTagCorefID()",
														"non-numeric COREFID attribute");
					}
					end++;
				}

				// If there wasn't a closing quote character before the
				// closing angle bracket, throw an exception.
				if (end >= max) {
					throw UnexpectedInputException(
						"SGMLTag::getTagCorefID()",
						"unterminated COREFID attribute");
				}

				// Return the substring between the quote characters.
				return source.substring(start, end);
			}
			else {
				int end = start;

				// HACK: this allows all kinds of punctuation that it shouldn't

				// Return the word terminated by whitespace or the closing angle bracket.
				while ((end < max) && !iswspace(source.charAt(end))) {
					if (!iswdigit(source.charAt(end)))	
					{
						throw UnexpectedInputException("SGMLTag::getTagCorefID()",
														"non-numeric COREFID attribute");
					}
					end++;
				}
				return source.substring(start, end);
			}
		}
		else {
			return NULL;
		}
	}
}

int SGMLTag::isParsableSGML(const LocatedString& source) {
	Symbol none[1] = {};
	return isParsableSGML(source, none, 0);
}

int SGMLTag::isParsableSGML(const LocatedString& source, Symbol relTags[], int n_relTags){
	SGMLTag stack[100];
	int stackLim = 100;
	int top = 0;
	int max = 0;
	const int len = source.length();
	int index = 0;
	SGMLTag tag;

	std::string standardErrorMessageStart = "The system cannot parse the SGML tags in this document:";
	std::string standardErrorMessageEnd = " Please check the document for structural errors.";

	while (index < len){
		//std::cerr<<"Debug isParsable-sgml at idx " << index <<" looking for a tag\n";	
		//std::cerr.flush();
		tag = SGMLTag::findNextSGMLTag(source, index);
		if (tag.notFound()){
			if (top == 0) {
				//std::cerr<<"Debug isParsable-sgml at idx " << index <<" finished doc with empty stack and max "<< max << "\n";	
				//std::cerr.flush();
				return max;
			} if (top == 1) {
				std::stringstream errMsg;
				errMsg << standardErrorMessageStart << " there is an unclosed <" << stack[0].getName() << "> tag. " << standardErrorMessageEnd;
				throw UnexpectedInputException("SGMLTag::isParsableSGML", errMsg.str().c_str());
			} else {
				std::stringstream errMsg;
				errMsg << standardErrorMessageStart << " there are unclosed tags:";
				for (int i = 0; i < top; i++) {
					errMsg << " <" << stack[i].getName() << ">";
				}
				errMsg << ". " << standardErrorMessageEnd;
				throw UnexpectedInputException("SGMLTag::isParsableSGML", errMsg.str().c_str());
			}
		}
		index = tag.getEnd(); 
		bool relevant = true;
		if (n_relTags > 0){
			relevant = false;
			for (int ir=0; ir<n_relTags; ir++){
				if (tag.getName() == relTags[ir]){
					relevant = true;
					break;
				}
			}
		}
		if (!relevant) continue;

		if (tag.isOpenTag()){
			// push it onto stack
			if (top < stackLim) {
				stack[top++] = tag;
				//std::cerr<<"Debug isParsable-sgml at idx " << index <<" pushing "<< tag.getName() << " at " << top-1 << " with max "<<  max << "\n";
				//std::cerr.flush();
				if (top > max) max = top;
			}else{
				std::stringstream errMsg;
				errMsg << standardErrorMessageStart << " there are too many nested SGML tags (stack limit = " << stackLim << "). Please remove some of the unnecessary markup before processing.";
				throw UnexpectedInputException("SGMLTag::isParsableSGML", errMsg.str().c_str());
			}
		}
		if (tag.isCloseTag()){ // there are tags that are both open and close of same name...
			// a close must match the name we pop off the top of the stack
			if (top < 1) {

				std::stringstream errMsg;
				errMsg << standardErrorMessageStart << " there was at least one </" << tag.getName() << "> tag found without a preceding <" << tag.getName() << "> tag. " << standardErrorMessageEnd;
				throw UnexpectedInputException("SGMLTag::isParsableSGML", errMsg.str().c_str());
			}else{
				if (stack[--top].getName() != tag.getName()) {
					// Can't seem to get this to fire, but I'm going to leave it here anyway.
					std::stringstream errMsg;
					errMsg << standardErrorMessageStart << " there was at least one open tag <" << stack[top].getName() << "> that failed to match the closing tag </" << tag.getName() << ">. " << standardErrorMessageEnd;
					throw UnexpectedInputException("SGMLTag::isParsableSGML", errMsg.str().c_str());
				}else{
					//std::cerr<<"Debug isParsable-sgml at idx " << index <<" popped "<<tag.getName() << " at "<< top-1 << " with max " << max << "\n";	
					//std::cerr.flush();
				}
			}
		}
	}
	return max;
}
