#pragma once

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "SlotFillerTypes.h"
#include "MentionToEntityMap.h"

BSP_DECLARE(AbstractLearnItSlot)
BSP_DECLARE(Target)
class DocTheory;

class ObjectWithSlots {
public:
	ObjectWithSlots(Target_ptr target, const std::vector<std::wstring>& slots);
	virtual ~ObjectWithSlots();

	const AbstractLearnItSlot_ptr getSlot(size_t slot_num) const;
	const std::vector<AbstractLearnItSlot_ptr>& getAllSlots() const;
	size_t numSlots() const;
	std::wstring commaSeparatedSlots() const;
	Target_ptr getTarget() const;

	virtual bool findInSentence( 
		const SlotFillersVector& slotFillersVector,
		const DocTheory* doc, Symbol docid, int sent_no,
		std::vector<SlotFillerMap>& output,
		const std::set<Symbol>& overlapIgnoreWords = std::set<Symbol>());

private:
	std::vector<AbstractLearnItSlot_ptr> _slots;
	Target_ptr _target;
	SlotFillersVector _slotFillerPossibilities;
};
