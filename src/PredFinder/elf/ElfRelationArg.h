/**
 * Parallel implementation of ElfRelationArg object based on Python
 * implementation in ELF.py. Uses Xerces-C++ for XML writing.
 *
 * @file ElfRelationArg.h
 * @author nward@bbn.com
 * @date 2010.06.16
 **/

#pragma once

#include "Generic/common/Symbol.h"
#include "Generic/common/bsp_declare.h"
#include "PredFinder/common/ContainerTypes.h"
#include "ElfIndividual.h"
#include "ElfType.h"
#include "xercesc/dom/DOM.hpp"

#include <string>
#include <map>

// Forward declaration to use shared pointer for class in class
class SynNode;
class DocTheory;
class Mention;
class SlotConstraints;
class SlotFiller;
class ElfRelationArg;
BSP_DECLARE(SlotConstraints);
BSP_DECLARE(SlotFiller);

/**
 * Shared pointer for use in vectors.
 *
 * @author nward@bbn.com
 * @date 2010.05.14
 **/
BSP_DECLARE(ElfRelationArg);
BSP_DECLARE(ReturnPatternFeature);
/**
 * Contains one argument for a relation. Used to convert
 * a LearnIt SlotFiller to an <arg> XML element.
 *
 * @author nward@bbn.com
 * @date 2010.05.14
 **/
class ElfRelationArg {
public:
	ElfRelationArg(const DocTheory* doc_theory, const SlotConstraints_ptr constraints, const SlotFiller_ptr slot);
	ElfRelationArg(const DocTheory* doc_theory, ReturnPatternFeature_ptr feature);
	ElfRelationArg(ElfRelationArg_ptr other);
	ElfRelationArg(const std::wstring & role, const ElfIndividual_ptr individual);
	ElfRelationArg(const std::wstring & role, const std::wstring & type_string, const std::wstring & value, 
		const std::wstring & text = L"", EDTOffset start = EDTOffset(), EDTOffset end = EDTOffset());
	ElfRelationArg(const xercesc::DOMElement* arg, const ElfIndividualSet & individuals, bool exclusive_end_offsets = false);
	int compare(const ElfRelationArg & other) const;
	bool isSameAndContains(const  ElfRelationArg_ptr other, const DocTheory* docTheory) const;
    bool less_than(const ElfRelationArg & other) const {
        return (compare(other) < 0);}
	void dump(std::ostream &out, int indent = 0) const;
	std::wstring to_dbg_string(bool get_individual = false) const;
	// can be useful to turn off IDs to simplify diffs for debugging
	static void set_show_id(bool show_id) {_show_id = show_id;} 

	/**
	 * Inlined accessor to the argument's underlying role string.
	 *
	 * @return The value of _role.
	 *
	 * @author nward@bbn.com
	 * @date 2010.08.18
	 **/
	std::wstring get_role(void) const { return _role; }

	/**
	 * Inlined mutator to the argument's underlying role string.
	 *
	 * @param role The new value of _role.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.13
	 **/
	void set_role(const std::wstring & role) { _role = role; }

	/**
	 * Inlined accessor to the argument's underlying type, if any.
	 *
	 * @return The value of _type, which might be null.
	 *
	 * @author nward@bbn.com
	 * @date 2010.08.19
	 **/
	ElfType_ptr get_type(void) const { if (_individual.get() != NULL) { return _individual->get_type(); } else { throw UnrecoverableException("ElfRelationArg::get_type()", "Arg doesn't have an individual."); } }
	
	/**
	 * Inlined accessor to the argument's underlying individual, if any.
	 *
	 * @return The value of _individual, which might be null.
	 , 
	 * @author nward@bbn.com
	 * @date 2010.06.23
	 **/
	ElfIndividual_ptr get_individual(void) const { return _individual; }

	/**
	 * Inlined accessor to the argument's start offset, if any.
	 *
	 * @return The value of _start, which might be -1.
	 *
	 * @author nward@bbn.com
	 * @date 2010.06.15
	 **/
	EDTOffset get_start(void) const { return _start; }

	/**
	 * Inlined accessor to the argument's end offset, if any.
	 *
	 * @return The value of _end, which might be -1.
	 *
	 * @author nward@bbn.com
	 * @date 2010.06.15
	 **/
	EDTOffset get_end(void) const { return _end; }
	const std::wstring& get_text(void) const;
	void set_individual(ElfIndividual_ptr individual);
	bool offsetless_equals(const ElfRelationArg_ptr other, bool check_roles = true);
	xercesc::DOMElement* to_xml(xercesc::DOMDocument* doc, const std::wstring & docid, bool suppress_type = false) const;
	bool individual_has_type(const std::wstring & compType);

private:
	// DATA MEMBERS


	/**
	 * A (possibly namespace-prefixed) unique identifier for this
	 * slot within the relation. Allows for distinguishing args
	 * in relations defined across arguments of the same type.
	 **/
	std::wstring _role;

	/**
	 * The underlying individual linked to this argument.
	 * Stores either a reference to an <individual> or an
	 * ElfIndividual containing a literal (stored as a string).
	 **/
	ElfIndividual_ptr _individual;

	/**
	 * Starting Serif character offset into the source document.
	 * The document starting at this offset should contain the
	 * arg _text. Optional, but must be paired with _end.
	 **/
	EDTOffset _start;

	/**
	 * Ending Serif character offset into the source document.
	 * The document ending at this offset should contain the
	 * arg _text. Optional, but must be paired with _start.
	 **/
	EDTOffset _end;

	/**
	 * Document substring between _start and _end. Optional.
	 **/
	std::wstring _text;
	/**
	 * It can be useful to turn off IDs to simplify sorting for debugging purposes.
	 **/
	static bool _show_id;
};

/**
 * Defines a dictionary (key = string; value = vector of ElfRelationArg elements)
 * intended for use in an ElfIndividualClusterMember
 * for collecting the arguments relevant to a particular
 * individual. Role name is used as a key. Generally, a role name 
 * will point to a single arg. However, the pattern may yield multiple 
 * args for a given role name, all of which are retained until we
 * split the original relation into multiple relations, each one
 * containing a unique instance of a particular arg. Thus, the value
 * pointed to by the key is a vector of args rather than a single
 * arg, though the size of the vector is usually 1 even in earlier stages,
 * and is always 1 after the uniquify step has been performed.
 * Identical roles from different relations shouldn't be mixed.
 *
 * @author nward@bbn.com
 * @date 2010.08.25
 **/
//typedef std::map<std::wstring, std::vector<ElfRelationArg_ptr>> ElfRelationArgMap;

size_t hash_value(ElfRelationArg_ptr const& arg);
