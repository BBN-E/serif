#ifndef _SENTENCE_BIAS_FEATURE_H_
#define _SENTENCE_BIAS_FEATURE_H_

#include <vector>
#include "Generic/common/bsp_declare.h"
#include "LearnIt/SlotFillerTypes.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/features/Feature.h"

BSP_DECLARE(SentenceBiasFeature)
class DocTheory;

class SentenceBiasFeature : public MatchMatchableFeature
{
public:
	SentenceBiasFeature();
	virtual bool matchesMatch(const SlotFillerMap& match,
		const SlotFillersVector& slotFillersVector,
		const DocTheory* docInfo, int sent_no) const;
};

#endif
