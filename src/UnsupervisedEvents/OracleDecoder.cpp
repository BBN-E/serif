/*#include "Generic/common/leak_detection.h"
#include <algorithm>
#include <boost/foreach.hpp>
#include "Generic/common/ParamReader.h"
#include "Generic/common/foreach_pair.hpp"
#include "OracleDecoder.h"
#include "Generic/common/SessionLogger.h"
#include "GraphicalModels/pr/FeatureImpliesClassConstraint.h"
#include "ACEEvent.h"
#include "ProblemDefinition.h"
#include "ACEEventGoldStandard.h"
#include "AnswerMatrix.h"

AnswerMatrix GoldToSerifOracleDecoder::decode(
		const GoldStandardACEInstance& gsInst) const
{
	const ACEEvent& event = *gsInst.serifEvent;
	AnswerMatrix ret(event.nEntities(), problem.nClasses());

	// for each gold entity
	const GoldAnswers& goldAnswers = *gsInst.goldAnswers;
	for (size_t goldEntity = 0; goldEntity < goldAnswers.size(); ++goldEntity) {
		int goldRole = goldAnswers[goldEntity];
		if (goldRole != -1) { // if it has a role
			// assign this role to the aligned Serif entity if it does not
			// already have a label
			int predictedEntity = goldToSerif[goldEntity];
			if (predictedEntity != -1) {
				ret[predictedEntity] = make_pair(goldRole, 1.0);
			}
		}
	}
	
	return ret;
}

AnswerMatrix SerifToGoldOracleDecoder::decode(
		const GoldStandardACEInstance& gsInst) const 
{
	const ACEEvent& event = *gsInst.serifEvent;
	EntityLabelling ret(event.nEntities(), std::make_pair(-1, 1.0));

	// for each gold entity
	const GoldAnswers& goldAnswers = *gsInst.goldAnswers;
	const std::vector<int>& serifToGold = gsInst.entityMap->serifToGold;
	for (size_t predictedEntity = 0; predictedEntity < event.nEntities(); ++predictedEntity) {
		int goldEntity = serifToGold[predictedEntity];
		if (goldEntity != -1) {
			ret[predictedEntity] = make_pair(goldAnswers[goldEntity], 1.0);
		}
	}

	return ret;
}
*/
