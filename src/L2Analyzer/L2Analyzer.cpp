#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
// Suppress a Visual Studio warning that using the standard library routine
// tmpnam exposes you to certain security risks.  
#pragma warning( disable: 4996 )
#include <iostream>
#include <string>
#ifndef WIN32
#include <unistd.h>
#endif
#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp>

#include "Generic/common/ParamReader.h"
#include "Generic/common/ConsoleSessionLogger.h"

#include "ActiveLearning/ActiveLearningUtilities.h"
#include "ActiveLearning/InstanceAnnotationDB.h"
#include "ActiveLearning/DataView.h"
#include "ActiveLearning/InstanceHashes.h"
#include "ActiveLearning/ActiveLearningData.h"
#include "ActiveLearning/StringStore.h"
#include "ActiveLearning/alphabet/FromDBFeatureAlphabet.h"
#include "ActiveLearning/alphabet/MultiAlphabet.h"

#include "learnit/db/LearnIt2DB.h"
#include "learnit/ProgramOptionsUtils.h"
#include "learnit/LearnIt2.h"
#include "learnit/InstanceLoader.h"


using std::cout; using std::cerr; using std::endl;
using std::wcout;
using std::string; using std::wstring; using std::stringstream;
using std::vector;
using Eigen::SparseVector;
using Eigen::VectorXd;
using boost::make_shared;
using boost::lexical_cast;
using boost::dynamic_pointer_cast;

void usage() {
	SessionLogger::info("LEARNIT") << "L2Analyzer <param file> <feature_vectors_file> "
		<< "<input_db_file> <output_db_file>" << endl;
}

void parse_arguments(int argc, char ** argv, string& feature_vectors_file, 
  string& preview_strings_file,  string& db_file)
{
	using namespace boost::program_options;
	if (argc < 5) {
		usage();
	}

	string param_file;
	options_description desc("Options");
	desc.add_options()
		("param-file,P", value<string>(&param_file),"[required] parameter file")
		("fv,F", value<string>(&feature_vectors_file), "[required] file of input feature vectors")
		("strings,s", value<string>(&preview_strings_file), "[required] file of preview strings")
		("db-file,o", value<string>(&db_file), "[required] database");

	positional_options_description pos;
	pos.add("param-file", 1).add("fv", 1).add("strings", 1).add("db-file", 1);

	variables_map var_map;
	try {
		store(command_line_parser(argc, argv).options(desc).positional(pos).run(),
				var_map);
	} catch (exception& exc) {
		cerr << "Command-line parsing exception: " << exc.what() << endl;
	}

	notify(var_map);

	validate_mandatory_unique_cmd_line_arg(desc, var_map, "param-file");
	validate_mandatory_unique_cmd_line_arg(desc, var_map, "fv");
	validate_mandatory_unique_cmd_line_arg(desc, var_map, "db-file");
	validate_mandatory_unique_cmd_line_arg(desc, var_map, "strings");

	ParamReader::readParamFile(param_file);
}

vector<int> instancesContaining(DataView_ptr data, int feat) {
	vector<int> ret;
	for (size_t i=0; i<data->nInstances(); ++i) {
		SparseVector<double>::InnerIterator it(data->features(i));
		for (; it; ++it) {
			if (it.index() == feat) {
				ret.push_back(i);
			}
		}
	}
	return ret;
}

void debugFeature(int feat, bool sent_feat, FeatureAlphabet_ptr alphabet, 
		StringStore_ptr previewStrings, const LoadInstancesReturn& views, 
		const VectorXd& weights)
{
	wcout << L"Debugging feature " << feat << L": " 
		<< alphabet->getFeatureName(feat) << endl;

	BOOST_FOREACH(int inst, instancesContaining(sent_feat?views.sentenceView:views.slotView, feat)) {
		wcout << L"\tInstance " << inst << L": ";
		wcout << previewStrings->getString(inst) << endl;
		wcout << L"\tSlot prob: "  << views.slotView->prediction(inst)
			<< L"\tSentence prob: " << views.sentenceView->prediction(inst) << endl;
		wcout << L"\tSentence features: " << endl;
		SparseVector<double>::InnerIterator it(views.sentenceView->features(inst));
		for (; it; ++it) {
			wcout << L"\t\t"  << alphabet->getFeatureName(it.index()) 
				 << L"(" << it.index() << L") = " << weights(it.index()) << endl;
		}
		wcout << L"\tSlot features: " << endl;
		SparseVector<double>::InnerIterator jt(views.slotView->features(inst));
		for (; jt; ++jt) {
			wcout << L"\t\t" << alphabet->getFeatureName(jt.index())
				<< L"(" << jt.index() << L") = " << weights(jt.index()) << endl;
		}
		cout << endl << endl;
	}
}

