#include "Generic/common/leak_detection.h"

#pragma warning (disable: 4996)

#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp>
#include <boost/archive/text_wiarchive.hpp>
#include "mongoose/mongoose.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/ConsoleSessionLogger.h"
#include "Generic/common/UnexpectedInputException.h"
#include "ActiveLearning/MultilabelInferenceDataView.h"
#include "ActiveLearning/InstanceHashes.h"
#include "ActiveLearning/ActiveLearningUtilities.h"
#include "ActiveLearning/ActiveLearningData.h"
#include "ActiveLearning/StringStore.h"
#include "ActiveLearning/strategies/ActiveLearningStrategy.h"
#include "ActiveLearning/strategies/CoverageStrategy.h"
//#include "ActiveLearning/strategies/WeightedUncertainty.h"
#include "ActiveLearning/strategies/InstanceStrategy.h"
#include "ActiveLearning/objectives/RegularizationLoss.h"
#include "ActiveLearning/objectives/InstanceLoss.h"
#include "ActiveLearning/objectives/FeatureAnnotation.h"
//#include "ActiveLearning/objectives/PriorLoss.h"
#include "ActiveLearning/alphabet/MultiAlphabet.h"
#include "learnit/ProgramOptionsUtils.h"
#include "temporal/TemporalDB.h"
#include "temporal/TemporalInstanceSerialization.h"
#include "temporal/FeatureMap.h"
#include "TemporalTrainerUI.h"
#include "TemporalTrainer.h"

using std::wstring;
using std::string;
using std::wstringstream;
using std::wifstream;
using boost::archive::text_wiarchive;
using boost::make_shared;
using boost::dynamic_pointer_cast;

TemporalTrainerUI_ptr trainerInterface;

static const void* eventHandler(enum mg_event event,
   struct mg_connection* conn, const struct mg_request_info *request_info)
 {
	 return trainerInterface->event_handler(event, conn, request_info);
}

MultilabelInferenceDataView_ptr load_instances(MultiAlphabet_ptr alphabet,
	const string& fv_file, unsigned int nFeatures, InstanceHashes& instanceHashes) 
{
	TemporalInstanceData instData;
	int nInstances = 0;

	{ // controls scope of input stream, which we will need to 
		// reopen below
		wifstream inpStr(fv_file.c_str(), std::ios_base::binary);

		if (!inpStr.good()) {
			wstringstream err;
			err << L"Error opening feature vector file " << wstring(fv_file.begin(), fv_file.end());
			throw UnexpectedInputException("TemporalTrainerDriver.cpp:load_instances",
				err);
		}

		text_wiarchive in(inpStr);


		try {
			while (true) {
				in >> instData;
				++nInstances;
			}
			// catching the exception is ugly but it's the only way to know
			// we've reached the end of the file
		} catch (boost::archive::archive_exception const&) {}
	}

	MultilabelInferenceDataView_ptr data = 
		make_shared<MultilabelInferenceDataView>(nInstances, nFeatures);

	SessionLogger::info("load_instances") << "Loading " << nInstances
		<< " instances";

	wifstream inpStr(fv_file.c_str(), std::ios_base::binary);
	text_wiarchive in(inpStr);

	int i =0;
	std::set<unsigned int> instFV;
	bool got_duplicates = false;
	for (int i=0; i<nInstances; ++i) {
		in >> instData;
		instanceHashes.registerInstance(i, instData.hashValue());
		instFV.clear();
		BOOST_FOREACH(unsigned int feat, instData.fv()) {
			instFV.insert(alphabet->indexFromAlphabet(feat, instData.type()));
		}

		if (instFV.size() != instData.fv().size()) {
			got_duplicates = true;
		}
		data->observeFeaturesForInstance(i, instData.type(), instFV);
	}

	if (got_duplicates) {
		SessionLogger::warn("duplicate_features") << "Duplicate features "
			<< "found in some instances";
	}

	data->finishedLoading();

	SessionLogger::info("load_instances") << "Instance loading complete.";

	return data;
}

