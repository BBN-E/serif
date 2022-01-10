// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.
//
// XMLElement: serialization & deserialization of SERIF's annotations to XML

#ifndef XML_ELEMENT_H
#define XML_ELEMENT_H

#include "Generic/state/XMLStrings.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include <boost/lexical_cast.hpp>
#include <vector>
#include <map>

// Forward declarations.
class Symbol;
class LocatedString;
XERCES_CPP_NAMESPACE_BEGIN
	class DOMElement;
XERCES_CPP_NAMESPACE_END

namespace SerifXML {
/** A single element in the XML tree used to serialize SERIF's objects.
  * Each XMLElement corresponds with a single node in the XML tree.
  * This is the base class for XMLTheoryElement used to serialize SERIF's 
  * docTheory and sub-theory objects.
  *
  * A special "NULL" XMLElement can be constructed with the default
  * constructor.  It can also be returned by some methods that check for
  * optional values.  Any attempt to access the tag, attributes, children,
  * or text of a "NULL" XMLElement will result in an exception.  Use
  * isNull() to check if an XMLElement is NULL.
  *
  * This class is not meant to be "complete" -- for example, there is 
  * currently no way to delete a child element or modify an element's
  * tag.  If you find that some functionality is missing, and you need
  * it, feel free to add a method.
  *
  * XMLElement objects are meant to be passed by value.  (They are basically
  * a wrapper for a pair of pointers, so they are cheap to copy).  As a 
  * consequence, they should not be used polymorphically and no methods
  * should be made virtual.
  */
class XMLElement {
public:
	XMLElement(); // Constructs the "NULL" XMLElement.
	~XMLElement() {}
	XMLElement(xercesc::DOMElement *element);

	XMLElement(const XMLElement& other); // Copy constructor
	XMLElement& operator=(const XMLElement& other); // Assignment operator

	/** Return true if this XMLElement is NULL -- i.e., if it doesn't refer
	  * to any actual element. */
	bool isNull() const { return _xercesDOMElement == 0; }

	// Allow XMLElement to be used in conditional expressions, to test
	// whether the XMLElement is NULL.
	operator bool() const { return !isNull(); }

//=============================== XML Tag ===================================

	/** Return this element's XML tag. */
	const XMLCh* getTag() const;

	/** Return true if this element has the specified XML tag. */
	bool hasTag(const XMLCh* tagName) const;

	// [no setTag() method is defined yet]

//=========================== XML Attributes ===============================

	/** Return true if this element defines a value for the specified
	  * XML attribute (or if the DTD defines a default value for 
	  * that attribute). */
	bool hasAttribute(const XMLCh* attrName) const;

	/** Return the value of the attribute with the specified name,
	  * using the type specified by the (explicit) template parameter 
	  * ReturnType.  If this element has no attribute with the given 
	  * name, and the XML DTD provides a default value, then return
	  * that value.  Otherwise, throw an UnexpectedInputException.
	  * The following ReturnTypes are currently supported:
	  * Symbol, wstring, bool, int, size_t, float, "const XMLCh*" */
	template<typename ReturnType>
	ReturnType getAttribute(const XMLCh* attrName) const;

	/** Return the value of the attribute with the specified name,
	  * using the type specified by the (explicit) template parameter 
	  * ReturnType.  If this element has no attribute with the given 
	  * name, and the XML DTD provides a default value, then return
	  * that value.  Otherwise, return default_value.
	  * The following ReturnTypes are currently supported:
	  * Symbol, wstring, bool, int, size_t, float, "const XMLCh*" */
	template<typename ReturnType>
	ReturnType getAttribute(const XMLCh* attrName, ReturnType default_value) const;

	/** Set the value of the specified attribute.  If the specified attribute
	  * already has a value, then report a load warning and overwrite the
	  * old value with the new one.  The following ValueTypes are currently
	  * supported: Symbol, wstring, bool, int, size_t, float, "const XMLCh*" */
	template<typename ValueType>
	void setAttribute(const XMLCh* attrName, ValueType attrValue);

//============================= XML Text =================================

	/** Return the text contents of this element using the specified
	  * type.  Currently supported types are: std::wstring and const XMLCh* */
	template<typename ReturnType>
	ReturnType getText() const;

	/** Add a given text string to the body of this element.  Generally, an
	  * element should not contain both text and child elements, though this
	  * isn't currently enforced. */
	template<typename ValueType>
	void addText(ValueType text);
	
//=========================== XML Comments ===============================

	/** Add a comment to this element.  The comment will be placed inside the
	  * body of the element, after any child elements or text that has already
	  * been added.  Dashes will be replaced with underscores (to avoid the string
	  * "--", which is disallowed in comments. */
	template<typename ValueType>
	void addComment(ValueType comment);

//=========================== XML Children ===============================

