#ifndef _SERIF_DECODER_H_
#define _SERIF_DECODER_H_

#include "ACEDecoder.h"
#include "Generic/common/bsp_declare.h"

class AnswerMatrix;
BSP_DECLARE(SerifDecoder)
BSP_DECLARE(ProblemDefinition)
class SerifDecoder : public ACEDecoder {
	public:
		SerifDecoder(const ProblemDefinition_ptr& problem) 
			: ACEDecoder(), _problem(problem) {}
		std::string name() const { return "SerifDecoder";}
		AnswerMatrix decode(const GoldStandardACEInstance& gsInst) const;
	private:
		ProblemDefinition_ptr _problem;
};


#endif

