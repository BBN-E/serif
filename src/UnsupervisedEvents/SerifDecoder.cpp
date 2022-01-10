#include "Generic/common/leak_detection.h"
#include "SerifDecoder.h"
#include "ACEEvent.h"
#include "ACEEventGoldStandard.h"
#include "ProblemDefinition.h"
#include "AnswerMatrix.h"

AnswerMatrix SerifDecoder::decode(const GoldStandardACEInstance& gsInst) const {
	const ACEEvent& event = *gsInst.serifEvent;
	AnswerMatrix ret(event.nEntities(), _problem->nClasses());

	for (size_t i=0; i<event.nEntities(); ++i) {
		int role = event.gold((unsigned int)i);

		if (role != -1) {
			ret.forEntity(i)[role] = 1.0;
		}
	}

	return ret;
}
