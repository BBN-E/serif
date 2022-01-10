/*#ifndef _ORACLE_DECODER_H_
#define _ORACLE_DECODER_H_

#include <string>
#include <map>
//#include "ProblemDefinition.h"
#include "ACEDecoder.h"

BSP_DECLARE(ProblemDefinition)

BSP_DECLARE(GoldToSerifOracleDecoder)
class GoldToSerifOracleDecoder : public ACEDecoder {
public:
	GoldToSerifOracleDecoder(const ProblemDefinition_ptr& problem) 
		: _problem(problem) {};
	std::string name() const { return "GtSOracleDecoder"; }
	AnswerMatrix decode(const GoldStandardACEInstance& aceInst) const;
private:
	ProblemDefinition_ptr _problem;
};

BSP_DECLARE(SerifToGoldOracleDecoder)
class SerifToGoldOracleDecoder : public ACEDecoder {
public:
	SerifToGoldOracleDecoder(const ProblemDefinition_ptr& problem)
		: _problem(problem) {};
	std::string name() const { return "StGOracleDecoder"; }
	AnswerMatrix decode(const GoldStandardACEInstance& aceInst) const;
private:
	ProblemDefinition_ptr problem;
};

#endif
*/
