/**
 * Parallel implementation of ElfString object based on Python
 * implementation in ELF.py. Uses Xerces-C++ for XML writing.
 *
 * ElfStrings are typically not instantiated directly, but rather
 * by one of the subclass constructors that hardcodes a
 * particular ElfString::Type.
 *
 * @file ElfString.h
 * @author nward@bbn.com
 * @date 2010.06.16
 **/

#pragma once

#include "Generic/theories/DocTheory.h"
#include "boost/shared_ptr.hpp"

#include "xercesc/dom/DOM.hpp"

#include <string>

// Forward declaration to use shared pointer for class in class
class ElfString;

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.05.21
 **/
typedef boost::shared_ptr<ElfString> ElfString_ptr;

/**
 * Contains an arbitrary string, and the optional
 * offsets indicating the evidence for that string.
 *
 * Currently used for storing types, descriptions, or
 * names associated with an ElfEntity, or types and
 * values associated with an ElfRelationArg.
 *
 * In an argument context, the offsets are duplicated in
 * the argument's offsets, but are still stored with the
 * type or value instance.
 *
 * @author nward@bbn.com
 * @date 2010.06.02
 **/
class ElfString {
public:
	/**
	 * The different types of of strings-with-offsets
	 * we can represent.
	 **/
	enum Type {
		TYPE,
		NAME,
		DESC
	};

	ElfString(const std::wstring & value, const ElfString::Type & type, double confidence = -1.0, 
		const std::wstring & string = L"", EDTOffset start = EDTOffset(), EDTOffset end = EDTOffset());
	ElfString(const ElfString_ptr other);

	/**
	 * Inlined accessor to get the ElfString's subtype.
	 *
	 * @return This string's _type.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.26
	 **/
	ElfString::Type get_type(void) const { return _type; }

	/**
	 * Inlined accessor to get the associated value.
	 *
	 * @return This string's _value.
	 *
	 * @author nward@bbn.com
	 * @date 2010.07.13
	 **/
	std::wstring get_value(void) const { return _value; }

	/**
	 * Inlined accessor to get the underlying document string.
	 *
	 * @return This string's _string.
	 *
	 * @author nward@bbn.com
	 * @date 2010.06.02
	 **/
	std::wstring get_string(void) const { return _string; }

	/**
	 * Inlined accessor to get the string's offsets.
	 *
	 * @param start This string's _start. Pass-by-reference.
	 * @param end This string's _end. Pass-by-reference.
	 *
	 * @author nward@bbn.com
	 * @date 2010.06.03
	 **/
	void get_offsets(EDTOffset &start, EDTOffset &end) const { start = _start; end = _end; }

    xercesc::DOMElement* to_xml(xercesc::DOMDocument* doc) const;

	static ElfString_ptr from_xml(const xercesc::DOMElement* element, bool exclusive_end_offsets = false);

	static bool are_coreferent(const DocTheory* doc_theory, 
		const ElfString_ptr left_string, const ElfString_ptr right_string);

	void dump(std::ostream &out, int indent = 0) const;
	std::string toDebugString(int indent) const;

private:
	/**
	 * The type of string this contains (used to modify
	 * subclass behavior).
	 **/
	ElfString::Type _type;

	/**
	 * An arbitrary string.
	 *
	 * For types, contains a namespace-prefixed RDF
	 * individual, or a placeholder type used in
	 * macro processing, or a URI.
	 *
	 * For names, contains the best Serif name string
	 * by coreference for that name.
	 *
	 * For descriptions, contains a substring of
	 * document text for that description
	 **/
	std::wstring _value;

	/**
	 * Some confidence score representing how likely it
	 * was for this string to be found, or to be associated
	 * with its offsets; generally intended for use with names
	 * and descriptors.
	 **/
	double _confidence;

	/**
	 * An arbitrary string, but should be the text from the
	 * source document contained by the associated offsets.
	 **/
	std::wstring _string;

	/**
	 * Starting Serif character offset into the source document.
	 * Optional, but must be paired with _end.
	 **/
	EDTOffset _start;

	/**
	 * Ending Serif character offset into the source document.
	 * Optional, but must be paired with _start.
	 **/
	EDTOffset _end;
};

/**
 * Comparison callable so we can use ElfString_ptr
 * in a std::set and get uniqueness on insert.
 *
 * @author nward@bbn.com
 * @date 2010.05.21
 **/
struct ElfString_less_than {
	/**
	 * We care about uniqueness of values and offsets; this way we
	 * collect all of the (potentially overlapping) offset evidence
	 * for a type, name, etc. We assume that matching offsets means
	 * matching evidence strings.
	 *
	 * @param s1 Left-hand ElfString pointer whose value
	 * and offsets are being compared.
	 * @param s2 Right-hand ElfString pointer whose value
	 * and offsets are being compared.
	 *
	 * @return True if s1's _value or _start and _end aren't equal to s2's.
	 *
	 * @author nward@bbn.com
	 * @date 2010.06.02
	 **/
	bool operator()(const ElfString_ptr s1, const ElfString_ptr s2) const
	{
		// Compare type strings, then offsets
		EDTOffset s1_start, s1_end, s2_start, s2_end;
		s1->get_offsets(s1_start, s1_end);
		s2->get_offsets(s2_start, s2_end);
		if (s1->get_value() < s2->get_value()) {
			return true;
		}
		else if (s1->get_value() > s2->get_value()) {
			return false;
		}
		else if (s1_start < s2_start) {
			return true;
		}
		else if (s1_start > s2_start) {
			return false;
		}
		else if (s1_end < s2_end) {
			return true;
		}
		else if (s1_end > s2_end) {
			return false;
		}
		else {
			return false;
		}
	}
};