TemporalTrainerUI_ptr createTrainer(const string& dbFile,
   const string& featureVectorsFile, const string& previewStringsFile) 
{
	TemporalDB_ptr db = make_shared<TemporalDB>(dbFile);
	//FeatureMap_ptr featureMap = db->createFeatureMap();
	MultiAlphabet_ptr alphabet = db->featureAlphabet();
	//unsigned int nFeatures = featureMap->size();
	unsigned int nFeatures = alphabet->size();

	InstanceHashes_ptr instanceHashes = make_shared<InstanceHashes>();
	MultilabelInferenceDataView_ptr view = load_instances(alphabet, 
		featureVectorsFile, nFeatures, *instanceHashes);
	
	ActiveLearningData_ptr alData = make_shared<ActiveLearningData>(
		db->instanceAnnotationView(), dynamic_pointer_cast<FeatureAlphabet>(alphabet), 
		instanceHashes, dynamic_pointer_cast<DataView>(view));
	ActiveLearningStrategy_ptr alStrategy = make_shared<CoverageStrategy>(
		alData, dynamic_pointer_cast<DataView>(view), 
		dynamic_pointer_cast<FeatureAlphabet>(alphabet));
	InstanceStrategy_ptr instanceStrategy = make_shared<InstanceStrategy>(view);

	TemporalTrainer_ptr trainer = make_shared<TemporalTrainer>(db, alphabet, 
		view, true);
	
	std::pair<Eigen::VectorXd, Eigen::VectorXd> regularizationWeights =
		alphabet->featureRegularizationWeights();
	trainer->addObjectiveComponent(make_shared<RegularizationLoss>(trainer->parameters(),
		regularizationWeights.first, regularizationWeights.second,
		(double)ParamReader::getOptionalFloatParamWithDefaultValue(
								"temporal_regularization_loss_weight", 1.0)));
	trainer->addObjectiveComponent(make_shared<InstanceLoss>(view, alData,
		(double)ParamReader::getOptionalFloatParamWithDefaultValue(
								"temporal_instance_loss_weight", 1.0)));
	trainer->addObjectiveComponent(make_shared<FeatureAnnotationLoss>(
		view, alData, (double)ParamReader::getOptionalFloatParamWithDefaultValue(
								"temporal_feature_annotation_loss_weight", 1.0)));
	/*trainer->addObjectiveComponent(make_shared<PriorLoss>(view, 
		ParamReader::getRequiredFloatParam("relation_prior"),
		(double)ParamReader::getOptionalFloatParamWithDefaultValue(
									"temporal_prior_loss_weight", 1.0)));*/

        StringStore_ptr previewStrings = dynamic_pointer_cast<StringStore>(
                SimpleInMemoryStringStore::create(previewStringsFile, view->nInstances()));
	// TODO: add objective components

	return make_shared<TemporalTrainerUI>(trainer, alData, alStrategy,
		instanceStrategy, instanceHashes, previewStrings);
}

void parse_arguments(int argc, char** argv, string& paramFile,
		string& featureVectorsFile, string& dbFile, string& previewStringsFile)
{
	boost::program_options::options_description desc("Options");
	desc.add_options()
		("param-file,P", boost::program_options::value<string>(&paramFile))
		("db-file,T", boost::program_options::value<string>(&dbFile))
		("preview-file,P", boost::program_options::value<string>(&previewStringsFile))
		("fv-file,F", boost::program_options::value<string>(&featureVectorsFile));

	boost::program_options::positional_options_description pos;
	pos.add("param-file", 1).add("db-file", 1).add("fv-file", 1).add("preview-file",1);

	boost::program_options::variables_map var_map;

	try {
		boost::program_options::store(
			boost::program_options::command_line_parser(argc,argv).
			options(desc).positional(pos).run(), var_map);
	} catch (exception& exc) {
		cerr << "Comamnd-line parsing exception: " << exc.what();
		exit(1);
	}

	boost::program_options::notify(var_map);

	validate_mandatory_unique_cmd_line_arg(desc, var_map, "param-file");
	validate_mandatory_unique_cmd_line_arg(desc, var_map, "db-file");
	validate_mandatory_unique_cmd_line_arg(desc, var_map, "fv-file");
	validate_mandatory_unique_cmd_line_arg(desc, var_map, "preview-file");
}

int main(int argc, char** argv) {
#ifdef NDEBUG
    try {
#endif
		string paramFile;
		string dbFile;
		string featureVectorsFile;
		string previewStringsFile;

		parse_arguments(argc, argv, paramFile, featureVectorsFile, dbFile,
			previewStringsFile);
		ParamReader::readParamFile(paramFile);
		ParamReader::logParams();

		ConsoleSessionLogger logger(std::vector<wstring>(), L"[TempTrain]");
		SessionLogger::setGlobalLogger(&logger);
		// When the unsetter goes out of scope, due either to normal 
		// termination or an exception, it will unset the logger ptr for us.
		SessionLoggerUnsetter unsetter;


		trainerInterface = createTrainer(dbFile, featureVectorsFile,
			previewStringsFile);

		boost::thread optimizationThread((void (TemporalTrainer::*)
			(void))&TemporalTrainer::optimize, &*trainerInterface->trainer());

		if (!ActiveLearningUtilities::runActiveLearning(
			ParamReader::getRequiredParam("temporal_trainer_html_path"),
			optimizationThread, eventHandler))
		{
			return -1;
		}

		SessionLogger::info("sync_to_db") << "Syncing results to DB..."
			<< endl;
		trainerInterface->syncToDB();
#ifdef NDEBUG
	} catch (UnrecoverableException &e) {
		std::cerr << "\n" << e.getMessage() << std::endl;
		std::cerr << "Error Source: " << e.getSource() << std::endl;
		return -1;
	}
	catch (std::exception &e) {
		std::cerr << "Uncaught Exception: " << e.what() << std::endl;
		return -1;
	}
#endif

	return 0;
}
