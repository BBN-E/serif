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
#include <math.h>
#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>
#include <boost/thread/thread.hpp>

#include "mongoose/mongoose.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/ConsoleSessionLogger.h"

#include "ActiveLearning/ActiveLearningUtilities.h"
#include "ActiveLearning/InstanceAnnotationDB.h"
#include "ActiveLearning/DataView.h"
#include "ActiveLearning/InstanceHashes.h"
#include "ActiveLearning/ActiveLearningData.h"
#include "ActiveLearning/StringStore.h"
#include "ActiveLearning/strategies/CoverageStrategy.h"
#include "ActiveLearning/strategies/InstanceStrategy.h"
#include "ActiveLearning/objectives/RegularizationLoss.h"
#include "ActiveLearning/objectives/FeatureAnnotation.h"
#include "ActiveLearning/objectives/InstanceLoss.h"
#include "ActiveLearning/objectives/PriorLoss.h"
#include "ActiveLearning/alphabet/FromDBFeatureAlphabet.h"
#include "ActiveLearning/alphabet/MultiAlphabet.h"

#include "LearnIt/ProgramOptionsUtils.h"
#include "LearnIt/lbfgs/lbfgs.h"
#include "LearnIt/db/LearnIt2DB.h"
#include "LearnIt/LearnIt2.h"
#include "LearnIt/InstanceLoader.h"
#include "OptimizationView.h"
#include "trainer.h"
#include "LearnIt2TrainerUI.h"
#include "objectives/UnlabeledDisagreementLoss.h"
#include "objectives/OneSidedUnlabeledDisagreementLoss.h"


using std::cout; using std::cerr; using std::endl;
using std::string; using std::wstring; using std::stringstream;
using boost::regex; using boost::match_results; using boost::regex_search;
using boost::make_shared;
using boost::lexical_cast;
using boost::dynamic_pointer_cast;

void calibrateWeights(const std::string& dummyDB, 
									   const std::string& dummyStringsFile);

