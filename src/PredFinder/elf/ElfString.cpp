/**
 * Parallel implementation of ElfString object based on Python
 * implementation in ELF.py. Uses Xerces-C++ for XML writing.
 *
 * ElfStrings are typically not instantiated directly, but rather
 * by one of the subclass constructors that hardcodes a
 * particular ElfString::Type.
 *
 * @file ElfString.cpp
 * @author nward@bbn.com
 * @date 2010.06.16
 **/

#include "Generic/common/leak_detection.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "LearnIt/MainUtilities.h"
#include "PredFinder/common/SXMLUtil.h"
#include "PredFinder/macros/ReadingMacroSet.h"
#include "ElfString.h"
#include "ElfName.h"
#include "ElfDescriptor.h"
#include "ElfType.h"
#include "boost/lexical_cast.hpp"
#include "boost/make_shared.hpp"
#pragma warning(push, 0)
#include "boost/regex.hpp"
#pragma warning(pop)
#include "boost/algorithm/string/replace.hpp"

XERCES_CPP_NAMESPACE_USE

/**
 * Store a string and optional offsets.
 *
 * @param string An arbitrary string, with possible
 * restrictions based on the string type.
 * @param type The type of the contents of this string.
 * @param start Defaults to -1. Optional, but if end is
 * specified, it must be as well.
 * @param end Defaults to -1. Optional, but if start is
 * specified, it must be as well.
 *
 * @author nward@bbn.com
 * @date 2010.06.02
 **/
ElfString::ElfString(const std::wstring & value, const ElfString::Type & type, double confidence, 
					 const std::wstring & string, EDTOffset start, EDTOffset end) {
	// Check that the offsets are valid
	if ((!start.is_defined() && end.is_defined()) ||
		(start.is_defined() && !end.is_defined())) {
			throw UnexpectedInputException("ElfString::ElfString", "Start and end offsets must both be specified, or neither");
	}

	// Store the value, string, confidence, type, and offsets
	_value = value;
	_type = type;
	_confidence = confidence;
	_string = string;
	_start = start;
	_end = end;
}

/**
 * Copy constructor.
 *
 * @param other ElfString to deep copy from.
 *
 * @author nward@bbn.com
 * @date 2010.08.18
 **/
ElfString::ElfString(const ElfString_ptr other) {
	// Copy the string's value, type, string, confidence, and offsets
	_value = other->_value;
	_type = other->_type;
	_confidence = other->_confidence;
	_string = other->_string;
	_start = other->_start;
	_end = other->_end;
}

/**
 * Converts this string to an XML element (as determined by _type)
 * using the Xerces-C++ library.
 *
 * XMLPlatformUtils::Initialize() must be called before this method.
 *
 * @param doc An already-instantiated Xerces DOMDocument that
 * provides namespace context for the created element (since
 * Xerces doesn't support easy anonymous element import).
 * @return The constructed DOMElement
 *
 * @author nward@bbn.com
 * @date 2010.06.02
 **/
DOMElement* ElfString::to_xml(DOMDocument* doc) const {
	// Create a new element based on the type and store this string's value
	// as an attribute of the same name
	DOMElement* string;
	switch (_type) {
	case TYPE:
		string = SXMLUtil::createElement(doc, "type");
		SXMLUtil::setAttributeFromStdWString(string, "type", _value);
		break;
	case NAME:
		string = SXMLUtil::createElement(doc, "name");
		SXMLUtil::setAttributeFromStdWString(string, "name", _value);
		break;
	case DESC:
		string = SXMLUtil::createElement(doc, "desc");
		SXMLUtil::setAttributeFromStdWString(string, "desc", _value);
		break;
	}

	// If present, insert the offsets as attributes of the element
	if (_start.is_defined() && _end.is_defined()) {
		SXMLUtil::setAttributeFromEDTOffset(string, "start", _start);
		SXMLUtil::setAttributeFromEDTOffset(string, "end", _end);
	}

	// Optionally print the text evidence for this string
	if (ParamReader::isParamTrue("elf_include_text_excerpts")) {
        SerifXML::xstring x_str = SerifXML::transcodeToXString(_string.c_str());
		DOMText* string_cdata = doc->createTextNode(x_str.c_str());
		string->appendChild(string_cdata);
	}

	// Done
	return string;
}

/**
 * Reads the appropriate subclass of ElfString from the specified
 * <name>, <desc>, or <type> element.
 *
 * XMLPlatformUtils::Initialize() must be called before this method.
 *
 * @param element The DOMElement containing the <name>, <desc>, or <type>.
 *
 * @author nward@bbn.com
 * @date 2010.08.31
 **/
