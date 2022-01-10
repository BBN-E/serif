#pragma once

#include <vector>
#include <string>
#include "Generic/common/bsp_declare.h"
#include "LearnIt/SlotFillerTypes.h"
#include "LearnIt/MentionToEntityMap.h"
#include "Feature.h"

BSP_DECLARE(YYMMDDFeature)
BSP_DECLARE(Target)
class DocTheory;

class YYMMDDFeatureBase : public MatchMatchableFeature {
public:
	virtual bool matchesMatch(const SlotFillerMap& match,
		const SlotFillersVector& slotFillersVector,
		const DocTheory* doc, int sent_no) const;
protected:
	YYMMDDFeatureBase(Target_ptr target, const std::wstring& name, 
		const std::wstring& metadata, bool pos_direction);
private:
	bool _pos_direction;
	int _slot_num;
};

class YYMMDDFeature : public YYMMDDFeatureBase {
public:
	YYMMDDFeature(Target_ptr target, const std::wstring& name,
		const std::wstring& metadata);
};

class NotYYMMDDFeature : public YYMMDDFeatureBase {
public:
	NotYYMMDDFeature(Target_ptr target, const std::wstring& name,
		const std::wstring& metadata);
};

