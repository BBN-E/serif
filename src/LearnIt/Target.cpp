#include "Generic/common/leak_detection.h"
#include "Target.h"
#include <stdexcept>
#include <cassert>
#include <boost/foreach.hpp>
#include "Generic/common/UnexpectedInputException.h"
#include "LearnIt/MainUtilities.h"
#include "LearnIt/SlotFiller.h"

Target::Target(const std::wstring& name,
    const std::vector<SlotConstraints_ptr> &slot_constraints,
    const std::vector<SlotPairConstraints_ptr> &slot_pair_constraints,
    const std::wstring& elf_ontology_type, 
    const std::vector<std::vector<int> > sufficient_slots):
_name(name), _slot_constraints(slot_constraints),
    _slot_pair_constraints(slot_pair_constraints),
    _elf_ontology_type(elf_ontology_type), 
    _sufficient_slots(sufficient_slots) 
{
	BOOST_FOREACH(const SlotSet& slots, sufficient_slots) {
		for (size_t i=1; i<slots.size(); ++i) {
			if (slots[i] < slots[i-1]) {
				throw UnexpectedInputException("Target::Target",
					"All sufficient slot sets should have their slot numbers "
					"in sorted order.");
			}
		}
	}
}

bool Target::operator==(const Target& rhs) const {
	_assertNameUniqueness(rhs);
	return (_name == rhs._name);
}

bool Target::operator<(const Target& rhs) const {
	return (_name<rhs._name);
}

void Target::_assertNameUniqueness(const Target& rhs) const {
	// Names are supposed to be unique.
	if (_name == rhs._name) {
		assert(_slot_constraints == rhs._slot_constraints);
		assert(_sufficient_slots == rhs._sufficient_slots);
		assert(_elf_ontology_type == rhs._elf_ontology_type);
	}
}

/** Return true if the given set of slot filler possibilities includes slots that satisfy the
  * sufficient slots requirements of the given target. */
bool Target::satisfiesSufficientSlots(const SlotFillersVector& slotFillerPossibilities) const
{
	BOOST_FOREACH(const SlotSet& slotSet, getSufficientSlotSets()) {
		bool good = true;
		BOOST_FOREACH(int slot, slotSet) {
			if (slotFillerPossibilities[slot]->empty()) {
				good = false;
			}
		}
		if (good) return true;
	}
	return false;
}

bool Target::slotsSuffice(const SlotSet& slots) const {
	return find(_sufficient_slots.begin(), _sufficient_slots.end(), slots) 
		!= _sufficient_slots.end();
}

bool Target::checkCoreferentArguments(const SlotFillerMap &slotFillerMap) const {
	// currently bans any relation with coreferent arguments
	for (size_t slot_a=1; slot_a<_slot_constraints.size(); ++slot_a) {
		for (size_t slot_b = 0; slot_b < slot_a; ++slot_b) {
			SlotFillerMap::const_iterator probe_a = slotFillerMap.find(static_cast<int>(slot_a));
			SlotFillerMap::const_iterator probe_b = slotFillerMap.find(static_cast<int>(slot_b));

			if (probe_a != slotFillerMap.end() && probe_b != slotFillerMap.end()) {
				if ((*probe_a->second).sameReferent(*probe_b->second)) {
					return false;
				}
			}
		}
	}
	return true;
}
