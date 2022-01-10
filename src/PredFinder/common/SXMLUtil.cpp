/**
 * Convenience methods for handling SerifXML.
 *
 * @file SXMLUtil.cpp
 * @author afrankel@bbn.com
 * @date 2011.08.17
 **/
#pragma warning(disable:4996)

#include "Generic/common/leak_detection.h"
#include "Generic/common/Offset.h"
#include "Generic/state/XMLStrings.h"
#include "SXMLUtil.h"

XERCES_CPP_NAMESPACE_USE

namespace SXMLUtil {

std::wstring getAttributeAsStdWString(const DOMElement* elem, const char * attrib_name) {
    SerifXML::xstring x_attrib_name = SerifXML::transcodeToXString(attrib_name);
    return SerifXML::transcodeToStdWString(elem->getAttribute(x_attrib_name.c_str()));
}

std::wstring getAttributeAsStdWString(const DOMElement* elem, const wchar_t * attrib_name) {
    SerifXML::xstring x_attrib_name = SerifXML::transcodeToXString(attrib_name);
    return SerifXML::transcodeToStdWString(elem->getAttribute(x_attrib_name.c_str()));
}

DOMNodeList* getNodesByTagName(const DOMElement* elem, const char * attrib_name) {
    SerifXML::xstring x_attrib_name = SerifXML::transcodeToXString(attrib_name);
	return elem->getElementsByTagName(x_attrib_name.c_str());
}    

void setAttributeFromStdWString(DOMElement* elem, const char * attrib_name, const std::wstring & attrib_contents) {
    SerifXML::xstring x_attrib_name = SerifXML::transcodeToXString(attrib_name);
    SerifXML::xstring x_attrib_contents = SerifXML::transcodeToXString(attrib_contents.c_str());
    elem->setAttribute(x_attrib_name.c_str(), x_attrib_contents.c_str());
}

void setAttributeFromEDTOffset(DOMElement* elem, const char * attrib_name, const EDTOffset & attrib_value) {
    std::wstring wstr = boost::lexical_cast<std::wstring>(attrib_value);
    setAttributeFromStdWString(elem, attrib_name, wstr);
}    

DOMElement* createElement(DOMDocument* doc, const char * new_elem_name) {
    SerifXML::xstring x_new_elem_name = SerifXML::transcodeToXString(new_elem_name);
    return doc->createElement(x_new_elem_name.c_str());
}

SerifXML::xstring transcodeNumToXString(const EDTOffset & offset) {
    return SerifXML::transcodeToXString((boost::lexical_cast<std::string>(offset)).c_str());
}

}
