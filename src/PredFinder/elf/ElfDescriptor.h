/**
 * Parallel implementation of ElfDescriptor object based on Python
 * implementation in ELF.py. Constructor-only subclass of
 * ElfString, so no associated source file.
 *
 * @file ElfDescriptor.h
 * @author nward@bbn.com
 * @date 2010.06.16
 **/

#pragma once

#include "ElfString.h"
#include <string>
/**
 * Contains one descriptor, associated with either a
 * relation argument or a document entity, and the optional
 * offsets indicating the evidence for that descriptor.
 *
 * In an argument context, the offsets are duplicated in
 * the argument's offsets, but are still stored with the value.
 *
 * @author nward@bbn.com
 * @date 2010.06.02
 **/
class ElfDescriptor : public ElfString {
public:
	/**
	 * Inlined superclass constructor. Creates an ElfString
	 * that will contain a descriptor.
	 **/
	ElfDescriptor(const std::wstring & descriptor, double confidence = -1.0, 
		const std::wstring & string = std::wstring(L""), 
		EDTOffset start = EDTOffset(), EDTOffset end = EDTOffset()) : 
		ElfString(descriptor, ElfString::DESC, confidence, string, start, end) { }
};

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.06.02
 **/
typedef boost::shared_ptr<ElfDescriptor> ElfDescriptor_ptr;
