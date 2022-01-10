#pragma once

#include <vector>
#include <string>
#include "Generic/common/bsp_declare.h"
#include "LearnIt/SlotFillerTypes.h"
#include "LearnIt/MentionToEntityMap.h"
#include "Feature.h"

BSP_DECLARE(SlotContainsWordFeature)
BSP_DECLARE(Target)
class DocTheory;

class SlotContainsWordFeature : public MatchMatchableFeature {
public:
	SlotContainsWordFeature(Target_ptr target, const std::wstring& name, 
		const std::wstring& metadata);
	virtual bool matchesMatch(const SlotFillerMap& match,
		const SlotFillersVector& slotFillersVector,
		const DocTheory* docInfo, int sent_no) const;
private:
	std::wstring _word;
	int _slot_num;
};
