#include "Generic/common/leak_detection.h"
#include "SentenceBiasFeature.h"

#include <boost/foreach.hpp>
#include "Generic/theories/DocTheory.h"
#include "LearnIt/SlotFiller.h"
#include "LearnIt/MentionToEntityMap.h"

SentenceBiasFeature::SentenceBiasFeature() {}

bool SentenceBiasFeature::matchesMatch(const SlotFillerMap& match,
	const SlotFillersVector& slotFillersVector,
	const DocTheory* doc, int sent_no) const 
{
	return true;
}
