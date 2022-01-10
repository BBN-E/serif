#ifndef _TEMPORAL_TRAINING_DATA_GENERATOR_
#define _TEMPORAL_TRAINING_DATA_GENERATOR_

#include <fstream>
#include <boost/archive/text_woarchive.hpp>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/BoostUtil.h"
#include "LearnIt/Eigen/Core"
#include "Temporal/TemporalAttributeAdder.h"

BSP_DECLARE(ElfRElation)
BSP_DECLARE(TemporalAttribute)
BSP_DECLARE(TemporalTypeTable)
BSP_DECLARE(TemporalFeature)
BSP_DECLARE(FeatureMap)
BSP_DECLARE(TemporalInstanceGenerator)
BSP_DECLARE(TemporalInstance)
BSP_DECLARE(FeatureMap)
BSP_DECLARE(TemporalFeatureVectorGenerator)
	
BSP_DECLARE(TemporalTrainingDataGenerator)

class TemporalTrainingDataGenerator : public TemporalAttributeAdder {
public:
	void addTemporalAttributes(ElfRelation_ptr relation, const DocTheory* dt, int sn);
	~TemporalTrainingDataGenerator();
	static TemporalTrainingDataGenerator_ptr create(const std::string& outputDir,
			const std::string& prefix="");
private:
	TemporalTrainingDataGenerator(TemporalDB_ptr db,
			const std::string& fvFile, const std::string& previewFile);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(TemporalTrainingDataGenerator, 
			TemporalDB_ptr, const std::string&, const std::string&);
	FeatureMap_ptr _featureMap;
	TemporalFeatureVectorGenerator_ptr _featureGenerator;
	std::wofstream _fvsStream;
	std::wofstream _previewStrings;
	boost::archive::text_woarchive _fvs;
	int _instanceCount;
};
#endif
