#pragma once

#include <vector>
#include <string>
#include "Generic/common/bsp_declare.h"
#include "LearnIt/SlotFillerTypes.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/features/Feature.h"

BSP_DECLARE(SlotIdentityFeature)
BSP_DECLARE(Target)
BSP_DECLARE(SlotConstraints)
class DocTheory;
BSP_DECLARE(AbstractLearnItSlot)

class SlotIdentityFeature : public MatchMatchableFeature {
public:
	SlotIdentityFeature(Target_ptr target, const std::wstring& name, 
		const std::wstring& metadata);
	virtual bool matchesMatch(const SlotFillerMap& match,
		const SlotFillersVector& slotFillersVector,
		const DocTheory* doc, int sent_no) const;
	const std::wstring& slotName() const;
private:
	AbstractLearnItSlot_ptr _slot_val;
	int _slot_num;
	SlotConstraints_ptr _slot_constraints;
};
