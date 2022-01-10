#ifndef _SLOT_BIAS_FEATURE_H_
#define _SLOT_BIAS_FEATURE_H_

#include <vector>
#include <string>
#include "Generic/common/bsp_declare.h"
#include "LearnIt/SlotFillerTypes.h"
#include "LearnIt/MentionToEntityMap.h"
#include "Feature.h"

BSP_DECLARE(SlotBiasFeature)
class DocTheory;

class SlotBiasFeature : public MatchMatchableFeature {
public:
	SlotBiasFeature();
	virtual bool matchesMatch(const SlotFillerMap& match,
		const SlotFillersVector& slotFillersVector,
		const DocTheory* docInfo, int sent_no) const;
};

#endif
