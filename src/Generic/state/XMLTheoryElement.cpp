// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/Theory.h"
#include "Generic/theories/Token.h"

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMException.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMEntityReference.hpp>
#include <xercesc/dom/DOMComment.hpp>
#include <xercesc/dom/DOMText.hpp>
#include <xercesc/util/XMLString.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

using namespace xercesc;
namespace SerifXML {

XMLTheoryElement::XMLTheoryElement() 
: XMLElement(), _xmlSerializedDocTheory(0) {}

XMLTheoryElement::XMLTheoryElement(const XMLTheoryElement& other)
: XMLElement(other), _xmlSerializedDocTheory(other._xmlSerializedDocTheory) {} 

XMLTheoryElement& XMLTheoryElement::operator=(const XMLTheoryElement& other) {
	_xmlSerializedDocTheory = other._xmlSerializedDocTheory;
	_xercesDOMElement = other._xercesDOMElement;
	return *this;
}

// Private constructor:
XMLTheoryElement::XMLTheoryElement(XMLSerializedDocTheory *xmlDocument, DOMElement *element)
: XMLElement(element), _xmlSerializedDocTheory(xmlDocument) {}

XMLSerializedDocTheory* XMLTheoryElement::getXMLSerializedDocTheory() const {
	checkNull();
	return _xmlSerializedDocTheory; 
}

//=========================== XML Children ===============================

std::vector<XMLTheoryElement> XMLTheoryElement::getChildElements() const {
	checkNull();
	std::vector<XMLTheoryElement> elements;
	DOMNode *node = _xercesDOMElement->getFirstChild();
	while (node) {
		if (DOMElement *elt = dynamic_cast<DOMElement*>(node))
			elements.push_back(XMLTheoryElement(_xmlSerializedDocTheory, elt));
		node = node->getNextSibling();
	}
	return elements;
}

XMLTheoryElement XMLTheoryElement::getUniqueChildElement() const {
	std::vector<XMLTheoryElement> elements = getChildElements();
	if (elements.size() != 0) {
		reportLoadError("Expected exactly one child element.");
		return XMLTheoryElement();
	} else {
		return elements[0];
	}
}


XMLTheoryElement XMLTheoryElement::getUniqueChildElementByTagName(const XMLCh* tag) const {
	XMLTheoryElement result = getOptionalUniqueChildElementByTagName(tag);
	if (result.isNull()) {
		std::wstringstream err;
		err << "Missing required <" << transcodeToStdWString(tag) << "> child.";
		reportLoadError(err.str());
	}
	return result;
}

XMLTheoryElement XMLTheoryElement::getOptionalUniqueChildElementByTagName(const XMLCh* tag) const {
	XMLTheoryElement result;
	checkNull();
	DOMNode *node = _xercesDOMElement->getFirstChild();
	while (node) {
		DOMElement *elt = dynamic_cast<DOMElement*>(node);
		if (elt && (XMLString::compareIString(elt->getTagName(), tag)==0)) {
			if (result.isNull()) {
				result = XMLTheoryElement(_xmlSerializedDocTheory, elt);
			} else {
				std::wstringstream err;
				err << "Expected to find exactly one <" << transcodeToStdWString(tag) << "> child.";
				reportLoadError(err.str());
			} 
		}
		node = node->getNextSibling();
	}
	return result;
}

std::vector<XMLTheoryElement> XMLTheoryElement::getChildElementsByTagName(const XMLCh* tag, bool warn_if_other_tags_are_found) const{
	std::vector<XMLTheoryElement> elements;
	checkNull();
	DOMNode *node = _xercesDOMElement->getFirstChild();
	while (node) {
		DOMElement *elt = dynamic_cast<DOMElement*>(node);
		if (elt) {
			if (XMLString::compareIString(elt->getTagName(), tag)==0)
				elements.push_back(XMLTheoryElement(_xmlSerializedDocTheory, elt));
			else if (warn_if_other_tags_are_found) {
				std::wstringstream err;
				err << "Unexpected child tag: \"" 
                    << transcodeToStdWString(elt->getTagName()) << "\"."
                    << "  Expected to see only: \"" 
                    << transcodeToStdWString(tag) << "\".";
				reportLoadWarning(err.str());
			}
		}
		node = node->getNextSibling();
	}
	return elements;
}

XMLTheoryElement XMLTheoryElement::addChild(const XMLCh *tag) {
	checkNull();
	DOMElement *child = _xercesDOMElement->getOwnerDocument()->createElement(tag);
	_xercesDOMElement->appendChild(child);
	return XMLTheoryElement(_xmlSerializedDocTheory, child);
}

//=========================== Serialization ===============================

XMLTheoryElement XMLTheoryElement::saveChildTheory(const XMLCh *tag, const Theory *child, const Theory *context) {
	if (child == 0) 
		throw InternalInconsistencyException("XMLTheoryElement::saveChildTheory",
			"Attempt to save a NULL theory.");
	XMLTheoryElement childElem = addChild(tag);
	XMLIdMap *idMap = getIdMap();
	if (child->hasXMLId()) {
		xstring id;
		if (idMap->hasId(child)) { // Id was pre-registered.
			id = idMap->getId(child);
		} else
			id = generateChildId(child);
		childElem.setAttribute(X_id, id.c_str());
	}
	child->saveXML(childElem, context);
	return childElem;
}

xstring XMLTheoryElement::generateChildId(const Theory *child) const {
	// Get the id of this element (or its closest ancestor that defines an id), 
	// for use as a prefix of the new child object's identifier.
	const XMLCh* parentId = 0;
	if (getOptions().use_hierarchical_ids) {
		DOMElement *ancestor = _xercesDOMElement;
		while ((ancestor != 0) && (!ancestor->hasAttribute(X_id)))
			ancestor = dynamic_cast<DOMElement*>(ancestor->getParentNode());
		if (ancestor)
			parentId = ancestor->getAttribute(X_id);
	}

	return _xmlSerializedDocTheory->getIdMap()->generateId(child, parentId, getOptions().use_verbose_ids);
}

void XMLTheoryElement::saveTheoryPointer(const XMLCh* attrName, const Theory* theory) {
	XMLIdMap *idMap = getIdMap();
	setAttribute(attrName, idMap->getId(theory));
}

void XMLTheoryElement::saveTheoryPointerList(const XMLCh* attrName, const std::vector<const Theory*>& theories) {
	XMLIdMap *idMap = getIdMap();
	xstring idlist;
	BOOST_FOREACH(const Theory* theory, theories) {
		if (!idlist.empty())
			idlist += X_SPACE;
		idlist += idMap->getId(theory);
	}
	setAttribute(attrName, idlist);
}

void XMLTheoryElement::saveOffsets(OffsetGroup start_offset, OffsetGroup end_offset) {
	fillInMissingOffsets(start_offset, end_offset);
	if (getOptions().include_byte_offsets)
		saveOffsetsHelper(start_offset.byteOffset, end_offset.byteOffset);
	if (getOptions().include_char_offsets)
		saveOffsetsHelper(start_offset.charOffset, end_offset.charOffset);
	if (getOptions().include_edt_offsets)
		saveOffsetsHelper(start_offset.edtOffset, end_offset.edtOffset);
	if (getOptions().include_asr_times)
		saveOffsetsHelper(start_offset.asrTime, end_offset.asrTime);
}

namespace {
	/** Return the XML attribute tag for the given offset type */
	template<typename OffsetType> const XMLCh* getOffsetAttribTag();
	/** Return the XML element tag for the given offset type */
	template<typename OffsetType> const XMLCh* getOffsetElementTag();
	/** Return the XML attribute tag for a range of offsets with the given offset type */
	template<typename OffsetType> const XMLCh* getOffsetRangeTag();
	/** Return the XML attribute tag for a begin offset with the given offset type. */
	template<typename OffsetType> const XMLCh* getOffsetStartTag();
	/** Return the XML attribute tag for an end offset with the given offset type. */
	template<typename OffsetType> const XMLCh* getOffsetEndTag();

