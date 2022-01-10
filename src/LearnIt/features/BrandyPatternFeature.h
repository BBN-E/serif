#pragma once

#include <string>
#include "Generic/common/bsp_declare.h"
#include "Generic/patterns/multilingual/AlignedDocSet.h"
#include "LearnIt/SlotFillerTypes.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/features/Feature.h"

BSP_DECLARE(PropPatternFeature)
BSP_DECLARE(LearnItPattern)
BSP_DECLARE(Target)
class DocTheory;

class BrandyPatternFeature : public SentenceMatchableFeature
{
public:
	BrandyPatternFeature(Target_ptr target, const std::wstring& name,
		const std::wstring& metadata, bool multi);
	virtual bool matchesSentence(const SlotFillersVector& slotFillersVector,
		const AlignedDocSet_ptr doc_set, Symbol docid, int sent_no, std::vector<SlotFillerMap>& matches) const;
	const std::wstring& patternString() const;
private:
	LearnItPattern_ptr _pattern;
};
