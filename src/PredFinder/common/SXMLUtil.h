/**
 * Convenience methods for handling SerifXML.
 *
 * @file SXMLUtil.h
 * @author afrankel@bbn.com
 * @date 2011.08.17
 **/

#pragma once

#include "Generic/common/Offset.h"

#include "xercesc/dom/DOM.hpp"
#include <string>
#include <map>
#include <vector>

/**
 * Convenience methods for handling SerifXML.
 *
 * @author afrankel@bbn.com
 * @date 2011.08.17
 **/
namespace SXMLUtil {
    std::wstring getAttributeAsStdWString(const xercesc::DOMElement* elem, const char * attrib_name);
    
    std::wstring getAttributeAsStdWString(const xercesc::DOMElement* elem, const wchar_t * attrib_name);
    
    xercesc::DOMNodeList* getNodesByTagName(const xercesc::DOMElement* elem, const char * attrib_name);
    
    void setAttributeFromStdWString(xercesc::DOMElement* elem, const char * attrib_name, const std::wstring & attrib_contents);

    void setAttributeFromEDTOffset(xercesc::DOMElement* elem, const char * attrib_name, const EDTOffset & attrib_value);

    xercesc::DOMElement* createElement(xercesc::DOMDocument* elem, const char * new_elem_name);
    
};

