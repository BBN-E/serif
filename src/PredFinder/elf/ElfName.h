/**
 * Parallel implementation of ElfName object based on Python
 * implementation in ELF.py. Constructor-only subclass of
 * ElfString, so no associated source file.
 *
 * @file ElfName.h
 * @author nward@bbn.com
 * @date 2010.06.16
 **/

#pragma once

#include "ElfString.h"
#include <string>

/**
 * Contains one Serif name, associated with either a
 * relation argument or a document entity, and the optional
 * offsets indicating the evidence for that name (which may
 * come from a Mention not associated with any ElfRelationArg).
 *
 * In an argument context, the offsets are duplicated in
 * the argument's offsets, but are still stored with the value.
 *
 * @author nward@bbn.com
 * @date 2010.06.02
 **/
class ElfName : public ElfString {
public:
	/**
	 * Inlined superclass constructor. Creates an ElfString
	 * that will contain a Serif name.
	 **/
	ElfName(const std::wstring & name, double confidence = -1.0, 
		const std::wstring & string = L"", EDTOffset start = EDTOffset(), EDTOffset end = EDTOffset()) 
		: ElfString(name, ElfString::NAME, confidence, string, start, end) { }
};

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.06.02
 **/
typedef boost::shared_ptr<ElfName> ElfName_ptr;