void usage() {
	SessionLogger::info("LEARNIT") << "LearnIt2Trainer.exe <param file> <feature_vectors_file> "
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

double weightForProb(double p) {
	return -log(1.0/p - 1.0);
}

LearnIt2TrainerUI_ptr createTrainer(const std::string& db_file,
									const std::string& feature_vectors_file,
									const std::string& preview_strings_file)
{
	LearnIt2DB_ptr db = make_shared<LearnIt2DB>(db_file, false);
	InstanceAnnotationDBView_ptr instAnnView = db->instanceAnnotationView();
	size_t n_threads = ParamReader::getOptionalIntParamWithDefaultValue(
			"inference_threads", 1);

	MultiAlphabet_ptr alphabet = MultiAlphabet::create(
		dynamic_pointer_cast<FeatureAlphabet>(db->getAlphabet(LearnIt2::SENTENCE_ALPHABET_INDEX)),
		dynamic_pointer_cast<FeatureAlphabet>(db->getAlphabet(LearnIt2::SLOT_ALPHABET_INDEX)));
			
	int nFeatures = alphabet->size();
//	int nInstances = peekAtNumInstances(feature_vectors_file);
	
	InstanceHashes_ptr instanceHashes = make_shared<InstanceHashes>();
	LoadInstancesReturn views = InstanceLoader::load_instances(alphabet, 
			feature_vectors_file, nFeatures, *instanceHashes);

	double prior = ParamReader::getRequiredFloatParam("relation_prior");
	double prior_weight = weightForProb(prior);
	unsigned int sentence_bias_feature = 
		alphabet->firstFeatureIndexByName(L"SentenceBiasFeature()");
	unsigned int slot_bias_feature = 
		alphabet->firstFeatureIndexByName(L"SlotBiasFeature()");

	OptimizationView_ptr slotView = 
		make_shared<OptimizationView>(views.slotView, 
		dynamic_pointer_cast<FeatureAlphabet>(alphabet), n_threads);
	slotView->fixFeature(slot_bias_feature, prior_weight);
	OptimizationView_ptr sentenceView =
		make_shared<OptimizationView>(views.sentenceView, 
		dynamic_pointer_cast<FeatureAlphabet>(alphabet), n_threads);
	sentenceView->fixFeature(sentence_bias_feature, prior_weight);

	SessionLogger::info("prior_weight") << "Set prior feature weight to " 
		<< prior_weight << " to acheive a prior of " << prior;

	DataView_ptr combinedView = views.combinedView;
	
	ActiveLearningData_ptr alData = make_shared<ActiveLearningData>(
		instAnnView, dynamic_pointer_cast<FeatureAlphabet>(alphabet),
		instanceHashes, combinedView);
	ActiveLearningStrategy_ptr alStrategy = make_shared<CoverageStrategy>(
		alData, combinedView, dynamic_pointer_cast<FeatureAlphabet>(alphabet));
	InstanceStrategy_ptr instanceStrategy = make_shared<InstanceStrategy>(combinedView);

	std::pair<Eigen::VectorXd, Eigen::VectorXd> regularizationWeights =
		alphabet->featureRegularizationWeights();
	slotView->addObjectiveComponent(make_shared<RegularizationLoss>(
		slotView->parameters(), regularizationWeights.first,
		regularizationWeights.second, 
		(double)ParamReader::getOptionalFloatParamWithDefaultValue("learnit2_regularization_loss_weight", 1.0f)));
	slotView->addObjectiveComponent(make_shared<InstanceLoss>(
		slotView->data(), alData,
		(double)ParamReader::getOptionalFloatParamWithDefaultValue("learnit2_instance_loss_weight", 1.0)));
	slotView->addObjectiveComponent(make_shared<FeatureAnnotationLoss>(
		slotView->data(), alData,
		(double)ParamReader::getOptionalFloatParamWithDefaultValue("learnit2_feature_annotation_loss_weight", 1.0)));
	slotView->addObjectiveComponent(make_shared<PriorLoss>(slotView->data(), 
		ParamReader::getRequiredFloatParam("relation_prior"),
		(double)ParamReader::getOptionalFloatParamWithDefaultValue("learnit2_prior_loss_weight", 1.0)));
	slotView->addObjectiveComponent(make_shared<OneSidedUnlabeledDisagreementLoss>(
		slotView->data(), sentenceView->data(), alData,
		(double)ParamReader::getOptionalFloatParamWithDefaultValue("learnit2_disagreement_loss_weight_with_annotation", 1.0),
		(double)ParamReader::getOptionalFloatParamWithDefaultValue("learnit2_disagreement_loss_weight_no_annotation", 1.0)));

	sentenceView->addObjectiveComponent(make_shared<RegularizationLoss>(
		sentenceView->parameters(), regularizationWeights.first,
		regularizationWeights.second, 
		(double)ParamReader::getOptionalFloatParamWithDefaultValue("learnit2_regularization_loss_weight", 1.0f)));
	sentenceView->addObjectiveComponent(make_shared<InstanceLoss>(
		sentenceView->data(), alData,
		(double)ParamReader::getOptionalFloatParamWithDefaultValue("learnit2_instance_loss_weight", 1.0)));
	sentenceView->addObjectiveComponent(make_shared<FeatureAnnotationLoss>(
		sentenceView->data(), alData,
		(double)ParamReader::getOptionalFloatParamWithDefaultValue("learnit2_feature_annotation_loss_weight", 1.0)));
	sentenceView->addObjectiveComponent(make_shared<PriorLoss>(sentenceView->data(), 
		prior,
		(double)ParamReader::getOptionalFloatParamWithDefaultValue("learnit2_prior_loss_weight", 1.0)));
	sentenceView->addObjectiveComponent(make_shared<OneSidedUnlabeledDisagreementLoss>(
		sentenceView->data(), slotView->data(), alData,
		(double)ParamReader::getOptionalFloatParamWithDefaultValue("learnit2_disagreement_loss_weight_with_annotation", 1.0),
		(double)ParamReader::getOptionalFloatParamWithDefaultValue("learnit2_disagreement_loss_weight_no_annotation", 1.0)));

	LearnIt2Trainer_ptr trainer = make_shared<LearnIt2Trainer>(db, 
		dynamic_pointer_cast<FeatureAlphabet>(alphabet), 
		slotView, sentenceView, views.slotView->nInstances(), true);

    int nInstances = combinedView->nInstances();
    StringStore_ptr previewStrings = dynamic_pointer_cast<StringStore>(
		SimpleInMemoryStringStore::create(preview_strings_file, nInstances));

	return make_shared<LearnIt2TrainerUI>(trainer, alData, alStrategy, 
		instanceStrategy, instanceHashes, previewStrings);
}

// this is needed as a workaround since we can't use a member function
// directly as a callback
LearnIt2TrainerUI_ptr trainerInterface;

static const void* event_handler(enum mg_event event,
struct mg_connection* conn, const struct mg_request_info *request_info)
{
	return trainerInterface->event_handler(event, conn, request_info);
}

int main(int argc, char** argv) {
	try {
		string db_file;
		string feature_vectors_file;
		string preview_strings_file;

		parse_arguments(argc, argv, feature_vectors_file, preview_strings_file,
			db_file);
		ConsoleSessionLogger logger(std::vector<wstring>(), L"[LIt2T]");
		SessionLogger::setGlobalLogger(&logger);
		// When the unsetter goes out of scope, due either to normal termination or an exception, it will unset the logger ptr for us.
		SessionLoggerUnsetter unsetter;

		if (ParamReader::isParamTrue("learnit2_calibrate_weights")) {
			calibrateWeights(db_file, preview_strings_file);
			return 0;
		}

		const string tmp_db_file = tmpnam(NULL);
		SessionLogger::info("db_copy") << "Copying input database file " << db_file 
			<< " to local temp file " << tmp_db_file << std::endl;
		boost::filesystem::copy_file(db_file, tmp_db_file);

		trainerInterface = createTrainer(tmp_db_file, feature_vectors_file,
			preview_strings_file);
		boost::thread optimizationThread((void (LearnIt2Trainer::*)(void))&LearnIt2Trainer::optimize, &*trainerInterface->trainer());

		if (!ActiveLearningUtilities::runActiveLearning(
			ParamReader::getParam("learnit2_trainer_html_path"),
			optimizationThread, event_handler)) 
		{
			return -1;
		}

		// wait for learning to complete
		SessionLogger::info("sync_to_db") << "Syncing results to DB..." << endl;
		trainerInterface->syncToDB();
		trainerInterface = LearnIt2TrainerUI_ptr(); // force destruction of containing database object

		std::string backup_file = db_file + ".backup";
		SessionLogger::info("db_copy") << "Backing up original database " << db_file 
			<< " to " << backup_file;
		boost::filesystem::remove(backup_file);
		boost::filesystem::copy_file(db_file, backup_file);
		boost::filesystem::remove(db_file);
		SessionLogger::info("db_copy") << "Copying local temp database file " << tmp_db_file 
			<< " back to " << db_file << std::endl;
		boost::filesystem::copy_file(tmp_db_file, db_file);
		boost::filesystem::remove(tmp_db_file);
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


// this creates a simple sample problem to calibrate the weights of the
// different loss functions.
// It has one instance i which is the source of all information. For sentence features,
// it has the sentence bias feature and one feature g annotated at 85% expectation.
// For slot features, it has the prior feature and a single feature h.
// There are ten more instances which all have the same slot features as i.
// However, they all have (in addition to the sentence bias feature) a distinct
// sentence feature apiece. There is a further layer of 10 instances, one for 
// each in the middle layer, which have once apiece the sentence features along
// with distinct slot features. We expect the slot predictions on this to be around 0.4.
// Finally, to set the prior, we have 90 instances which
// each have the two priors and one distinct slot and sentence feature apiece.
// The goal is to have p_sent(i) > 80% and p_sent(x) > 40% for all instances x
// with slot feature g, while maintaining p_sent(y) < 12% for all other instance
// y.
void calibrateWeights(const std::string& dummyDB, const std::string& dummyStringsFile) {
	throw UnrecoverableException("LearnIt2Trainer.cpp:calibrateWeights",
		"Need to update for alternating optimization.");

	//LearnIt2DB_ptr db = make_shared<LearnIt2DB>(dummyDB, false);
	//FromDBFeatureAlphabet_ptr alphabet = db->getAlphabet(0);
	//InstanceHashes_ptr instanceHashes = make_shared<InstanceHashes>();
	//int nInstances = 111;
	//int nFeatures = alphabet->size();
	//InferenceDataView_ptr slotView =
	//	make_shared<InferenceDataView>(nInstances, nFeatures);
	//InferenceDataView_ptr sentenceView = 
	//	make_shared<InferenceDataView>(nInstances, nFeatures);
	//InferenceDataView_ptr combinedView =
	//	make_shared<InferenceDataView>(nInstances, nFeatures);

	//int slotBias = 0;
	//int sentBias = 1;
	//int labeledSentFeature = 2;
	//int transmittingSlotFeature = 3;
	//std::set<unsigned int> featuresInInstance;

	//for (int i=0; i<nInstances; ++i) {
	//	instanceHashes->registerInstance(i, lexical_cast<string>(i));
	//}

	//// create our only (indirectly) labeled instance
	//featuresInInstance.insert(0);
	//featuresInInstance.insert(3);
	//slotView->observeFeaturesForInstance(0, featuresInInstance);
	//combinedView->observeFeaturesForInstance(0, featuresInInstance);
	//featuresInInstance.clear();
	//featuresInInstance.insert(1);
	//featuresInInstance.insert(2);
	//sentenceView->observeFeaturesForInstance(0, featuresInInstance);
	//combinedView->observeFeaturesForInstance(0, featuresInInstance);

	//// make the ten instances we want to propogate information to from the
	//// labeled instance
	//int nextFeature = 4;
	//int nextInstance = 1;
	//std::vector<int> middle_layer_sentence_features;
	//std::vector<int> middle_layer_instances;
	//for (int i=0; i<10; ++i) {
	//	int inst = nextInstance++;
	//	featuresInInstance.clear();
	//	featuresInInstance.insert(slotBias);
	//	featuresInInstance.insert(transmittingSlotFeature);
	//	slotView->observeFeaturesForInstance(inst, featuresInInstance);
	//	combinedView->observeFeaturesForInstance(inst, featuresInInstance);
	//	featuresInInstance.clear();
	//	featuresInInstance.insert(sentBias);
	//	int feat = nextFeature++;
	//	featuresInInstance.insert(feat);
	//	middle_layer_sentence_features.push_back(feat);
	//	sentenceView->observeFeaturesForInstance(inst, featuresInInstance);
	//	combinedView->observeFeaturesForInstance(inst, featuresInInstance);
	//	middle_layer_instances.push_back(inst);
	//}

	//std::vector<int> bottom_layer_instances;
	//// the second layer of propagated information
	//for (int i=0; i<10; ++i) {
	//	int inst = nextInstance++;
	//	featuresInInstance.clear();
	//	featuresInInstance.insert(sentBias);
	//	featuresInInstance.insert(middle_layer_sentence_features[i]);
	//	sentenceView->observeFeaturesForInstance(inst, featuresInInstance);
	//	combinedView->observeFeaturesForInstance(inst, featuresInInstance);
	//	featuresInInstance.clear();
	//	featuresInInstance.insert(slotBias);
	//	featuresInInstance.insert(nextFeature++);
	//	slotView->observeFeaturesForInstance(inst, featuresInInstance);
	//	combinedView->observeFeaturesForInstance(inst, featuresInInstance);
	//	bottom_layer_instances.push_back(inst);
	//}

	//// these are a bunch of dummies just to make the whole distribution
	//// approximately match the prior
	//std::vector<int> prior_tuning_instances;
	//for (int i=0; i<90; ++i) {
	//	int inst = nextInstance++;
	//	featuresInInstance.clear();
	//	featuresInInstance.insert(slotBias);
	//	featuresInInstance.insert(nextFeature++);
	//	slotView->observeFeaturesForInstance(inst, featuresInInstance);
	//	combinedView->observeFeaturesForInstance(inst, featuresInInstance);
	//	featuresInInstance.clear();
	//	featuresInInstance.insert(sentBias);
	//	featuresInInstance.insert(nextFeature++);
	//	sentenceView->observeFeaturesForInstance(inst, featuresInInstance);
	//	combinedView->observeFeaturesForInstance(inst, featuresInInstance);
	//	prior_tuning_instances.push_back(inst);
	//}

	//ActiveLearningData_ptr alData = make_shared<ActiveLearningData>(
	//	db->instanceAnnotationView(), 
	//	dynamic_pointer_cast<FeatureAlphabet>(alphabet),
	//	instanceHashes, dynamic_pointer_cast<DataView>(combinedView));
	//alData->_test_clear_annotations();
	//alData->registerAnnotation(2, "positive");
	//alData->updateBetweenIterations();
	//Eigen::VectorXd regWeights(nFeatures);
	//for (int i=0; i<nextFeature; ++i) {
	//	regWeights.coeffRef(i) = 1.0;
	//}
	//for (int i=nextFeature; i<nFeatures; ++i) {
	//	regWeights.coeffRef(i) = 1.0;
	//}

	//if (ParamReader::isParamTrue("learnit2_calibration_exponential_ballpark_search")) {
	//	typedef std::pair<double, std::string> Result;
	//	vector<Result> results;
	//	
	//	LearnIt2Trainer_ptr trainer = make_shared<LearnIt2Trainer>(db, 
	//		dynamic_pointer_cast<FeatureAlphabet>(alphabet),
	//		slotView, sentenceView, slotView->nInstances(), false, false);

	//	bool l1 = false;
	//	double inst_weight = 1.0;
	//	double prior_weight = 1.0;
	//	//double reg_weight = 1.0;
	//	for (double reg_weight = 0.01; reg_weight < 1.1; reg_weight *= 10.0) {
	//		for (double diss_ann_weight = 50.0; diss_ann_weight < 200; diss_ann_weight += 50.0) {
	//			for (double fan_weight = 400.0; fan_weight < 1001.0; fan_weight +=100.0) {
	//				for (double diss_not_weight = 1.0; diss_not_weight < 50.0; diss_not_weight += 10.0) {
	//					/*reg_weight = 1.0;
	//					diss_ann_weight = 100.0;
	//					fan_weight = 1000.0;
	//					diss_not_weight = 10.0;*/
	//					/*fan_weight = 1000.0;
	//					diss_ann_weight = 200.0;
	//					diss_not_weight = 50.0;*/

	//					trainer->_test_clear_params();
	//					trainer->_test_clear_objective_components();

	//					trainer->addObjectiveComponent(make_shared<RegularizationLoss>(
	//						trainer->parameters(), regWeights, 
	//						reg_weight));
	//					trainer->addObjectiveComponent(make_shared<InstanceLoss>(sentenceView,
	//						alData, inst_weight));
	//					trainer->addObjectiveComponent(make_shared<InstanceLoss>(slotView, 
	//						alData, inst_weight));
	//					if (l1) {
	//						trainer->addObjectiveComponent(make_shared<FeatureAnnotationLoss>(
	//							slotView, alData, fan_weight, FeatureAnnotationLoss::L1));
	//						trainer->addObjectiveComponent(make_shared<FeatureAnnotationLoss>(
	//							sentenceView, alData, fan_weight, FeatureAnnotationLoss::L1));
	//						trainer->addObjectiveComponent(make_shared<UnlabeledDisagreementLoss>(
	//							slotView, sentenceView, alData, diss_ann_weight, diss_not_weight,
	//							UnlabeledDisagreementLoss::L1));
	//						//if (ParamReader::isParamTrue("learnit2_prior_in_objective") {
	//						trainer->addObjectiveComponent(make_shared<PriorLoss>(slotView, 
	//							ParamReader::getRequiredFloatParam("relation_prior"),
	//							prior_weight, PriorLoss::L1));
	//						trainer->addObjectiveComponent(make_shared<PriorLoss>(sentenceView, 
	//							ParamReader::getRequiredFloatParam("relation_prior"),
	//							prior_weight, PriorLoss::L1));
	//					} else {
	//						trainer->addObjectiveComponent(make_shared<FeatureAnnotationLoss>(
	//							slotView, alData, fan_weight, FeatureAnnotationLoss::KL));
	//						trainer->addObjectiveComponent(make_shared<FeatureAnnotationLoss>(
	//							sentenceView, alData, fan_weight, FeatureAnnotationLoss::KL));
	//						trainer->addObjectiveComponent(make_shared<UnlabeledDisagreementLoss>(
	//							slotView, sentenceView, alData, diss_ann_weight, diss_not_weight,
	//							UnlabeledDisagreementLoss::KL));
	//						//if (ParamReader::isParamTrue("learnit2_prior_in_objective") {
	//						trainer->addObjectiveComponent(make_shared<PriorLoss>(slotView, 
	//							ParamReader::getRequiredFloatParam("relation_prior"),
	//							prior_weight, PriorLoss::KL));
	//						trainer->addObjectiveComponent(make_shared<PriorLoss>(sentenceView, 
	//							ParamReader::getRequiredFloatParam("relation_prior"),
	//							prior_weight, PriorLoss::KL));
	//					}

	//					trainer->optimize(50);

	//					// we score a set of parameters by the the squared differences
	//					// between the predictions of the following probabilities
	//					// and what we want them to be
	//					// feature-labeled instance, sentence prob: >= 80
	//					// feature-labeled instance, slot prob: >= 75
	//					// 'transmitted' instances, sent_prob: >= 40
	//					// prior instance, slot_prob: <= 12
	//					// prior instance, sent_prob: <= 12

	//					// feature labeled instance is zero
	//					double loss = 0.0;
	//					// feature-labeled instance, sentence prob: >= 80
	//					double pred = sentenceView->prediction(0);
	//					double fl_sent_pred = pred;
	//					if (pred < 0.8) {
	//						loss += abs((0.8 - pred)) /** (0.8 - pred)*/;
	//					}
	//					// feature-labeled instance, slot prob: should be around .70
	//					// we multiply by 11 because this is also the slot prob for 
	//					// all the transmitted instances
	//					pred = slotView->prediction(0);
	//					double fl_slot_pred = pred;
	//					loss += abs(11.0*(pred - .70)) /** (pred-.70)*/;

	//					// 'transmitted' instances, sent prob, around .55
	//					double trans_sent_pred = 0.0;
	//					BOOST_FOREACH(int inst, middle_layer_instances) {
	//						pred = sentenceView->prediction(inst);
	//						trans_sent_pred += pred/20.0;
	//						loss += abs((pred - 0.55)) /** (pred - 0.55)*/;
	//					}

	//					double trans_slot_pred = 0.0;
	//					BOOST_FOREACH(int inst, bottom_layer_instances) {
	//						pred = sentenceView->prediction(inst);
	//						trans_sent_pred += pred/20.0;
	//						loss += (pred - 0.55) * (pred - 0.55);
	//						pred = slotView->prediction(inst);
	//						trans_slot_pred= slotView->prediction(inst);
	//						loss += abs((pred - 0.3)) /** (pred - 0.3)*/;
	//					}

	//					// prior instances; slot and sent probs <= 12, >= .08
	//					double prior_sent_pred = 0.0;
	//					double prior_slot_pred = 0.0;
	//					BOOST_FOREACH(int inst, prior_tuning_instances) {
	//						pred = sentenceView->prediction(inst);
	//						prior_sent_pred += pred/90.0;
	//						loss += (pred-.1)*(pred-.1);
	//						pred = slotView->prediction(inst);
	//						prior_slot_pred += pred/90.0;
	//						loss += abs((pred - .1)) /** (pred - .1)*/ ;
	//					}

	//					stringstream str;
	//					str << fixed;
	//					str.precision(2);
	//					str << "loss=" << loss << "; reg=" << reg_weight << "; fan="
	//						<< fan_weight << "; disa=" << diss_ann_weight << "; disn=" 
	//						<< diss_not_weight << endl << "\tp_st(fl): "
	//						<< fl_sent_pred << "; p_sl(fl): " << fl_slot_pred <<
	//						"; trans t: " << trans_sent_pred << "; trans l: "
	//						<< trans_slot_pred << "; pr: "
	//						<< prior_sent_pred << "; pr_sl: " << prior_slot_pred;
	//					results.push_back(make_pair(loss, str.str()));
	//				}
	//			}
	//		}
	//	}
	//	sort(results.begin(), results.end());

	//	BOOST_FOREACH(const Result& result, results) {
	//		cout << result.second << endl;
	//	}
	//}
}
