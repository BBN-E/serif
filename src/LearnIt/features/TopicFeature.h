#pragma once

#include <vector>
#include <string>
#include "Generic/common/bsp_declare.h"
#include "LearnIt/SlotFillerTypes.h"
#include "LearnIt/MentionToEntityMap.h"
#include "Feature.h"

BSP_DECLARE(Target)

class TopicFeature : public MatchMatchableFeature {
public:
	TopicFeature(Target_ptr target, const std::wstring& name, 
		const std::wstring& metadata);
	virtual bool matchesMatch(const SlotFillerMap& match,
		const SlotFillersVector& slotFillersVector,
		const DocTheory* docInfo, int sent_no) const;


private:
	int _topic_id;
	std::wstring _topic_weight;
};