void run(const std::string& db_file,
									const std::string& feature_vectors_file,
									const std::string& preview_strings_file)
{
	LearnIt2DB_ptr db = make_shared<LearnIt2DB>(db_file, false);
	InstanceAnnotationDBView_ptr instAnnView = db->instanceAnnotationView();

	FeatureAlphabet_ptr sentenceAlphabet = db->getAlphabet(LearnIt2::SENTENCE_ALPHABET_INDEX);
	FeatureAlphabet_ptr slotAlphabet = db->getAlphabet(LearnIt2::SLOT_ALPHABET_INDEX);
	MultiAlphabet_ptr alphabet = MultiAlphabet::create( sentenceAlphabet, slotAlphabet);
			
	int nFeatures = alphabet->size();
//	int nInstances = peekAtNumInstances(feature_vectors_file);
	
	InstanceHashes_ptr instanceHashes = make_shared<InstanceHashes>();
	LoadInstancesReturn views = InstanceLoader::load_instances(alphabet, 
			feature_vectors_file, nFeatures, *instanceHashes);
	DataView_ptr combinedView = views.combinedView;
	
	ActiveLearningData_ptr alData = make_shared<ActiveLearningData>(
		instAnnView, dynamic_pointer_cast<FeatureAlphabet>(alphabet),
		instanceHashes, combinedView);
	/*ActiveLearningStrategy_ptr alStrategy = make_shared<CoverageStrategy>(
		alData, combinedView, dynamic_pointer_cast<FeatureAlphabet>(alphabet));
	InstanceStrategy_ptr instanceStrategy = make_shared<InstanceStrategy>(combinedView);*/

    int nInstances = combinedView->nInstances();
    StringStore_ptr previewStrings = dynamic_pointer_cast<StringStore>(
		SimpleInMemoryStringStore::create(preview_strings_file, nInstances));

	Eigen::VectorXd weights = alphabet->getWeights();
	views.slotView->inference(weights);
	views.sentenceView->inference(weights);
	double slot_total = 0.0, sentence_total = 0.0;

	for (size_t i=0; i<views.sentenceView->nInstances(); ++i) {
		sentence_total += views.sentenceView->prediction(i);
		slot_total += views.slotView->prediction(i);
	}

	SessionLogger::info("foo") << "Averages are " 
		<< sentence_total/views.sentenceView->nInstances() << ", "
		<< slot_total/views.slotView->nInstances();

	std::cout << "Sentence alphabet size is " << sentenceAlphabet->size() << std::endl;

	int feat;
	std::cout << "> ";
	while (cin >> feat) {
		debugFeature(feat, feat<sentenceAlphabet->size(), alphabet, previewStrings, 
				views, weights);
		std::cout << "> ";
	}
}

int main(int argc, char** argv) {
	try {
		string db_file;
		string feature_vectors_file;
		string preview_strings_file;

		parse_arguments(argc, argv, feature_vectors_file, preview_strings_file,
			db_file);
		ConsoleSessionLogger logger(std::vector<wstring>(), L"[L2Analyzer]");
		SessionLogger::setGlobalLogger(&logger);
		// When the unsetter goes out of scope, due either to normal termination or an exception, it will unset the logger ptr for us.
		SessionLoggerUnsetter unsetter;

		run(db_file, feature_vectors_file, preview_strings_file);
	} catch (UnrecoverableException &e) {
		std::cerr << "\n" << e.getMessage() << std::endl;
		std::cerr << "Error Source: " << e.getSource() << std::endl;
		return -1;
	}
	catch (std::exception &e) {
		std::cerr << "Uncaught Exception: " << e.what() << std::endl;
		return -1;
	}

	return 0;
}