	// Definitions.
	template<> const XMLCh* getOffsetAttribTag<ByteOffset>() { return X_byte_offsets; }
	template<> const XMLCh* getOffsetAttribTag<CharOffset>() { return X_char_offsets; }
	template<> const XMLCh* getOffsetAttribTag<EDTOffset>() { return X_edt_offsets; }
	template<> const XMLCh* getOffsetAttribTag<ASRTime>() { return X_asr_times; }
	template<> const XMLCh* getOffsetElementTag<ByteOffset>() { return X_ByteOffsets; }
	template<> const XMLCh* getOffsetElementTag<CharOffset>() { return X_CharOffsets; }
	template<> const XMLCh* getOffsetElementTag<EDTOffset>() { return X_EDTOffsets; }
	template<> const XMLCh* getOffsetElementTag<ASRTime>() { return X_ASRTimes; }
	template<> const XMLCh* getOffsetRangeTag<ByteOffset>() { return X_ByteOffsetRange; }
	template<> const XMLCh* getOffsetRangeTag<CharOffset>() { return X_CharOffsetRange; }
	template<> const XMLCh* getOffsetRangeTag<EDTOffset>() { return X_EDTOffsetRange; }
	template<> const XMLCh* getOffsetRangeTag<ASRTime>() { return X_ASRTimeRange; }
	template<> const XMLCh* getOffsetStartTag<ByteOffset>() { return X_start_byte; }
	template<> const XMLCh* getOffsetStartTag<CharOffset>() { return X_start_char; }
	template<> const XMLCh* getOffsetStartTag<EDTOffset>() { return X_start_edt; }
	template<> const XMLCh* getOffsetStartTag<ASRTime>() { return X_start_time; }
	template<> const XMLCh* getOffsetEndTag<ByteOffset>() { return X_end_byte; }
	template<> const XMLCh* getOffsetEndTag<CharOffset>() { return X_end_char; }
	template<> const XMLCh* getOffsetEndTag<EDTOffset>() { return X_end_edt; }
	template<> const XMLCh* getOffsetEndTag<ASRTime>() { return X_end_time; }
}
template<typename OffsetType>
void XMLTheoryElement::saveOffsetsHelper(OffsetType start_offset, OffsetType end_offset) {
	if ((start_offset.is_defined() || end_offset.is_defined())) {
		if (getOptions().use_condensed_offsets) {
			const XMLCh* offsets_tag = getOffsetAttribTag<OffsetType>();
			std::wstringstream offsets;
			offsets << start_offset << L":" << end_offset;
			setAttribute(offsets_tag, offsets.str());
		} else {
			setAttribute(getOffsetStartTag<OffsetType>(), start_offset.value());
			setAttribute(getOffsetEndTag<OffsetType>(), end_offset.value());
		}
	}
}

std::wstring XMLTheoryElement::getOriginalTextSubstring(const OffsetGroup &startOffset, const OffsetGroup &endOffset) const {
	const LocatedString *originalText = getXMLSerializedDocTheory()->getOriginalText();
	if (originalText == 0)
		throw InternalInconsistencyException("XMLSerializedDocTheory::getOriginalTextSubstring",
			"original text must be defined");
	int startpos = getOriginalStringPositionOfStartOffset(startOffset);
	int endpos = getOriginalStringPositionOfEndOffset(endOffset);
	return originalText->substringAsWString(startpos, endpos+1);
}

//========================== Deserialization ==============================

const Theory* XMLTheoryElement::loadTheoryPointerHelper(const XMLCh* attrName, bool optional) const {
	const XMLCh* id = getAttribute<const XMLCh*>(attrName, X_NULL);
	// If it's not optional, then make sure it was specified (and not specified as NULL).
	if (!optional && (XMLString::compareIString(id, X_NULL)==0))
		throw reportMissingAttribute(attrName);
	return getIdMap()->getTheory(id);
}

void XMLTheoryElement::loadTheoryPointerListHelper(const XMLCh* attrName, std::vector<xstring> &idList) const {
	if (hasAttribute(attrName)) {
		xstring attrVal = getAttribute<xstring>(attrName);
		if (attrVal.size() > 0)
			boost::split(idList, attrVal, boost::is_any_of(xstring(X_WHITESPACE)));
	}
}


bool XMLTheoryElement::loadId(Theory* theory) {
	const XMLCh* id = getAttribute<const XMLCh*>(X_id, 0);
	if (id) {
		if (_xmlSerializedDocTheory->getIdMap()->hasId(id))
			reportLoadError("Id " + transcodeToStdString(id) + " is already defined");
		_xmlSerializedDocTheory->getIdMap()->registerId(id, theory);
		return true;
	}
	else
		return false;
}

template<typename OffsetType>
void XMLTheoryElement::loadOffsetsHelper(OffsetType &start_offset, OffsetType &end_offset) const {
	// Initialize to undefined.
	start_offset = OffsetType();
	end_offset = OffsetType();
	// Check for condensed attribute.
	const XMLCh* offsets_tag = getOffsetAttribTag<OffsetType>();
	if (hasAttribute(offsets_tag)) {
		std::wstring offsets = getAttribute<std::wstring>(offsets_tag);
		size_t splitpos = offsets.find(L":");
		start_offset = OffsetType::parse(offsets.substr(0, splitpos));
		end_offset = OffsetType::parse(offsets.substr(splitpos+1));
	}
	// Check for start attribute.
	const XMLCh* start_offset_tag = getOffsetStartTag<OffsetType>();
	if (hasAttribute(start_offset_tag)) {
		if (start_offset.is_defined())
			reportLoadError("Start offset multiply defined"); // ok if they're the same?
		start_offset = OffsetType::parse(getAttribute<std::wstring>(start_offset_tag));
	}
	// Check for end attribute.
	const XMLCh* end_offset_tag = getOffsetEndTag<OffsetType>();
	if (hasAttribute(end_offset_tag)) {
		if (end_offset.is_defined())
			reportLoadError("End offset multiply defined"); // ok if they're the same?
		end_offset = OffsetType::parse(getAttribute<std::wstring>(end_offset_tag));
	}
}

void XMLTheoryElement::loadOffsets(OffsetGroup &start_offsets, OffsetGroup &end_offsets) const {
	loadOffsetsHelper(start_offsets.byteOffset, end_offsets.byteOffset);
	loadOffsetsHelper(start_offsets.charOffset, end_offsets.charOffset);
	loadOffsetsHelper(start_offsets.edtOffset, end_offsets.edtOffset);
	loadOffsetsHelper(start_offsets.asrTime, end_offsets.asrTime);
	// If we have an original text, then check to make sure all offsets are within range.
	const LocatedString *originalText = getXMLSerializedDocTheory()->getOriginalText();
	if (originalText) {
		if ((start_offsets.byteOffset.is_defined() && originalText->start<ByteOffset>().is_defined()&&
			 start_offsets.byteOffset < originalText->start<ByteOffset>()) ||
			(start_offsets.charOffset.is_defined() && originalText->start<CharOffset>().is_defined()&&
			 start_offsets.charOffset < originalText->start<CharOffset>()) ||
			(start_offsets.edtOffset.is_defined() && originalText->start<EDTOffset>().is_defined()&&
			 start_offsets.edtOffset < originalText->start<EDTOffset>()) ||
			(start_offsets.asrTime.is_defined() && originalText->start<ASRTime>().is_defined()&&
			 start_offsets.asrTime < originalText->start<ASRTime>()))
			 throw reportLoadError("Start offset is less than the start offset of the original text");
		if ((end_offsets.byteOffset.is_defined() && originalText->end<ByteOffset>().is_defined()&&
			 end_offsets.byteOffset > originalText->end<ByteOffset>()) ||
			(end_offsets.charOffset.is_defined() && originalText->end<CharOffset>().is_defined()&&
			 end_offsets.charOffset > originalText->end<CharOffset>()) ||
			(end_offsets.edtOffset.is_defined() && originalText->end<EDTOffset>().is_defined()&&
			 end_offsets.edtOffset > originalText->end<EDTOffset>()) ||
			(end_offsets.asrTime.is_defined() && originalText->end<ASRTime>().is_defined()&&
			 end_offsets.asrTime > originalText->end<ASRTime>())) {
			std::stringstream ss;
			ss << "End offset (";
			if (end_offsets.byteOffset.is_defined()) { ss << end_offsets.byteOffset << "/"; } 
			else { ss << "UNDEF/"; }
			if (end_offsets.charOffset.is_defined()) { ss << end_offsets.charOffset << "/"; } 
			else { ss << "UNDEF/"; }
			if (end_offsets.edtOffset.is_defined()) { ss << end_offsets.edtOffset << "/"; } 
			else { ss << "UNDEF/"; }
			if (end_offsets.asrTime.is_defined()) { ss << end_offsets.asrTime; } 
			else { ss << "UNDEF"; }
			ss << ") is greater than the end offset of the original text (";
			if (originalText->end<ByteOffset>().is_defined()) { ss << originalText->end<ByteOffset>() << "/"; } 
			else { ss << "UNDEF/"; }
			if (originalText->end<CharOffset>().is_defined()) { ss << originalText->end<CharOffset>() << "/"; } 
			else { ss << "UNDEF/"; }
			if (originalText->end<EDTOffset>().is_defined()) { ss << originalText->end<EDTOffset>() << "/"; } 
			else { ss << "UNDEF/"; }
			if (originalText->end<ASRTime>().is_defined()) { ss << originalText->end<ASRTime>() << "/"; } 
			else { ss << "UNDEF"; }
			ss << ")";
			throw reportLoadError(ss.str());
		}
	}
	// If we have an original text to work with, then try to fill in any
	// offsets that were not explicitly specified.
	if (originalText) {
		fillInMissingOffsets(start_offsets, end_offsets);
		if (!(start_offsets.charOffset.is_defined()))
			throw reportLoadError("Unable to determine the start character offset");
		if (!(end_offsets.charOffset.is_defined()))
			throw reportLoadError("Unable to determine the end character offset");
	}
	// Ensure that the offsets all make sense.
	if ((start_offsets.byteOffset.is_defined() && end_offsets.byteOffset.is_defined() && 
	     (start_offsets.byteOffset > end_offsets.byteOffset)) ||
	    (start_offsets.charOffset.is_defined() && end_offsets.charOffset.is_defined() && 
	     (start_offsets.charOffset > end_offsets.charOffset)) ||
	    (start_offsets.edtOffset.is_defined() && end_offsets.edtOffset.is_defined() && 
	     (start_offsets.edtOffset > end_offsets.edtOffset)) ||
	    (start_offsets.asrTime.is_defined() && end_offsets.asrTime.is_defined() && 
	     (start_offsets.asrTime > end_offsets.asrTime)))
		throw reportLoadError("Start offset must be less than or equal to end offset");
}

void XMLTheoryElement::fillInMissingOffsets(OffsetGroup& start, OffsetGroup& end) const {
	//std::cerr << "fill in missing offsets currently turned off" << std::endl;
	//return; 
	const LocatedString *originalText = _xmlSerializedDocTheory->getOriginalText();
	if (!originalText)
			throw reportLoadError("Original Text not found");
	if (!(start.byteOffset.is_defined() && start.charOffset.is_defined() &&
		start.edtOffset.is_defined() && start.asrTime.is_defined())) {
		int start_pos = getOriginalStringPositionOfStartOffset(start);
		if (!start.byteOffset.is_defined())
			start.byteOffset = originalText->start<ByteOffset>(start_pos);
		if (!start.charOffset.is_defined())
			start.charOffset = originalText->start<CharOffset>(start_pos);
		if (!start.edtOffset.is_defined())
			start.edtOffset = originalText->start<EDTOffset>(start_pos);
		if (!start.asrTime.is_defined())
			start.asrTime = originalText->start<ASRTime>(start_pos);
	}

	if (!(end.byteOffset.is_defined() && end.charOffset.is_defined() &&
		end.edtOffset.is_defined() && end.asrTime.is_defined())) {
		int end_pos = getOriginalStringPositionOfEndOffset(end);
		if (!end.byteOffset.is_defined())
			end.byteOffset = originalText->end<ByteOffset>(end_pos);
		if (!end.charOffset.is_defined())
			end.charOffset = originalText->end<CharOffset>(end_pos);
		if (!end.edtOffset.is_defined())
			end.edtOffset = originalText->end<EDTOffset>(end_pos);
		if (!end.asrTime.is_defined())
			end.asrTime = originalText->end<ASRTime>(end_pos);
	}
}

int XMLTheoryElement::getOriginalStringPositionOfStartOffset(const OffsetGroup &offsets) const {
	const LocatedString *originalText = _xmlSerializedDocTheory->getOriginalText();
	if (offsets.charOffset.is_defined())
		return offsets.charOffset.value() - originalText->start<CharOffset>().value();
	else if (offsets.byteOffset.is_defined())
		return originalText->positionOfStartOffset(offsets.byteOffset);
	else if (offsets.edtOffset.is_defined())
		return originalText->positionOfStartOffset(offsets.edtOffset);
	else if (offsets.asrTime.is_defined())
		return originalText->positionOfStartOffset(offsets.asrTime);
	else
		// [XX] this method isn't just called when loading -- we might be saving.
		reportLoadError("Expected start offset information");
	return -1; // This line is never reached (reportLoadError throws an exception)
}

int XMLTheoryElement::getOriginalStringPositionOfEndOffset(const OffsetGroup &offsets) const {
	const LocatedString *originalText = _xmlSerializedDocTheory->getOriginalText();
	if (offsets.charOffset.is_defined())
		return offsets.charOffset.value() - originalText->start<CharOffset>().value();
	else if (offsets.byteOffset.is_defined())
		return originalText->positionOfEndOffset(offsets.byteOffset);
	else if (offsets.edtOffset.is_defined())
		return originalText->positionOfEndOffset(offsets.edtOffset);
	else if (offsets.asrTime.is_defined())
		return originalText->positionOfEndOffset(offsets.asrTime);
	reportLoadError("Expected start offset information");
	return -1; // This line is never reached (reportLoadError throws an exception)
}

//========================= Identifier Maps =============================

XMLIdMap *XMLTheoryElement::getIdMap() const { 
	return getXMLSerializedDocTheory()->getIdMap();
}

const Theory* XMLTheoryElement::getTheoryFromIdMap(const XMLCh* id) const {
	return getXMLSerializedDocTheory()->getIdMap()->getTheory(id);
}



//======================= Serialization Options =======================

const XMLTheoryElement::SerializationOptions& XMLTheoryElement::getOptions() const {
	checkNull();
	return _xmlSerializedDocTheory->getOptions();
}

void XMLTheoryElement::SerializationOptions::setOptionsToDefaultValues() {
	external_original_text = false;
	include_asr_times = true;
	include_byte_offsets = false;
	include_char_offsets = true;
	include_edt_offsets = true;
	include_spans_as_elements = false;
	include_spans_as_comments = false;
	include_mentions_as_comments = false;
	include_mention_transliterations = false;
	include_mention_confidences = true;
	include_name_transliterations = false;
	include_canonical_names = false;
	input_type = Symbol(L"auto");
	skip_dummy_pos_tags = true;
	use_condensed_offsets = true;
	use_hierarchical_ids = false;
	use_implicit_tokens = false;
	use_treebank_tree = true;
	use_verbose_ids = false;
}

XMLTheoryElement::SerializationOptions::SerializationOptions() { 
	// Start with the default options
	setOptionsToDefaultValues(); 

	// Check parameter file values
	setOptionsFromParamReader(); 
}

/** Option attributes:
  *
  *   - external_original_text: If the document's original text has a
  *     URL, then do not include the contents of the original text in 
  *     the serialized XML file.  The serialized file will only be usable
  *     if the original text can be read from its original location.
  *   - verbose_ids: Each XML identifier generated during serialization 
  *     includes the name of the type of object serialized.
  *   - hierarchical_ids: Each XML identifier generated during serialization
  *     includes its parent object's identifier as a prefix.
  *   - include_mentions_as_comments: Each entity, relation and event generated
  *     during serialization contains comments representing each of its mentions.
  *   - include_spans_as_comments: Each element representing a span of text 
  *     includes a comment containing the actual text string.
  *   - include_spans_as_elements: Each element representing a span of text
  *     includes the actual text string as a child element.
  *   - include_mention_transliterations: Add an attribute containing the
  *     transcribed text of each mention.
  *   - include_name_transliterations: Add an attribute containing the
  *     transcribed text of each name.
  *   - include_canonical_names: Add an attribute containing the canonical
  *     name mention (if one exists) for each entity.
  */
XMLTheoryElement::SerializationOptions::SerializationOptions(const XMLTheoryElement::SerializationOptions::OptionMap &optionMap)
{
	using namespace SerifXML;

	// Start with the default options
	setOptionsToDefaultValues();

	// Check parameter file values
	setOptionsFromParamReader();

	// Set boolean options
	setBooleanOption(optionMap, external_original_text, X_external_original_text);
	setBooleanOption(optionMap, use_verbose_ids, X_verbose_ids);
	setBooleanOption(optionMap, use_hierarchical_ids, X_hierarchical_ids);
	setBooleanOption(optionMap, use_condensed_offsets, X_condensed_offsets);
	setBooleanOption(optionMap, use_implicit_tokens, X_implicit_tokens);
	setBooleanOption(optionMap, include_mentions_as_comments, X_include_mentions_as_comments);
	setBooleanOption(optionMap, include_spans_as_comments, X_include_spans_as_comments);
	setBooleanOption(optionMap, include_spans_as_elements, X_include_spans_as_elements);
	setBooleanOption(optionMap, include_mention_transliterations, X_include_mention_transliterations);
	setBooleanOption(optionMap, include_mention_confidences, X_include_mention_confidences);
	setBooleanOption(optionMap, include_name_transliterations, X_include_name_transliterations);
	setBooleanOption(optionMap, include_canonical_names, X_include_canonical_names);

    #ifndef MT_DECODER
	if (include_mention_transliterations || include_name_transliterations) {
		throw UnexpectedInputException("XMLTheoryElement::SerializationOptions::SerializationOptions",
									   "include_*_transliterations options not supported: this version of SERIF was not linked with the MTDecoder library.");
	}
    #endif

	if (optionMap.find(X_input_type) != optionMap.end()) {
		std::wstring input_type_str = transcodeToStdWString(optionMap.find(X_input_type)->second.c_str());
		std::transform(input_type_str.begin(), input_type_str.end(), input_type_str.begin(), towlower);
		if (input_type_str == L"auto" || input_type_str == L"dtra" || input_type_str == L"text" || input_type_str == L"sgm")
			input_type = Symbol(input_type_str);
		else
			throw UnexpectedInputException("XMLTheoryElement::SerializationOptions::SerializationOptions",
				"Expected \"input_type\" attribute value to be \"auto\", \"dtra\", \"text\", or \"sgm\".");
	}

	// Decide how to encode parse trees.
	if (optionMap.find(X_parse_encoding) != optionMap.end()) {
		std::wstring parse_encoding = transcodeToStdWString(optionMap.find(X_parse_encoding)->second.c_str());
		std::transform(parse_encoding.begin(), parse_encoding.end(), parse_encoding.begin(), towlower);
		if (parse_encoding == L"treebank")
			use_treebank_tree = true;
		else if (parse_encoding == L"synnode")
			use_treebank_tree = false;
		else
			throw UnexpectedInputException("XMLTheoryElement::SerializationOptions::SerializationOptions",
				"Expected \"parse_encoding\" attribute value to be \"treebank\" or \"SynNode\".");
	}

	// Decide which offset types we should include.
	if (optionMap.find(X_offset_type) != optionMap.end()) {
		std::wstring offsets = transcodeToStdWString(optionMap.find(X_offset_type)->second.c_str());
		std::transform(offsets.begin(), offsets.end(), offsets.begin(), towlower);
		if (offsets == L"all") {
			include_byte_offsets = true;  include_char_offsets = true;  include_edt_offsets = true;
		} else if (offsets == L"byte") {
			include_byte_offsets = true;  include_char_offsets = false; include_edt_offsets = false;
		} else if (offsets == L"char") {
			include_byte_offsets = false; include_char_offsets = true;  include_edt_offsets = false;
		} else if (offsets == L"edt" || offsets == L"EDT") {
			include_byte_offsets = false; include_char_offsets = false; include_edt_offsets = true;
		} else {
			throw UnexpectedInputException("XMLTheoryElement::SerializationOptions::SerializationOptions",
				"Expected \"offset_type\" attribute value to be \"all\", \"byte\", \"char\", or \"edt\".");
		}
	}
}

void XMLTheoryElement::SerializationOptions::setOptionsFromParamReader() {

	external_original_text = ParamReader::getOptionalTrueFalseParamWithDefaultVal("external_original_text", external_original_text);
	use_verbose_ids = ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_verbose_ids", use_verbose_ids);
	use_hierarchical_ids = ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_hierarchical_ids", use_hierarchical_ids);
	use_condensed_offsets = ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_condensed_offsets", use_condensed_offsets);
	use_implicit_tokens = ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_implicit_tokens", use_implicit_tokens);
	include_mentions_as_comments = ParamReader::getOptionalTrueFalseParamWithDefaultVal("include_mentions_as_comments", include_mentions_as_comments);
	include_spans_as_comments = ParamReader::getOptionalTrueFalseParamWithDefaultVal("include_spans_as_comments", include_spans_as_comments);
	include_spans_as_elements = ParamReader::getOptionalTrueFalseParamWithDefaultVal("include_spans_as_elements", include_spans_as_elements);
	include_mention_transliterations = ParamReader::getOptionalTrueFalseParamWithDefaultVal("include_mention_transliterations", include_mention_transliterations);
	include_mention_confidences = ParamReader::getOptionalTrueFalseParamWithDefaultVal("include_mention_confidences", include_mention_confidences);
	include_name_transliterations = ParamReader::getOptionalTrueFalseParamWithDefaultVal("include_name_transliterations", include_name_transliterations);
	include_canonical_names = ParamReader::getOptionalTrueFalseParamWithDefaultVal("include_canonical_names", include_canonical_names);

    #ifndef MT_DECODER
	if (include_mention_transliterations || include_name_transliterations) {
		throw UnexpectedInputException("XMLTheoryElement::SerializationOptions::setOptionsFromParamReader()",
									   "include_*_transliterations options not supported: this version of SERIF was not linked with the MTDecoder library.");
	}
    #endif

	std::string input_type_str = ParamReader::getParam("input_type");
	if (input_type_str != "") {
		boost::to_lower(input_type_str);

		if (input_type_str == "auto" || input_type_str == "dtra" || input_type_str == "text" || input_type_str == "sgm" || input_type_str == "osctext" || 
			input_type_str == "proxy" || input_type_str == "events_workshop" || input_type_str == "discussion_forum") // add 3 new types, e.g., "osctext"

			input_type = ParamReader::getParam(Symbol(L"input_type"));
		else
			throw UnexpectedInputException("XMLTheoryElement::SerializationOptions::setOptionsFromParamReader()",
				"Expected \"input_type\" attribute value to be \"auto\", \"dtra\", \"text\", \"osctext\", \"sgm\", or \"discussion_forum\".");
	}

	// Decide how to encode parse trees.
	std::string parse_encoding = ParamReader::getParam("parse_encoding");
	if (parse_encoding != "") {
		boost::to_lower(parse_encoding);
		if (parse_encoding == "treebank")
			use_treebank_tree = true;
		else if (parse_encoding == "synnode")
			use_treebank_tree = false;
		else
			throw UnexpectedInputException("XMLTheoryElement::SerializationOptions::setOptionsFromParamReader()",
				"Expected \"parse_encoding\" attribute value to be \"treebank\" or \"SynNode\".");
	}

	// Decide which offset types we should include.
	std::string offsets = ParamReader::getParam("offset_type");
	if (offsets != "") {
		boost::to_lower(offsets);
		if (offsets == "all") {
			include_byte_offsets = true;  include_char_offsets = true;  include_edt_offsets = true;
		} else if (offsets == "byte") {
			include_byte_offsets = true;  include_char_offsets = false; include_edt_offsets = false;
		} else if (offsets == "char") {
			include_byte_offsets = false; include_char_offsets = true;  include_edt_offsets = false;
		} else if (offsets == "edt") {
			include_byte_offsets = false; include_char_offsets = false; include_edt_offsets = true;
		} else {
			throw UnexpectedInputException("XMLTheoryElement::SerializationOptions::SerializationOptions",
				"Expected \"offset_type\" attribute value to be \"all\", \"byte\", \"char\", or \"edt\".");
		}
	}
}

void XMLTheoryElement::SerializationOptions::setBooleanOption(const XMLTheoryElement::SerializationOptions::OptionMap &optionMap,
														bool &option, xstring option_name) 
{
	if (optionMap.find(option_name) == optionMap.end()) 
		return;
	xstring value = optionMap.find(option_name)->second;
	if (XMLString::compareIString(value.c_str(), X_TRUE)==0)
		option = true;
	else if (XMLString::compareIString(value.c_str(), X_FALSE)==0)
		option = false;
	else
		throw UnexpectedInputException("XMLTheoryElement::SerializationOptions::setBooleanOption",
			("Bad value for serialization option "+
			 transcodeToStdString(option_name.c_str())+": "+
			 transcodeToStdString(value.c_str())).c_str());
}

}
