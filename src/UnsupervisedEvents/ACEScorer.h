#ifndef _ACE_SCORER_H_
#define _ACE_SCORER_H_

#include <string>
#include "GraphicalModels/DataSet.h"
#include "ACEEvent.h"
#include "ACEDecoder.h"

BSP_DECLARE(ACEDecoder)
BSP_DECLARE(ProblemDefinition)

class ACEScorer {
	public:
		static void dumpScore(const ProblemDefinition_ptr& problem,
				const DecoderLists& decoderSets,
				const std::string& output_dir);
};

#endif

