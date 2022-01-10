// disables warning from within boost serialziation
#pragma warning( disable : 4099 )
#include "Generic/common/leak_detection.h"
#include "TemporalTrainingDataGenerator.h"

#include <map>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>
#include "Generic/common/ParamReader.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/common/SessionLogger.h"
#include "Temporal/TemporalAttribute.h"
#include "Temporal/ManualTemporalInstanceGenerator.h"
#include "Temporal/TemporalDB.h"
#include "Temporal/TemporalInstance.h"
#include "Temporal/TemporalInstanceSerialization.h"
#include "Temporal/features/TemporalFeature.h"
#include "Temporal/TemporalFeatureGenerator.h"
#include "Temporal/FeatureMap.h"
#include "PredFinder/elf/ElfRelation.h"

using std::wstring;
using boost::make_shared;

TemporalTrainingDataGenerator::TemporalTrainingDataGenerator(
	TemporalDB_ptr db, const std::string& fvFile, const std::string& previewFile):
	TemporalAttributeAdder(db), _featureMap(db->createFeatureMap()),
	_featureGenerator(make_shared<TemporalFeatureVectorGenerator>(_featureMap)),
	_fvsStream(fvFile.c_str(), std::ios::binary),
	_fvs(_fvsStream), _previewStrings(previewFile.c_str()), _instanceCount(0)
{
}


void TemporalTrainingDataGenerator::addTemporalAttributes(ElfRelation_ptr relation,
				const DocTheory* dt, int sn)
{
	// we don't actually adder temporal attributes. Rather we mimic the
	// decision procedure to produce training data instead
	std::vector<TemporalInstance_ptr> instances;
	_instanceGenerator->instances(dt->getDocument()->getName(), 
			dt->getSentenceTheory(sn), relation, instances);
	_instanceCount += static_cast<int>(instances.size());

	BOOST_FOREACH(TemporalInstance_ptr inst, instances) {
		const TemporalFV fv = *_featureGenerator->fv(dt, sn, *inst);
		const TemporalInstanceData_ptr tid =
			TemporalInstanceData::create(*inst, fv);
		_fvs << *tid;
		_previewStrings << inst->previewString() << std::endl;
	}
}

// add virtual destructor
TemporalTrainingDataGenerator::~TemporalTrainingDataGenerator() {
	SessionLogger::info("sync_temporal_feature") 
		<< "Syncing temporal features to database";
	_db->syncFeatures(*_featureMap);
}

TemporalTrainingDataGenerator_ptr TemporalTrainingDataGenerator::create(
	const std::string& outputDir, const std::string& prefix)
{
	boost::filesystem::path outputDirPath =
		boost::filesystem::system_complete(boost::filesystem::path(outputDir));
	boost::filesystem::create_directories(outputDirPath);

	std::string dottedPrefix = prefix.empty()?"":(prefix + ".");
	std::string dbFile = outputDir + "/temporal." + dottedPrefix + "db";
	std::string fvFile = outputDir + "/temporal." + dottedPrefix + "fvs";
	std::string previewFile = outputDir + "/temporal." + dottedPrefix +
		"preview";

	SessionLogger::info("temporal_training_generation")
		<< L"Writing temporal training DB to " << dbFile
		<< L"\nWriting temporal feature vectors to " << fvFile 
		<< L"\nWriting preview strings to " << previewFile;

	return make_shared<TemporalTrainingDataGenerator>(
			make_shared<TemporalDB>(dbFile), fvFile, previewFile);
}

