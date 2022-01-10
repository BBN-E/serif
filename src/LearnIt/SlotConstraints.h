// Should this class be renamed to "SlotInfo" to correspond with the python code?

#pragma once
#include <string>
#include <vector>
#include "boost/shared_ptr.hpp"
#include "boost/noncopyable.hpp"
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(SlotConstraints)
BSP_DECLARE(Pattern)

/** A set of constraints on the values for a single slot ("x" or "y") of
  * a target concept or relation.  Currently, the following constraints are supported:
  *   - Mention Type: The type of value that we expect to fill a given slot.
  *   - Brandy Constraints: A Brandy sexpr string supplying additional 
  *     constraints for the slot value.  E.g.: "(acetype PER) (mentiontype name)"
  *   - Seed Type: constraints applied beyond Brandy in the seed matching;
  *     e.g. a Brandy TIMEX value may be seed_type "year" or "yymmdd"
  */
class SlotConstraints: private boost::noncopyable {
public:
	/* The type of value that we expect to fill the slot.  Use MENTION for ACE
	 * entities (e.g., people, places, organizations, countries), and VALUE for ACE
	 * value mentions (e.g., phone numbers, emails, URLs, job titles, and times).  
	 * From the ACE documentation: "a value is a string that further characterizes 
	 * the properties of some entity or event." 
	 * The seed_type is a possible additional modifier to specify format, etc.
	 * UNFILLED_OPTIONAL and UNMET_CONSTRAINT are special types for SlotFillers.
	 */
	enum SlotType { MENTION, VALUE, UNFILLED_OPTIONAL, UNMET_CONSTRAINTS };

	SlotConstraints(SlotType slot_type, const std::wstring& brandy_constraints, 
		const std::wstring & seed_type,
		const std::wstring & elf_ontology_type,
		const std::wstring & elf_role,
		bool use_best_name,
		bool allow_desc_training);
	SlotConstraints(const std::wstring& slot_type, const std::wstring& brandy_constraints, 
		const std::wstring & seed_type,
		const std::wstring & elf_ontology_type,
		const std::wstring & elf_role,
		bool use_best_name,
		bool allow_desc_training);

    // Default constructor:
	SlotConstraints(): _slot_type(MENTION), _brandy_constraints(L""),
		_seed_type(L""), _elf_ontology_type(L""), _elf_role(L""), 
		_use_best_name(true), _allow_desc_training(true) {}

	/** Return the mention type used by the constrained slot. */
	SlotType getMentionType() const { return _slot_type; }
	bool slotIsMentionSlot() { return (_slot_type == MENTION); }

	std::wstring getMentionTypeAsString() const;
	std::wstring getMentionTypeAsLabelTypeString() const;
	
	/** Return a Brandy sexpr string describing additional constraints on the 
	  * constrained slot. */
	const std::wstring& getBrandyConstraints() const { return _brandy_constraints; }

	/** Return a string describing seed constraints of the constrained slot. 
	 */
	const std::wstring& getSeedType() const { return _seed_type; }

	/** Return a Distillation pattern that can be used to search for mentions 
	  * or value mentions that satisfy these slot constraints. */
	Pattern_ptr getBrandyPattern() const { return _brandyPattern; }

	/** Return a string matching this slot's contents to a type in the
	  * ontology being used to produce ELF output. */
	std::wstring getELFOntologyType() const { return _elf_ontology_type; }

	/** Return a string matching this slot's role in the target to a type in the
	  * ontology being used to produce ELF output. */
	std::wstring getELFRole() const { return _elf_role; }
	
	/** Specifies whether to try to find a better name for this slot when
	    matching rather than simply using the matching string itself */
	bool useBestName() const {return _use_best_name; }

	bool allowDescTraining() const {return _allow_desc_training; }

        /** Gets a list of those slot pairs which must corefer */
	const std::vector<std::pair<int, int> >& must_corefer() {
		return _must_corefer;
	}

	/** Return true if this target is equal to rhs.  Two targets are considered
	  * equal iff all of their attributes are equal. */
	bool operator==(const SlotConstraints& rhs) const;
	bool operator!=(const SlotConstraints& rhs) const { return !(*this == rhs); }
private:
	SlotType _slot_type;
	std::wstring _brandy_constraints;
	std::wstring _seed_type;
	Pattern_ptr _brandyPattern;
	std::wstring _elf_ontology_type;
	std::wstring _elf_role;
	bool _use_best_name;
	bool _allow_desc_training;
        std::vector<std::pair<int, int> > _must_corefer;

	SlotType stringToType(const std::wstring& ws);
	void initBrandyPattern();
};

