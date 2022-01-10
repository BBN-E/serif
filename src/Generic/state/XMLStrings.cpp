// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/state/XMLStrings.h"
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <boost/lexical_cast.hpp>

namespace SerifXML {

std::string transcodeToStdString(XMLCh* xmlstr);
std::wstring transcodeToStdWString(XMLCh* xmlstr);

std::string transcodeToStdString(const XMLCh* xmlstr) {
	std::string result;
	char* utf8str = xercesc::XMLString::transcode(xmlstr);
	result += utf8str;
	xercesc::XMLString::release(&utf8str);
	return result;
}

std::wstring transcodeToStdWString(const XMLCh* xmlstr) {
	#ifdef XMLCH_IS_WCHAR_T
		return std::wstring(xmlstr);
	#else
		size_t len = xercesc::XMLString::stringLen(xmlstr);
		return std::wstring(xmlstr, xmlstr+len);
	#endif
}

xstring transcodeToXString(const char* utf8str) {
	xstring result;
	XMLCh* xmlstr = xercesc::XMLString::transcode(utf8str);
	result += xmlstr;
	xercesc::XMLString::release(&xmlstr);
	return result;
}

xstring transcodeToXString(const wchar_t* unicodestr) {
	#ifdef XMLCH_IS_WCHAR_T
		return xstring(unicodestr);
	#else
		size_t len = wcslen(unicodestr);
		return xstring(unicodestr, unicodestr+len);
	#endif
}



/* This is basically a static code block that initializes the xercesc
 * XML utilities, so we can create static XMLCh* strings.  We put it
 * in an anonymous namespace to keep from polluting the global namespace. */
namespace {
	struct XercesInitializer {
		XercesInitializer() { xercesc::XMLPlatformUtils::Initialize(); }
	};
	XercesInitializer _init;
}

// Define the DECLARE_XML_STR macro to actually *define* the strings, and 
// then import XMLStringConstants.h (which consists of a bunch of calls to
// the DECLARE_XML_STR macro).
#define DECLARE_XML_STR(s) const XMLCh* X_##s = xercesc::XMLString::transcode(#s)
#define DECLARE_XML_STR2(s,s2) const XMLCh* X_##s = xercesc::XMLString::transcode(s2)
#include "Generic/state/XMLStringConstants.h"
#undef DECLARE_XML_STR
#undef DECLARE_XML_STR2

} // namespace SerifXML