ElfString_ptr ElfString::from_xml(const DOMElement* element, bool exclusive_end_offsets) {
	// Determine the tag name (determines the attribute we load into _value)
	std::string tag = SerifXML::transcodeToStdString(element->getTagName());

	// Read the required attributes
	std::wstring value = SXMLUtil::getAttributeAsStdWString(element, tag.c_str());
	if (value == L"")
		throw std::runtime_error("ElfString::from_xml(DOMElement*): No value attribute specified");

	// Read the optional attributes
	std::wstring start_string = SXMLUtil::getAttributeAsStdWString(element, "start");
	EDTOffset start;
	if (start_string != L"")
		start = EDTOffset(boost::lexical_cast<int>(start_string));
	std::wstring end_string = SXMLUtil::getAttributeAsStdWString(element, "end");
	EDTOffset end;
	if (end_string != L"")
		end = EDTOffset(boost::lexical_cast<int>(end_string));

	// Adjust the offsets if necessary
	if (exclusive_end_offsets && end.is_defined())
		--end;

	// Get the element's text content, if any
	std::wstring text = SerifXML::transcodeToStdWString(element->getTextContent());

	// Check the validity of the offsets and text
	if (start.is_defined() != end.is_defined()) {
		throw std::runtime_error("ElfString::from_xml(DOMElement*): Invalid offset attributes, specify both or neither");
	} else if ((start.is_defined() && end.is_defined() && text == L"") || (!start.is_defined() && !end.is_defined() && text != L"")) {
		throw std::runtime_error("ElfString::from_xml(DOMElement*): Must specify both offset attributes and text content or neither");
	}

	// Check the child element tag name
	if (tag == "name")
		return boost::make_shared<ElfName>(value, -1.0, text, start, end);
	else if (tag == "desc")
		return boost::make_shared<ElfDescriptor>(value, -1.0, text, start, end);
	else if (tag == "type") {
		// Temporary hack to handle bad type namespace from UW
		boost::replace_first(value, L"eru:", ReadingMacroSet::domain_prefix + L":");
		return boost::make_shared<ElfType>(value, text, start, end);
	} else
		throw std::runtime_error("ElfString::from_xml(DOMElement*): Invalid child element, expected <name>/<desc>/<type>");
}

bool ElfString::are_coreferent(const DocTheory* doc_theory, const ElfString_ptr left_string, 
							   const ElfString_ptr right_string) {
	// Make sure these strings have offsets
	if (!left_string->_start.is_defined() || !left_string->_end.is_defined() || !right_string->_start.is_defined() || !right_string->_end.is_defined())
		// One or both strings lacks offsets, no point
		return false;

	// Check if these string's character offsets align with Serif tokens
	int left_sent_no, left_start_token, left_end_token;
	bool left_token_align = MainUtilities::getSerifStartEndTokenFromCharacterOffsets(doc_theory, left_string->_start, left_string->_end, left_sent_no, left_start_token, left_end_token);
	int right_sent_no, right_start_token, right_end_token;
	bool right_token_align = MainUtilities::getSerifStartEndTokenFromCharacterOffsets(doc_theory, right_string->_start, right_string->_end, right_sent_no, right_start_token, right_end_token);
	if (!left_token_align || !right_token_align)
		// No token alignment, give up
		return false;

	// Get the sentence theories containing these strings
	//   Give up if for some reason (bad sentence number?) we can't get the sentence theories
	SentenceTheory* left_sent_theory = doc_theory->getSentenceTheory(left_sent_no);
	if (left_sent_theory == NULL)
		return false;
	SentenceTheory* right_sent_theory = doc_theory->getSentenceTheory(right_sent_no);
	if (right_sent_theory == NULL)
		return false;

	// First, try a mention alignment for these strings
	const Mention* left_mention = MainUtilities::getMentionFromTokenOffsets(left_sent_theory, left_start_token, left_end_token);
	const Mention* right_mention = MainUtilities::getMentionFromTokenOffsets(right_sent_theory, right_start_token, right_end_token);
	if (left_mention != NULL && right_mention != NULL) {
		// If they're the exact same mention, we win
		if (left_mention == right_mention)
			return true;

		// Different mentions, try an entity alignment
		Entity* left_entity = left_mention->getEntity(doc_theory);
		Entity* right_entity = right_mention->getEntity(doc_theory);
		if (left_entity != NULL && right_entity != NULL) {
			// If they're the same entity (i.e. the mentions were coreferent), we win
			if (left_entity == right_entity)
				return true;
		}
	}

	// Second, try a value mention alignment for these strings
	const ValueMention* left_value_mention = MainUtilities::getValueMentionFromTokenOffsets(left_sent_theory, left_start_token, left_end_token);
	const ValueMention* right_value_mention = MainUtilities::getValueMentionFromTokenOffsets(right_sent_theory, right_start_token, right_end_token);
	if (left_value_mention != NULL && right_value_mention != NULL) {
		// If they're the exact same ValueMention, we win
		if (left_value_mention == right_value_mention)
			return true;
	}

	// Third, try a raw parse tree alignment for these strings
	const SynNode* left_parse_node = MainUtilities::getParseNodeFromTokenOffsets(left_sent_theory, left_start_token, left_end_token);
	const SynNode* right_parse_node = MainUtilities::getParseNodeFromTokenOffsets(right_sent_theory, right_start_token, right_end_token);
	if (left_parse_node != NULL && right_parse_node != NULL) {
		// If they're the exact same SynNode, we win
		if (left_parse_node == right_parse_node)
			return true;
	}

	// No match
	return false;
}

void ElfString::dump(std::ostream &out, int indent /*= 0*/) const {
	std::wstring spaces(indent, L' ');
	std::wstring spaces_plus_2(indent + 2, L' ');
	out << spaces;
	boost::wregex spaces_re(L"\\s+");
	std::wstring cleaned_string = boost::regex_replace(_string, spaces_re, L" ");
	out << "ElfString: type: " << _type;
	out << "; value: " << _value;
	out << "; confidence: " << _confidence;
	out << "; string: <" << cleaned_string;
	out << ">; start: " << _start;
	out << "; end: " << _end;
	out << std::endl;
}

std::string ElfString::toDebugString(int indent) const {
	std::ostringstream ostr;
	dump(ostr, indent);
	return ostr.str();
}
