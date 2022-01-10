#include "Generic/common/leak_detection.h"
#include "SlotBiasFeature.h"

#include <vector>
#include "Generic/theories/DocTheory.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/SlotFiller.h"


SlotBiasFeature::SlotBiasFeature() {}

bool SlotBiasFeature::matchesMatch(const SlotFillerMap& match,
	const SlotFillersVector& slotFillersVector,
	const DocTheory* doc, int sent_no) const 
{
	return true;
}
