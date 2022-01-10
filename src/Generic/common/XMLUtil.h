// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XML_UTIL_H
#define XML_UTIL_H

#include <xercesc/util/XercesDefs.hpp>
#include <iostream>

// Forward declarations.
XERCES_CPP_NAMESPACE_BEGIN
	class DOMDocument;
	class DOMNode;
	class DOMElement;
    class XMLFormatTarget;
XERCES_CPP_NAMESPACE_END

class XMLUtil {
public:
	/** Parse the specified XML file, and return a xerces DOMDocument
	* for its contents.  The caller is responsible for disposing of
	* the returned DOMDocument, which must be done using 
	* DOMDocument::release().  If a parsing error is encountered, then
	* raise an UnexpectedInputException with details about where the 
	* error occured. */
	static xercesc::DOMDocument* loadXercesDOMFromFilename(const char* filename); 
	static xercesc::DOMDocument* loadXercesDOMFromFilename(const wchar_t* filename); 

	/** Parse the XML in the given stream, and return a xerces 
	* DOMDocument for its contents.  The caller is responsible for 
	* disposing of the returned DOMDocument, which must be done using 
	* DOMDocument::release().  If a parsing error is encountered, 
	* then raise an UnexpectedInputException with details about where 
	* the error occured. */
	static xercesc::DOMDocument* loadXercesDOMFromStream(std::istream& stream); 
	static xercesc::DOMDocument* loadXercesDOMFromStream(std::wistream& stream); 

	/** Parse the XML in the given string, and return a xerces 
	* DOMDocument for its contents.  The caller is responsible for
	* disposing of the returned DOMDocument, which must be done using 
	* DOMDocument::release(). If a parsing error is encountered, then 
	* raise an UnexpectedInputException with details about where the 
	* error occured.
	*
	* If length is not specified, then it will be determined using
	* strlen (byte-string version only). */
	static xercesc::DOMDocument* loadXercesDOMFromString(const char* xml_string, size_t length=0);
	static xercesc::DOMDocument* loadXercesDOMFromString(const wchar_t* xml_string);
    
    /** Convenience method to save the XML contents of the given DOM node to the specified target. */
    static void saveXercesDOMToTarget(const xercesc::DOMNode* xml_doc, xercesc::XMLFormatTarget* target);

	/** Save the XML contents of the given DOM node in the specified file. */
	static void saveXercesDOMToFilename(const xercesc::DOMNode* xml_doc, const char* filename);
	static void saveXercesDOMToFilename(const xercesc::DOMNode* xml_doc, const wchar_t* filename);

	/** Save the XML contents of the given DOM node to the specified stream. */
	static void saveXercesDOMToStream(const xercesc::DOMNode* xml_doc, std::ostream& stream);
	static void saveXercesDOMToStream(const xercesc::DOMNode* xml_doc, std::wostream& stream);

	/** Save the XML contents of the given DOM node to the given
	  * string (overwrites its current contents, if any). */
	static void saveXercesDOMToString(const xercesc::DOMNode* xml_doc, std::string& result);
	static void saveXercesDOMToString(const xercesc::DOMNode* xml_doc, std::wstring& result);
};

#endif
