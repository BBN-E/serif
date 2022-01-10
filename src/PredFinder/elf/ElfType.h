/**
 * Parallel implementation of ElfType object based on Python
 * implementation in ELF.py. Constructor-only subclass of
 * ElfString, so no associated source file.
 *
 * @file ElfType.h
 * @author nward@bbn.com
 * @date 2010.06.16
 **/

#pragma once

#include "ElfString.h"
#include <string>
#include <set>

/**
 * Contains one ontology type, associated with either a
 * relation argument or a document entity, and the optional
 * offsets indicating the evidence for that type.
 *
 * In an argument context, the offsets are duplicated in
 * the argument's offsets, but are still stored with the type.
 *
 * @author nward@bbn.com
 * @date 2010.06.02
 **/
class ElfType : public ElfString {
public:
	/**
	 * Inlined superclass constructor. Creates an ElfString
	 * that will contain an ontology type.
	 **/
	ElfType(const std::wstring & type, const std::wstring & string = L"", EDTOffset start = EDTOffset(), EDTOffset end = EDTOffset()) 
		: ElfString(type, ElfString::TYPE, -1.0, string, start, end) { }
};

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.05.21
 **/
typedef boost::shared_ptr<ElfType> ElfType_ptr;

/**
 * Set type for use in ElfIndividual and related classes.
 *
 * @author nward@bbn.com
 * @date 2010.06.23
 **/
typedef std::set<ElfType_ptr, ElfString_less_than> ElfTypeSet;
