#pragma once

#include <vector>
#include <set>
#include "Generic/common/bsp_declare.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/SlotFillerTypes.h"
class DocTheory;

BSP_DECLARE(Target)
BSP_DECLARE(SlotCanopy)
BSP_DECLARE(SlotsCanopy)
BSP_DECLARE(SlotsIdentityFeature)

class SlotsCanopy {
public:
	SlotsCanopy(Target_ptr target, const std::vector<SlotsIdentityFeature_ptr>& features);
	bool unconstraining() const;
	void potentialMatches(const SlotFillersVector& fillersVec, const DocTheory* docTheory, std::set<int>& output);

private:
	std::vector<SlotCanopy_ptr> _slotCanopies;
	std::vector<std::set<int> > _slotsPossibilities;
	std::vector<std::set<int>::const_iterator > _slotPosIterators;
	std::vector<std::set<int>::const_iterator > _slotPosIteratorEnds;
	std::vector<bool> _slotsUnconstraining;
	bool _unconstraining;

	bool allConstrainingSlotIteratorsValid() const;
};
