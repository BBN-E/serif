#pragma once
#include <string>
#include <vector>
#include "boost/shared_ptr.hpp"
#include "boost/noncopyable.hpp"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/bsp_declare.h"
#include "SlotFillerTypes.h"

BSP_DECLARE(Target)
BSP_DECLARE(SlotFiller)
BSP_DECLARE(SlotConstraints)
BSP_DECLARE(SlotPairConstraints)
typedef std::vector<Target_ptr> Targets;

/** A specification for a relation (e.g. "invent(x,y)" or concept (e.g. 
  * "person(x)") that LearnIt should attempt to learn.  Each Target is 
  * characterized by a name, which should be unique across both relations
  * and concepts.
  *
  * In addition, the Target class encodes constraints on the slot values
  * ("x" and "y") for a given concxept or relation, by associating a 
  * SlotConstraints object with each (valid) slot.
  *
  * The (C++) Target class intentionally does NOT encode constraints that
  * are placed on relations between slot values (e.g., one-to-one), or between
  * different targets, since these constraints are not used anywhere in the
  * C++ portion of LearnIt.
  */
class Target: private boost::noncopyable {
public:
	//======================================================================
	// Constructor

	Target(const std::wstring& name,
            const std::vector<SlotConstraints_ptr> &slot_constraints,
            const std::vector<SlotPairConstraints_ptr> &slot_pair_constraints,
            const std::wstring &elf_ontology_type, 
            const std::vector<std::vector<int> > sufficient_slots);
	
	//======================================================================
	// Types
	typedef std::vector<int> SlotSet;

	//======================================================================
	// Accessors

	/** Return this target's unique name (e.g., "invent"). */
	std::wstring getName() const { return _name; }

	/** Return the slot constraints for the given slot. */
	SlotConstraints_ptr getSlotConstraints(int slot_num) const { return _slot_constraints.at(slot_num); }

	/** Return the slot constraints for all the slots. */
	std::vector<SlotConstraints_ptr> getAllSlotConstraints() const { return _slot_constraints; }

    /** Return the slot pair constraints for all slots. */
	std::vector<SlotPairConstraints_ptr> getSlotPairConstraints() const { return _slot_pair_constraints; }

	/** Return the number of slots that this target's seeds expect. */
	int getNumSlots() const { return static_cast<int>(_slot_constraints.size()); }

	/** Return a string matching this found relation to a term in the
	  * ontology being used to produce ELF output. */
	std::wstring getELFOntologyType() const { return _elf_ontology_type; }

	const std::vector<SlotSet>& getSufficientSlotSets() const { return _sufficient_slots; }
	bool satisfiesSufficientSlots(const SlotFillersVector& slotFillerPossibilities) const;
	bool slotsSuffice(const SlotSet& slots) const;
	bool checkCoreferentArguments(const SlotFillerMap& slotFillerMap) const;

	//======================================================================
	// Operators

	/** Return true if this target is equal to rhs.  Two targets are considered
	  * equal iff all of their attributes are equal. */
	bool operator==(const Target& rhs) const;
	bool operator!=(const Target& rhs) const { return !(*this == rhs); }

	// so we can use target in std::maps
	bool operator<(const Target& rhs) const;

	//======================================================================
	// String Representations
	std::wstring toString() const {
		return getName();
	}
private:
	std::wstring _name;
	std::vector<SlotConstraints_ptr> _slot_constraints;
        std::vector<SlotPairConstraints_ptr> _slot_pair_constraints;
	std::wstring _elf_ontology_type;
	std::vector<SlotSet> _sufficient_slots;

	void _assertNameUniqueness(const Target& rhs) const;
};

inline std::wostream& operator<<(std::wostream& out, const Target& target)
{ out << target.toString(); return out; }
inline UTF8OutputStream& operator<<(UTF8OutputStream& out, const Target& target)
{ out << target.toString(); return out; }

