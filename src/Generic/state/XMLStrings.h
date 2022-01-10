// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XML_STRINGS_H
#define XML_STRINGS_H

/** This header provides XML string support for SERIF's XML serialization.  In 
  * particular, strings that are used to construct XML are required (by the
  * xercesc library) to be XMLCh* strings.  This header defines:
  *
  *   (1) The classes xstring and xstringstream, which are analogous to 
  *       std::wstring and std::wstringstream, except that they use XMLCh
  *       characters rather than wchar_t characters.
  *
  *   (2) A set of static XMLCh* string constants that are used repeatedly when
  *       serializing SERIF objects to XML.  These string constants all begin
  *       with the prefix "X_", and generally follow the convention that the
  *       identifier name after the prefix is the content of the string.  E.g.,
  *       X_start_offset is a string constant containing the string "start_offset".
  */

#include <string>
#include "Generic/common/Symbol.h"
#include <xercesc/util/XercesDefs.hpp>

// This constant is used to check whether XMLCh is actually the same
// type as wchar_t -- if so, then we can save time since we don't need
// to convert back and forth.  It should probably be set automatically
// somehow (using cmake?), but for now we'll just leave it turned on -
// only for Windows, though; it doesn't appear to be true in Linux.
#if defined(_WIN32)
	#define XMLCH_IS_WCHAR_T
#endif

namespace SerifXML {

// xstring and xstringstream are like wstring and wstringstream, except that they
// use XMLCh as their character type.  (But note that under some configurations, 
// XMLCh is identical to wchar_t!)
typedef std::basic_string<XMLCh, std::char_traits<XMLCh>, std::allocator<XMLCh> > xstring;
//typedef std::basic_stringstream<XMLCh, std::char_traits<XMLCh>, std::allocator<XMLCh> > xstringstream;

std::string transcodeToStdString(const XMLCh* xmlstr);
std::wstring transcodeToStdWString(const XMLCh* xmlstr);
xstring transcodeToXString(const char* utf8str);
xstring transcodeToXString(const wchar_t* unicodestr);

// Include the file XMLStringConstants.h, which defines transcoded string consts
// for many strings that will get (re)used while serializing to XML.  
#define DECLARE_XML_STR(s) extern const XMLCh* X_##s
#define DECLARE_XML_STR2(s,s2) extern const XMLCh* X_##s
#include "Generic/state/XMLStringConstants.h"
#undef DECLARE_XML_STR
#undef DECLARE_XML_STR2

} // namespace SerifXML

#endif
