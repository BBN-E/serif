#include "Generic/common/leak_detection.h"
#include "GuidedDecoder.h"

#include <boost/foreach.hpp>
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UnexpectedInputException.h"
#include "ACEEvent.h"
#include "ACEPassageDescription.h"
#include "ACEEventGoldStandard.h"
#include "ACEPREMDecoder.h"
#include "AnswerMatrix.h"

using std::make_pair;

const double GuidedDecoder::HIGH_THRESHOLD = 0.1;
const double GuidedDecoder::MEDIUM_THRESHOLD = 0.3;
const double GuidedDecoder::LOW_THRESHOLD = 0.8;
const double GuidedDecoder::RARE_THRESHOLD = 0.9;

AnswerMatrix GuidedDecoder::decode(const GoldStandardACEInstance& gsInst) const {
    AnswerMatrix ret(gsInst.serifEvent->nEntities(), problem().nClasses());
    ACEEvent_ptr premEvent = matchingEvent(*gsInst.serifEvent);

	if (ret.size()) {
		BOOST_FOREACH(const ProblemDefinition::DecoderGuide& decoderGuide, 
				_premDecoder->problem().decoderGuides())
		{
			unsigned int role = decoderGuide.first;
			const std::wstring& decoderString = decoderGuide.second;

			if (!decoderString.empty()) {
				for (size_t i = 0; i<decoderString.size(); ++i) { 
					wchar_t extract_char = decoderString[i];

					double threshold = 1.0;
					switch (extract_char) {
						case L'H':
							threshold = HIGH_THRESHOLD;
							break;
						case 'M':
							threshold = MEDIUM_THRESHOLD;
							break;
						case 'L':
							threshold = LOW_THRESHOLD;
							break;
						case 'R':
							threshold = RARE_THRESHOLD;
							break;
						default:
							std::wstringstream err;
							err << L"Invalid decoder string " << decoderString;
							throw UnexpectedInputException("GuidedDecoder::decode",
									err);
					}

					IdxScore best = maxAvailableOverSentence(*premEvent, role, ret);

					if (best.second >= threshold) {
						ret.forEntity(best.first)[role] = 1.0;
					}
				}
			}
		}
    }
	ret.normalize();
    return ret;
}
