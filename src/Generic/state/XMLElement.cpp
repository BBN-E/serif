// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/state/XMLElement.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/XMLUtil.h"

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMException.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMEntityReference.hpp>
#include <xercesc/dom/DOMComment.hpp>
#include <xercesc/dom/DOMText.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <xercesc/util/XMLString.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

using namespace xercesc;
namespace SerifXML {

XMLElement::XMLElement() 
: _xercesDOMElement(0) {}

XMLElement::XMLElement(const XMLElement& other)
: _xercesDOMElement(other._xercesDOMElement) {} 

XMLElement& XMLElement::operator=(const XMLElement& other) {
	_xercesDOMElement = other._xercesDOMElement;
	return *this;
}

// Private constructor:
XMLElement::XMLElement(DOMElement *element)
: _xercesDOMElement(element) {}

//=============================== XML Tag ===================================

const XMLCh* XMLElement::getTag() const {
	checkNull();
	return _xercesDOMElement->getTagName();
}

bool XMLElement::hasTag(const XMLCh* tagName) const {
	checkNull();
	return (XMLString::compareIString(_xercesDOMElement->getTagName(), tagName)==0);
}

//======================== Value Conversion ============================

bool XMLElement::convertBoolFromXMLString(const XMLCh* value) const {
    if (XMLString::compareIString(value, X_TRUE)==0)
        return true;
    else if (XMLString::compareIString(value, X_FALSE)==0)
        return false;
    else
        throw boost::bad_lexical_cast();
}

// XMLCh* -> Value
template<typename ValueType> ValueType XMLElement::convertFromXMLString(const XMLCh* value) const {
#ifdef XMLCH_IS_WCHAR_T
    return boost::lexical_cast<ValueType>(value);
#else
    size_t len = xercesc::XMLString::stringLen(value);
    std::string value_str(value, value+len);
    return boost::lexical_cast<ValueType>(value_str);
#endif
}
template int XMLElement::convertFromXMLString<int>(const XMLCh* value) const;
template size_t XMLElement::convertFromXMLString<size_t>(const XMLCh* value) const;
template float XMLElement::convertFromXMLString<float>(const XMLCh* value) const;
template double XMLElement::convertFromXMLString<double>(const XMLCh* value) const;

template<> Symbol XMLElement::convertFromXMLString<Symbol>(const XMLCh* value) const {
#ifdef XMLCH_IS_WCHAR_T
    return Symbol(boost::lexical_cast<std::wstring>(value));
#else
    return Symbol(transcodeToStdWString(value));
#endif
}
template<> bool XMLElement::convertFromXMLString<bool>(const XMLCh* value) const {
    return convertBoolFromXMLString(value);
}
template<> std::string XMLElement::convertFromXMLString<std::string>(const XMLCh* value) const {
    return transcodeToStdString(value);
}
template<> const XMLCh* XMLElement::convertFromXMLString<const XMLCh*>(const XMLCh* value) const {
    return value;
}
template<> xstring XMLElement::convertFromXMLString<xstring>(const XMLCh* value) const {
    return value;
}
#ifndef XMLCH_IS_WCHAR_T
template<> std::wstring XMLElement::convertFromXMLString<std::wstring>(const XMLCh* value) const {
    size_t len = xercesc::XMLString::stringLen(value);
    return std::wstring(value, value+len);
}
#endif

// Value -> XMLCh*
template<typename ValueType> xstring XMLElement::convertToXMLString(ValueType value) const {
#ifdef XMLCH_IS_WCHAR_T
    return boost::lexical_cast<xstring>(value);
#else
    std::string result = boost::lexical_cast<std::string>(value);
    return xstring(result.begin(), result.end());
#endif
}
template xstring XMLElement::convertToXMLString<int>(int value) const;
template xstring XMLElement::convertToXMLString<size_t>(size_t value) const;
template<> xstring XMLElement::convertToXMLString<float>(float value) const {
    std::ostringstream ss;
    ss.precision(6);
    ss << value;
    return transcodeToXString(ss.str().c_str());
}
template<> xstring XMLElement::convertToXMLString<double>(double value) const {
    std::ostringstream ss;
    ss.precision(6);
    ss << value;
    return transcodeToXString(ss.str().c_str());
}

template<> xstring XMLElement::convertToXMLString<Symbol>(Symbol value) const {
    //if value is Symbol() (i.e. the null symbol), transcodeToXString will be passed a NULL pointer - bad!
    if(value.is_null()) {
        throw UnexpectedInputException("XMLElement::convertToXMLString<Symbol>", "Trying to convert the NULL Symbol to XML String. Check to make sure all attributes are set to non-NULL Symbols.");
    }
    return transcodeToXString(value.to_string());
}
template<> xstring XMLElement::convertToXMLString<const std::string&>(const std::string &value) const {
    return transcodeToXString(value.c_str());
}
template<> xstring XMLElement::convertToXMLString<std::string>(std::string value) const {
    return transcodeToXString(value.c_str());
}
template<> xstring XMLElement::convertToXMLString<const char*>(const char* value) const {
    return transcodeToXString(value);
}
template<> xstring XMLElement::convertToXMLString<bool>(bool value) const {
    return (value ? X_TRUE : X_FALSE);
}
template<> xstring XMLElement::convertToXMLString<const std::wstring&>(const std::wstring &value) const {
    return xstring(value.begin(), value.end());
}
template<> xstring XMLElement::convertToXMLString<std::wstring>(std::wstring value) const {
    return xstring(value.begin(), value.end());
}
template<> xstring XMLElement::convertToXMLString<const wchar_t*>(const wchar_t* value) const {
    size_t len = wcslen(value);
    return xstring(value, value+len);
}
#ifndef XMLCH_IS_WCHAR_T
template<> xstring XMLElement::convertToXMLString<const xstring&>(const xstring &value) const {
    return value;
}
template<> xstring XMLElement::convertToXMLString<xstring>(xstring value) const {
    return value; 
}
template<> xstring XMLElement::convertToXMLString<const XMLCh*>(const XMLCh* value) const {
    size_t len = xercesc::XMLString::stringLen(value);
    return xstring(value, value+len); 
}
#endif
    
//=========================== XML Attributes ===============================

template<typename ReturnType>
ReturnType XMLElement::getAttribute(const XMLCh* attrName) const {
	const XMLCh* attrVal = getAttributeHelper(attrName);
	if (!attrVal) 
		throw reportMissingAttribute(attrName);
	try { 
		return convertFromXMLString<ReturnType>(attrVal);
	} catch (boost::bad_lexical_cast) { 
		throw reportBadAttributeValue<ReturnType>(attrName, attrVal);
	} 
}

template<typename ReturnType>
ReturnType XMLElement::getAttribute(const XMLCh* attrName, ReturnType default_value) const {
	const XMLCh* attrVal = getAttributeHelper(attrName);
	if (!attrVal)
		return default_value;
	try {
		return convertFromXMLString<ReturnType>(attrVal);
	} catch (boost::bad_lexical_cast) { 
		throw reportBadAttributeValue<ReturnType>(attrName, attrVal);
	}
}

template<typename ValueType>
void XMLElement::setAttribute(const XMLCh* attrName, ValueType attrValue) {
	setAttributeHelper(attrName, convertToXMLString(attrValue).c_str());
}

template<typename ValueType>
UnexpectedInputException XMLElement::reportBadAttributeValue(const XMLCh* attrName, const XMLCh* attrVal) const {
	std::wstringstream err;
	err << L"Attribute " << transcodeToStdWString(attrName) << L" expects a " << getTypeName<ValueType>()
		<< L" value, but got \"" << transcodeToStdWString(attrVal) << L"\"";
	throw reportLoadError(err.str());
}

template<typename KeyType, typename ValueType>
void XMLElement::saveMap(const XMLCh* attrName, std::map<KeyType, ValueType>& m)
{
	xstring list; 

	typename std::map<KeyType, ValueType>::iterator i = m.begin();
	for( ; i != m.end(); ++i )
	{
		ValueType key = i->first;
		ValueType value = i->second;

		xstring keyString = convertToXMLString(key);
		xstring valueString = convertToXMLString(value);

		if (!list.empty())
			list += X_SPACE;

		list += keyString;
		list += X_COLON;
		list += valueString;
	}

	setAttribute(attrName, list);
}

template<typename KeyType, typename ValueType>
std::map<KeyType, ValueType> XMLElement::loadMap(const XMLCh* attrName) const
{
	std::map<KeyType, ValueType> results;

	xstring list = getAttribute<xstring>(attrName);
	if (list.size() == 0)
		return results;

	std::vector<xstring> pieces;
	boost::split(pieces, list, boost::is_any_of(xstring(X_WHITESPACE)));

	BOOST_FOREACH(xstring piece, pieces) {
		std::vector<xstring> keyValuePair;
		boost::split(keyValuePair, piece, boost::is_any_of(xstring(X_COLON)));
		if (keyValuePair.size() != 2)
			throw UnexpectedInputException("XMLElement::loadMap", "Could not parse map, must have tokens like: \"key:value\"");
		xstring keyString = keyValuePair[0];
		xstring valueString = keyValuePair[1];

		KeyType key = convertFromXMLString<KeyType>(keyString.c_str());
		ValueType value = convertFromXMLString<ValueType>(valueString.c_str());

		results[key] = value;
	}
	return results;
}

// Friendly type names for error messages.
template<> std::wstring XMLElement::getTypeName<const XMLCh*>() const { return L"string"; }
template<> std::wstring XMLElement::getTypeName<std::string>() const { return L"string"; }
template<> std::wstring XMLElement::getTypeName<xstring>() const { return L"string"; }
template<> std::wstring XMLElement::getTypeName<Symbol>() const { return L"string"; }
template<> std::wstring XMLElement::getTypeName<int>() const { return L"integer"; }
template<> std::wstring XMLElement::getTypeName<bool>() const { return L"boolean"; }
template<> std::wstring XMLElement::getTypeName<size_t>() const { return L"positive integer"; }
template<> std::wstring XMLElement::getTypeName<float>() const { return L"float"; }
template<> std::wstring XMLElement::getTypeName<double>() const { return L"float"; }
#ifndef XMLCH_IS_WCHAR_T
template<> std::wstring XMLElement::getTypeName<std::wstring>() const { return L"string"; }
template<> std::wstring XMLElement::getTypeName<const wchar_t*>() const { return L"string"; }
#endif

bool XMLElement::hasAttribute(const XMLCh* attrName) const {
	checkNull();
	return _xercesDOMElement->hasAttribute(attrName);
}

const XMLCh* XMLElement::getAttributeHelper(const XMLCh* attrName) const {
	checkNull();
	if (_xercesDOMElement->hasAttribute(attrName))
		return _xercesDOMElement->getAttribute(attrName);
	else
		return 0;
}

void XMLElement::setAttributeHelper(const XMLCh* attrName, const XMLCh* attrVal) {
	checkNull();
	_xercesDOMElement->setAttribute(attrName, attrVal);
}

UnexpectedInputException XMLElement::reportMissingAttribute(const XMLCh* attrName) const {
	std::wstringstream err;
	err << "Attribute \"" << transcodeToStdWString(attrName) << "\" is required!";
	throw reportLoadError(err.str());
}

// Explicit template instantiations: getAttribute()
template int XMLElement::getAttribute<int>(const XMLCh* attrName) const;
template int XMLElement::getAttribute<int>(const XMLCh* attrName, int default_value) const;
template size_t XMLElement::getAttribute<size_t>(const XMLCh* attrName) const;
template size_t XMLElement::getAttribute<size_t>(const XMLCh* attrName, size_t default_value) const;
template float XMLElement::getAttribute<float>(const XMLCh* attrName) const;
template float XMLElement::getAttribute<float>(const XMLCh* attrName, float default_value) const;
template double XMLElement::getAttribute<double>(const XMLCh* attrName) const;
template double XMLElement::getAttribute<double>(const XMLCh* attrName, double default_value) const;
template bool XMLElement::getAttribute<bool>(const XMLCh* attrName) const;
template bool XMLElement::getAttribute<bool>(const XMLCh* attrName, bool default_value) const;
template Symbol XMLElement::getAttribute<Symbol>(const XMLCh* attrName) const;
template Symbol XMLElement::getAttribute<Symbol>(const XMLCh* attrName, Symbol default_value) const;
template std::string XMLElement::getAttribute<std::string>(const XMLCh* attrName) const;
template std::string XMLElement::getAttribute<std::string>(const XMLCh* attrName, std::string default_value) const;
template xstring XMLElement::getAttribute<xstring>(const XMLCh* attrName) const;
template xstring XMLElement::getAttribute<xstring>(const XMLCh* attrName, xstring default_value) const;
template const XMLCh* XMLElement::getAttribute<const XMLCh*>(const XMLCh* attrName) const;
template const XMLCh* XMLElement::getAttribute<const XMLCh*>(const XMLCh* attrName, const XMLCh* default_value) const;
#ifndef XMLCH_IS_WCHAR_T
template std::wstring XMLElement::getAttribute<std::wstring>(const XMLCh* attrName) const;
template std::wstring XMLElement::getAttribute<std::wstring>(const XMLCh* attrName, std::wstring default_value) const;
#endif

// Explicit template instantiation: setAttribute()
template void XMLElement::setAttribute<int>(const XMLCh* attrName, int attrValue);
template void XMLElement::setAttribute<size_t>(const XMLCh* attrName, size_t attrValue);
template void XMLElement::setAttribute<float>(const XMLCh* attrName, float attrValue);
template void XMLElement::setAttribute<double>(const XMLCh* attrName, double attrValue);
template void XMLElement::setAttribute<bool>(const XMLCh* attrName, bool attrValue);
template void XMLElement::setAttribute<Symbol>(const XMLCh* attrName, Symbol attrValue);
template void XMLElement::setAttribute<std::string>(const XMLCh* attrName, std::string attrValue);
template void XMLElement::setAttribute<const char*>(const XMLCh* attrName, const char* attrValue);
template<> void XMLElement::setAttribute<xstring>(const XMLCh* attrName, xstring attrValue) {
	setAttributeHelper(attrName, attrValue.c_str()); }
template<> void XMLElement::setAttribute<const XMLCh*>(const XMLCh* attrName, const XMLCh* attrValue) {
	setAttributeHelper(attrName, attrValue); }

#ifndef XMLCH_IS_WCHAR_T
template void XMLElement::setAttribute<std::wstring>(const XMLCh* attrName, std::wstring attrValue);
template void XMLElement::setAttribute<const wchar_t*>(const XMLCh* attrName, const wchar_t* attrValue);
#endif

// Explicit template instantiations: saveMap()
template void XMLElement::saveMap<std::wstring, std::wstring>(const XMLCh* attrName, std::map<std::wstring, std::wstring>& m);

// Explicit template instantiations: loadMap()
template std::map<std::wstring, std::wstring> XMLElement::loadMap<std::wstring, std::wstring>(const XMLCh* attrName) const;

//=========================== XML Children ===============================

std::vector<XMLElement> XMLElement::getChildElements() const {
	checkNull();
	std::vector<XMLElement> elements;
	DOMNode *node = _xercesDOMElement->getFirstChild();
	while (node) {
		if (DOMElement *elt = dynamic_cast<DOMElement*>(node))
			elements.push_back(XMLElement(elt));
		node = node->getNextSibling();
	}
	return elements;
}

XMLElement XMLElement::getUniqueChildElement() const {
	std::vector<XMLElement> elements = getChildElements();
	if (elements.size() != 0) {
		reportLoadError("Expected exactly one child element.");
		return XMLElement();
	} else {
		return elements[0];
	}
}


XMLElement XMLElement::getUniqueChildElementByTagName(const XMLCh* tag) const {
	XMLElement result = getOptionalUniqueChildElementByTagName(tag);
	if (result.isNull()) {
		std::wstringstream err;
		err << "Missing required <" << transcodeToStdWString(tag) << "> child.";
		reportLoadError(err.str());
	}
	return result;
}

XMLElement XMLElement::getOptionalUniqueChildElementByTagName(const XMLCh* tag) const {
	XMLElement result;
	checkNull();
	DOMNode *node = _xercesDOMElement->getFirstChild();
	while (node) {
		DOMElement *elt = dynamic_cast<DOMElement*>(node);
		if (elt && (XMLString::compareIString(elt->getTagName(), tag)==0)) {
			if (result.isNull()) {
				result = XMLElement(elt);
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

std::vector<XMLElement> XMLElement::getChildElementsByTagName(const XMLCh* tag, bool warn_if_other_tags_are_found) const{
	std::vector<XMLElement> elements;
	checkNull();
	DOMNode *node = _xercesDOMElement->getFirstChild();
	while (node) {
		DOMElement *elt = dynamic_cast<DOMElement*>(node);
		if (elt) {
			if (XMLString::compareIString(elt->getTagName(), tag)==0)
				elements.push_back(XMLElement(elt));
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

XMLElement XMLElement::addChild(const XMLCh *tag) {
	checkNull();
	DOMElement *child = _xercesDOMElement->getOwnerDocument()->createElement(tag);
	_xercesDOMElement->appendChild(child);
	return XMLElement(child);
}

//============================= Text =================================

template<typename ReturnType>
ReturnType XMLElement::getText() const {
	return convertFromXMLString<ReturnType>(getTextHelper());
}

template<typename ValueType>
void XMLElement::addText(ValueType text) {
	addTextHelper(convertToXMLString(text).c_str());
}

const XMLCh* XMLElement::getTextHelper() const {
	checkNull();
	return _xercesDOMElement->getTextContent(); 
}

void XMLElement::addTextHelper(const XMLCh* text) {
	checkNull();
    _xercesDOMElement->appendChild(_xercesDOMElement->getOwnerDocument()->createTextNode(text));
	return;
	/*
    // XML processors will treat \r\n as a single character.  This is
    // problematic when we are using character offsets.  To avoid
    // this, we encode \r (carriage return) using an entity reference
    // (&#xD;) when it occurs.  For details, see: 
    // <http://www.w3.org/TR/REC-xml/#sec-line-ends>
    const XMLCh* start = text;
    const XMLCh cr = X_CARRIAGE_RETURN[0];
    try {
        doc->createEntityReference(transcodeToXString("#xD").c_str());
    } catch (DOMException& e) {
        std::cerr << transcodeToStdString(e.msg) << std::endl;
    }
    for (const XMLCh* pos = text; *pos; ++pos) {
        if (*pos == cr) {
            xstring piece(start, (pos-start));
            _xercesDOMElement->appendChild(doc->createTextNode(piece.c_str()));
             _xercesDOMElement->appendChild(doc->createEntityReference(transcodeToXString("foo").c_str()));
            start = pos+1;
        }
    }
    _xercesDOMElement->appendChild(doc->createTextNode(start));
	*/
}

// Explicit template instantiations: getText
template std::string XMLElement::getText<std::string>() const;
template Symbol XMLElement::getText<Symbol>() const;
template xstring XMLElement::getText<xstring>() const;
#ifndef XMLCH_IS_WCHAR_T
template std::wstring XMLElement::getText<std::wstring>() const;
#endif

// Explicit template instantiations: addText
template void XMLElement::addText<Symbol>(Symbol text);
template void XMLElement::addText<std::string>(std::string text);
template<> void XMLElement::addText<xstring>(xstring text) {
	addTextHelper(text.c_str()); }
template<> void XMLElement::addText<const XMLCh*>(const XMLCh* text) {
	addTextHelper(text); }
template void XMLElement::addText<const char*>(const char* text);
#ifndef XMLCH_IS_WCHAR_T
template void XMLElement::addText<std::wstring>(std::wstring text);
template void XMLElement::addText<const wchar_t*>(const wchar_t* text);
#endif

//=========================== Comments ===============================

template<typename ValueType>
void XMLElement::addComment(ValueType comment) {
	addCommentHelper(convertToXMLString(comment).c_str());
}

void XMLElement::addCommentHelper(const XMLCh* comment) {
	checkNull();

	// Replace dash with underscore (to avoid "--" in comments)
	xstring stripped_comment(comment);
	boost::algorithm::replace_all(stripped_comment, xstring(X_DASH), xstring(X_UNDERSCORE));

	_xercesDOMElement->appendChild(
		_xercesDOMElement->getOwnerDocument()->createComment(stripped_comment.c_str()));
}

// Explicit template instantiations: addComment
template void XMLElement::addComment<Symbol>(Symbol comment);
template void XMLElement::addComment<std::string>(std::string comment);
template<> void XMLElement::addComment<xstring>(xstring comment) {
	addCommentHelper(comment.c_str()); }
template<> void XMLElement::addComment<const XMLCh*>(const XMLCh* comment) {
	addCommentHelper(comment); }
#ifndef XMLCH_IS_WCHAR_T
template void XMLElement::addComment<std::wstring>(std::wstring comment);
template void XMLElement::addComment<const wchar_t*>(const wchar_t* comment);
#endif

//======================== Output =======================================

std::wstring XMLElement::toWString() {
	std::wstring result;
	XMLUtil::saveXercesDOMToString(_xercesDOMElement, result);
	return result;
}

//======================== Errors & Warnings ============================

// TODO: give some context about *where* the warning/error occured.
void XMLElement::reportLoadWarning(const std::string &msg) const {	
	SessionLogger::warn("load_warning") << msg << std::endl;
}

void XMLElement::reportLoadWarning(const std::wstring &msg) const {
	SessionLogger::warn("load_warning") << msg << std::endl;
}

UnexpectedInputException XMLElement::reportLoadError(const std::string &msg) const {
	return reportLoadError(boost::lexical_cast<std::wstring>(msg.c_str()));
}

UnexpectedInputException XMLElement::reportLoadError(const std::wstring &msg) const {
	std::stringstream err;
	err << "Error while loading <" << transcodeToStdString(getTag()) << ">: \n";
	err << OutputUtil::convertToUTF8BitString(msg.c_str()) << "\n";
	throw UnexpectedInputException("XMLElement::reportLoadError", 
		err.str().c_str());
}

} // namespace SerifXML

