#include <vector>
#include <string>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#include "Generic/common/leak_detection.h"
#include "Generic/theories/DocTheory.h"
#include "LearnIt/MainUtilities.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/SlotFiller.h"
#include "LearnIt/ObjectWithSlots.h"

using std::vector;
using std::wstring;
using boost::make_shared;

ObjectWithSlots::ObjectWithSlots(Target_ptr target,
								 const vector<wstring>& slots ) :
_slots(), _target(target), _slotFillerPossibilities(target->getNumSlots())
{
	if (slots.size() != (size_t)target->getNumSlots()) {
		throw UnexpectedInputException("ObjectWithSlots::ObjectWithSlots",
			"Number of slots in constructor argument must match number in target");
	}

	for (size_t i=0; i<slots.size(); ++i) {
		SlotConstraints_ptr slot_constraints = target->getSlotConstraints(static_cast<int>(i));
		_slots.push_back(make_shared<AbstractLearnItSlot>(slot_constraints,
			MainUtilities::normalizeString(slots[i])));
	}

	for (SlotFillersVector::iterator it=_slotFillerPossibilities.begin(); 
		it!=_slotFillerPossibilities.end(); ++it) 
	{
		*it = boost::make_shared<SlotFillers>();
	}
}

ObjectWithSlots::~ObjectWithSlots() {}

const AbstractLearnItSlot_ptr ObjectWithSlots::getSlot(size_t slot_num) const {
	return _slots[slot_num];
}

size_t ObjectWithSlots::numSlots() const {
	return _slots.size();
}

const vector<AbstractLearnItSlot_ptr>& ObjectWithSlots::getAllSlots() const {
	return _slots;
}

wstring ObjectWithSlots::commaSeparatedSlots() const {
	std::wstringstream wss;
	wss << getSlot(0)->name();
	for (size_t i=1; i<numSlots(); ++i) {
		wss << ", " << getSlot(i)->name();
	}
	return wss.str();
}

Target_ptr ObjectWithSlots::getTarget() const {
	return _target;
}

bool ObjectWithSlots::findInSentence( 
	const SlotFillersVector& slotFillersVector,
	const DocTheory* doc, Symbol docid, int sent_no,
	std::vector<SlotFillerMap>& output, const std::set<Symbol>& overlapIgnoreWords) 
{
	for( int i = 0; i < _target->getNumSlots(); i++ ){
		SlotFillers_ptr fillers = _slotFillerPossibilities[i];
		fillers->clear();
		SlotFiller::filterBySeed( *slotFillersVector[i], *fillers,
			*getSlot(i), _target->getSlotConstraints(i),false, overlapIgnoreWords);
	}

	bool ret = false;
	// Make sure that we have filled a sufficient set of slots to uniquely pick out the desired seed.
	if (_target->satisfiesSufficientSlots(_slotFillerPossibilities)) {
		ret = ret || SlotFiller::findAllCombinations(_target,
			_slotFillerPossibilities, output);
	} 
	return ret;
}
