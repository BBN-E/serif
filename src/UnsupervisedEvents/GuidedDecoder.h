#ifndef _GUIDED_DECODER_H_
#define _GUIDED_DECODER_H_

#include <vector>
#include <string>
#include <utility>
#include <sstream>
#include "Generic/common/bsp_declare.h"
#include "ACEDecoder.h"
#include "ACEPREMDecoder.h"
#include "AnswerExtractor.h"

class ACEEvent;

class GuidedDecoder : public PREMAdapter {
public:
	GuidedDecoder(const ACEPREMDecoder_ptr& premDecoder)  :
		PREMAdapter(premDecoder) {}
	AnswerMatrix decode(const GoldStandardACEInstance& event) const;
	std::string name() const {
		return "GuidedDecoder";
	}
	static const double HIGH_THRESHOLD;
	static const double MEDIUM_THRESHOLD;
	static const double LOW_THRESHOLD;
	static const double RARE_THRESHOLD;
};

#endif

