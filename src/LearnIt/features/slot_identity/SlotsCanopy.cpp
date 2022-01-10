#include "Generic/common/leak_detection.h"
#include <boost/make_shared.hpp>
#include "Generic/common/UnrecoverableException.h"
#include "LearnIt/Target.h"
#include "LearnIt/features/slot_identity/SlotCanopy.h"
#include "LearnIt/features/slot_identity/SlotsCanopy.h"
#include "theories/DocTheory.h"

using std::vector;
using std::set;
using boost::make_shared;

SlotsCanopy::SlotsCanopy(Target_ptr target, 
						 const std::vector<SlotsIdentityFeature_ptr>& features)
{
	_unconstraining = true;
	for (int i=0; i<target->getNumSlots(); ++i) {
		_slotsPossibilities.push_back(set<int>());
		
		SlotCanopy_ptr slot_canopy = 
			make_shared<SlotCanopy>(*target->getSlotConstraints(i), i, features);
		_slotCanopies.push_back(slot_canopy);

		bool slot_unconstraining = slot_canopy->unconstraining();
		_unconstraining = _unconstraining && slot_unconstraining;
		_slotsUnconstraining.push_back(slot_unconstraining);
	}

	_slotPosIterators.resize(target->getNumSlots());
	_slotPosIteratorEnds.resize(target->getNumSlots());
}

bool SlotsCanopy::unconstraining() const {
	return _unconstraining;
}


void SlotsCanopy::potentialMatches(const SlotFillersVector &fillersVec, const DocTheory* docTheory, std::set<int> &output)
{
	if (unconstraining()) {
		throw UnrecoverableException("SlotsCanopy::potentialMatches",
			"Cannot call potentialMatches if unconstraining() is true");
	}
	// first gather features compatible with each slot
	for (size_t slot_num = 0; slot_num < _slotCanopies.size(); ++slot_num) {
		set<int>& slotPossibilities = _slotsPossibilities[slot_num];
		if (!_slotsUnconstraining[slot_num]) {
			_slotCanopies[slot_num]->potentialMatches(*fillersVec[slot_num], docTheory, slotPossibilities);
			_slotPosIterators[slot_num] = slotPossibilities.begin();
			_slotPosIteratorEnds[slot_num] = slotPossibilities.end();
			if (_slotPosIterators[slot_num] == _slotPosIteratorEnds[slot_num]) {
				// if there's no possiblities for one slot, then we might
				// as well stop now
				return; 
			}
		}
	}

	// Then use these to find features compatible with all slots.
	// By previous checks we're guaranteed all constraining slots
	// have non-empty possibility vectors, and we are guaranteed
	// there is at least one constraining slot
	int biggest_feature_idx = -1;

	while (allConstrainingSlotIteratorsValid()) {
		// find the biggest feature index
		for (size_t slot_num = 0; slot_num < _slotPosIterators.size(); ++slot_num) {
			if (!_slotsUnconstraining[slot_num]) {
				if (*_slotPosIterators[slot_num] > biggest_feature_idx) {
					biggest_feature_idx = *_slotPosIterators[slot_num];
				}
			}
		}

		bool all_match = true;
		for (size_t slot_num = 0; slot_num < _slotPosIterators.size(); ++slot_num) {
			if (!_slotsUnconstraining[slot_num]) {
				if (*_slotPosIterators[slot_num] == biggest_feature_idx) {
					++_slotPosIterators[slot_num];
				} else {
					while (*_slotPosIterators[slot_num] < biggest_feature_idx) {
						++_slotPosIterators[slot_num];
						if (_slotPosIterators[slot_num] == _slotPosIteratorEnds[slot_num]) {
							// as soon as we run out of options for one slot,
							// we can quit
							return;
						}
					}
					if (*_slotPosIterators[slot_num] != biggest_feature_idx) {
						all_match = false;
					}
				}
			}
		}

		if (all_match) {
			output.insert(biggest_feature_idx);
		}
	}
}


bool SlotsCanopy::allConstrainingSlotIteratorsValid() const {
	for (size_t i=0; i<_slotPosIterators.size(); ++i) {
		if (!_slotsUnconstraining[i]) {
			if (_slotPosIterators[i] == _slotPosIteratorEnds[i]) {
				return false;
			}
		}
	}

	return true;
}
