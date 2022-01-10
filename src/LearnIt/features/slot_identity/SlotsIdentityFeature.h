#pragma once

#include <vector>
#include <string>

#include "Generic/common/bsp_declare.h"
#include "LearnIt/SlotFillerTypes.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/features/Feature.h"

class DocTheory;
BSP_DECLARE(SlotsIdentityFeature)
BSP_DECLARE(Seed)
BSP_DECLARE(Target)

class SlotsIdentityFeature : public SentenceMatchableFeature {
public:
	SlotsIdentityFeature(Target_ptr target, const std::wstring& name, 
		const std::wstring& metadata);
	virtual bool matchesSentence(const SlotFillersVector& slotFillersVector,
		const AlignedDocSet_ptr doc_set, Symbol docid, int sent_no, std::vector<SlotFillerMap>& matches) const;
	const std::wstring& slotName(int slot_num) const;
private:
	static std::vector<std::wstring> metadataToSlots(const std::wstring& metadata);

	Seed_ptr _seed;
};
