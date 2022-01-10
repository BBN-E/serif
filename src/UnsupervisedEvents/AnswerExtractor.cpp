#include "Generic/common/leak_detection.h"
#include "AnswerExtractor.h"

#include "ACEEvent.h"
#include "ACEPassageDescription.h"
#include "ACEEventGoldStandard.h"

using std::make_pair;

/*EntityLabelling BestBuyerThemeSellerByThreshold::decode(const ACEEvent& event) const {
	EntityLabelling ret(event.nEntities(), std::make_pair(-1,1.0));
	ACEEvent_ptr premEvent = matchingEvent(event);

	if (ret.size()) {
		IdxScore bestBuyer = maxOverSentence(*premEvent, 0);
		ret[bestBuyer.first] = std::make_pair(0, 1.0);

		IdxScore bestArtifact = maxOverSentence(*premEvent, 2);
		if (ret[bestArtifact.first].first == -1) {
			ret[bestArtifact.first] = std::make_pair(2, 1.0);
		}

		IdxScore bestSeller = maxOverSentence(*premEvent, 1);
		if (ret[bestSeller.first].first == -1 && bestSeller.second >= _threshold) {
			ret[bestSeller.first] = std::make_pair(1, 1.0);
		}
	}

	return ret;
}*/
/*
EntityLabelling MaxVarThreshold::decode(const GoldStandardACEInstance& gsInst) const
{
	EntityLabelling ret(gsInst.serifEvent->nEntities(), std::make_pair(-1, 1.0));

	if (ret.size()) {
		ACEEvent_ptr premEvent = matchingEvent(*gsInst.serifEvent);

		for (size_t i=0; i<ret.size(); ++i) {
			std::vector<double> probs = premEvent->probsOfEntity((unsigned int)i);
			size_t max_idx;
			double max_val = 0.0;

			for (size_t j=0; j<probs.size(); ++j) {
				if (probs[j] > max_val) {
					max_val = probs[j];
					max_idx = j;
				}
			}

			if (max_val > _threshold) {
				ret[i] = make_pair(max_idx, max_val);
			}
		}
	}

	return ret;
}


EntityLabelling MaxSentenceThreshold::decode(const GoldStandardACEInstance& gsInst) const
{
	EntityLabelling ret(gsInst.serifEvent->nEntities(), std::make_pair(-1, 1.0));

	if (ret.size()) {
		ACEEvent_ptr premEvent = matchingEvent(*gsInst.serifEvent);
		std::vector<double> probs = premEvent->probsOfEntity((unsigned int)0);

		for (size_t i=0; i<probs.size(); ++i) {
			IdxScore bestForLabel = maxOverSentence(*premEvent, i);
			if (bestForLabel.second >= _threshold) {
				ret[bestForLabel.first] = make_pair(i, bestForLabel.second);
			}
		}
	}

	return ret;
}

EntityLabelling BestBuyerThemeSellerByThreshold::decode(const GoldStandardACEInstance& gsInst) const {
	EntityLabelling ret(gsInst.serifEvent->nEntities(), std::make_pair(-1,1.0));
	ACEEvent_ptr premEvent = matchingEvent(*gsInst.serifEvent);

	if (ret.size()) {
		IdxScore bestBuyer = maxOverSentence(*premEvent, 0);
		ret[bestBuyer.first] = std::make_pair(0, 1.0);

		IdxScore bestArtifact = maxOverSentence(*premEvent, 2);
		if (ret[bestArtifact.first].first == -1) {
			ret[bestArtifact.first] = std::make_pair(2, 1.0);
		}

		IdxScore bestSeller = maxOverSentence(*premEvent, 1);
		if (ret[bestSeller.first].first == -1 && bestSeller.second >= _threshold) {
			ret[bestSeller.first] = std::make_pair(1, 1.0);
		}
	}

	return ret;
}
*/
