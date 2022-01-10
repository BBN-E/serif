#ifndef _ACE_DECODER_H_
#define _ACE_DECODER_H_

#include <vector>
#include <string>
#include <utility>
#include "Generic/common/bsp_declare.h"

typedef std::pair<int, double> LabelAndScore;
typedef std::vector<LabelAndScore > EntityLabelling;
class GoldStandardACEInstance;

BSP_DECLARE(ACEDecoder)
BSP_DECLARE(AnswerMatrix)
typedef std::vector<ACEDecoder_ptr> DecoderList;
typedef std::vector<DecoderList> DecoderLists;

class ACEDecoder {
	public:
		ACEDecoder() {}
		virtual std::string name() const=0;
		virtual AnswerMatrix decode(const GoldStandardACEInstance& gsInst) const=0;
};

#endif