	/** Return a vector containing the child elements of this element,
	  * in order. */
	std::vector<XMLElement> getChildElements() const;

	/** Return the child element with the given tag name.  If no child
	  * element has the given tag name, or if multiple child elements
	  * have the given tag name, then throw an UnexpectedInputException. */
	XMLElement getUniqueChildElementByTagName(const XMLCh* tag) const;

	/** Return the child element with the given tag name.  If no child
	  * element has the given tag name, return a NULL XMLElement.  If
	  * multiple child elements have the given tag name, then throw an 
	  * UnexpectedInputException. */
	XMLElement getOptionalUniqueChildElementByTagName(const XMLCh* tag) const;

	/** Return a vector containing the child elements with the given tag
	  * name.  By default, a warning will be issued if this element 
	  * contains any children that do *not* have the specified tag name, 
	  * since the typical use case in our XML serialization is to have 
	  * lists consist of elements with the same tag.  If you wish to allow
	  * for children with a different tag name, then set the optional
	  * warn_if_other_tags_found parameter to false. */
	std::vector<XMLElement> getChildElementsByTagName(const XMLCh* tag, 
		bool warn_if_other_tags_found=true) const;

	/** Return the single child element of this element.  If this element
	  * has no children, or multiple children, then throw an 
	  * UnexpectedInputException. */
	XMLElement getUniqueChildElement() const;

	/** Add a new child node to this element with the given tag, and return
	  * an XMLElement pointing to it.  The new element will be empty (no
	  * attributes or children). */
	XMLElement addChild(const XMLCh* tag);

	/** Add attribute of the given name and save the keys and values of 
	  * the map in the attribute. Currently only maps between wstrings 
	  * and wstrings are allowed. To add more types, add explicit template
	  * instantiation to XMLElement.cpp . */
	template<typename KeyType, typename ValueType>
	void saveMap(const XMLCh* attrName, std::map<KeyType, ValueType>& m);

	/** Read map from attribute of given name. Currently only maps between
	  * wstrings and wstrings are allowed. To add more types, add explicit 
	  * template instantiation in XMLElement.cpp . */
	template<typename KeyType, typename ValueType>
	std::map<KeyType, ValueType> loadMap(const XMLCh* attrName) const;


//======================== Output =======================================
	std::wstring toWString();

//======================== Errors & Warnings ============================

	// Issue a warning message. (may throw an exception)
	void reportLoadWarning(const std::string &msg) const;
	void reportLoadWarning(const std::wstring &msg) const;

	// Issue a warning error. (throws an exception)
	UnexpectedInputException reportLoadError(const std::string &msg) const;
	UnexpectedInputException reportLoadError(const std::wstring &msg) const;

protected:
//======================== Member Variables ============================

	// Pointer into the parallel DOM structure
	xercesc::DOMElement* _xercesDOMElement;

//======================== Helper Methods ============================

	void requireAttribute(const XMLCh* attrName) const;

	// Raise an InternalInconsistency exception if this XMLElement is NULL (i.e.,
	// if it does not refer to any element in the DOM tree).  This helper method
	// should be called by any method that access the dom tree or the doc theory.
	void checkNull() const {
		if (isNull())
			throw InternalInconsistencyException("XMLElement::checkNull",
				"Attempt to access a NULL XMLElement.");
	}

	bool convertBoolFromXMLString(const XMLCh* value) const;
	
	template<typename ValueType>
	UnexpectedInputException reportBadAttributeValue(const XMLCh* attrName, const XMLCh* attrVal) const;
	
	UnexpectedInputException reportMissingAttribute(const XMLCh* attrName) const;

	template<typename ValueType>
	UnexpectedInputException reportBadTheoryPointerType(const XMLCh* attrName) const {
		std::wstringstream err;
		err << L"Attribute \"" << transcodeToStdWString(attrName) << L"\" pointed to an unexpected value type.";
		throw reportLoadError(err.str());
	}

	const XMLCh* getAttributeHelper(const XMLCh* attrName) const;
	void setAttributeHelper(const XMLCh* attrName, const XMLCh* attrVal);
	const XMLCh* getTextHelper() const;
	void addTextHelper(const XMLCh* text);
	void addCommentHelper(const XMLCh* text);

	template<typename T> T convertFromXMLString(const XMLCh* value) const;
	template<typename T> xstring convertToXMLString(T value) const;
	template<typename T> std::wstring getTypeName() const;

};

typedef std::vector<XMLElement> XMLElementList;

} // namespace SerifXML

#endif
