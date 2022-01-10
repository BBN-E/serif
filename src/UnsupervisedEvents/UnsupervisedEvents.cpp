#include "Generic/common/leak_detection.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <cstdio>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/ConsoleSessionLogger.h"
#include "Generic/common/FeatureModule.h"
#include "Generic/common/UnicodeUtil.h"
#include "English/EnglishModule.h"
/*#include "LearnIt/MainUtilities.h"
#include "LearnIt/util/FileUtilities.h"*/
#include "ProblemDefinition.h"
#include "ACEPREMDecoder.h"
#include "ACEScorer.h"
#include "SerifDecoder.h"
#include "BaselineDecoder.h"
#include "OracleDecoder.h"
#include "AnswerExtractor.h"
#include "GuidedDecoder.h"

using std::endl;
using std::string;
using std::wstring;
using std::vector;
using boost::to_lower;

void aceMode() {
	string probFile = ParamReader::getRequiredParam("problem_definition");
	ProblemDefinition_ptr problem = ProblemDefinition::load(probFile);
	std::string PRTrainingDocTable = ParamReader::getRequiredParam(
			"pr_training_doc_table");
	std::string testDocTable = ParamReader::getRequiredParam(
			"test_doc_table");
	std::string outputDirectory = ParamReader::getRequiredParam(
			"output_directory");
	if (!boost::filesystem::is_directory(outputDirectory)) {
		boost::filesystem::create_directory(outputDirectory);
	}

	ACEPREMDecoder_ptr premDecoder = boost::make_shared<ACEPREMDecoder>(problem, 
			PRTrainingDocTable, testDocTable);
	premDecoder->train();

	std::stringstream scoreOutput;
	scoreOutput << outputDirectory << "/final_scoring";
	SessionLogger::info("final_scores") 
		<<  "Writing finals scores to " << scoreOutput.str();

	DecoderLists decoders(3, DecoderList());

	decoders[0].push_back(boost::make_shared<SerifDecoder>(problem));

	decoders[1].push_back(boost::make_shared<BaselineDecoder>(problem, false,
				BaselineDecoder::NOT_AGGRESSIVE, false));
	decoders[1].push_back(boost::make_shared<BaselineDecoder>(problem, false,
				BaselineDecoder::H_ONLY, false));
	decoders[1].push_back(boost::make_shared<BaselineDecoder>(problem, false,
				BaselineDecoder::H_AND_M_ONLY, false));
	decoders[1].push_back(boost::make_shared<BaselineDecoder>(problem, false,
				BaselineDecoder::H_ONLY, true));
	decoders[1].push_back(boost::make_shared<BaselineDecoder>(problem, false,
				BaselineDecoder::H_AND_M_ONLY, true));

	decoders[1].push_back(boost::make_shared<BaselineDecoder>(problem, true,
				BaselineDecoder::NOT_AGGRESSIVE, false));
	decoders[1].push_back(boost::make_shared<BaselineDecoder>(problem, true,
				BaselineDecoder::H_ONLY, false));
	decoders[1].push_back(boost::make_shared<BaselineDecoder>(problem, true,
				BaselineDecoder::H_AND_M_ONLY, false));
	decoders[1].push_back(boost::make_shared<BaselineDecoder>(problem, true,
				BaselineDecoder::H_ONLY, true));
	decoders[1].push_back(boost::make_shared<BaselineDecoder>(problem, true,
				BaselineDecoder::H_AND_M_ONLY, true));

	/*decoders[2].push_back(boost::make_shared<GoldToSerifOracleDecoder>());
	decoders[3].push_back(boost::make_shared<SerifToGoldOracleDecoder>());*/
	decoders[2].push_back(boost::make_shared<GuidedDecoder>(premDecoder));
	/*
	for (double threshold = 0.05; threshold<1.0; threshold+=0.05) {
		decoders[4].push_back(boost::make_shared<BestBuyerThemeSellerByThreshold>(
					premDecoder, threshold));
		decoders[5].push_back(boost::make_shared<MaxVarThreshold>(
					premDecoder, threshold));
		decoders[6].push_back(boost::make_shared<MaxSentenceThreshold>(
					premDecoder, threshold));
	}*/

	ACEScorer::dumpScore(problem, decoders, scoreOutput.str());
}

int main(int argc, char** argv) {
	ConsoleSessionLogger logger;
	SessionLogger::setGlobalLogger(&logger);
	SessionLoggerUnsetter slu;
#ifdef NDEBUG
	try {
#endif
		bool debug_mode = false;
		if (argc != 2) {
			if (argc == 3 && strcmp(argv[2], "debug") == 0) {
				debug_mode = true;
			} else {
				SessionLogger::err("bad_arguments") 
					<< L"UnsupervisedEvents requires exactly one argument, a parameter file" << endl;
				exit(1);
			}
		}
		ParamReader::readParamFile(argv[1]);
		if (debug_mode) {
			ParamReader::setParam("event_debug_mode", "true");
		}
		FeatureModule::load();

		if (ParamReader::getRequiredParam("event_style") == "ace") {
			aceMode();
		} else {
			throw UnexpectedInputException("main", 
					"Only ACE mode supported currently.");
		}


		return 0;
#ifdef NDEBUG
    // If something broke, then say what it was.
    } catch (UnrecoverableException &e) {
		SessionLogger::err("unrecoverable_exception") <<
         "\n" << e.getMessage() <<  "; Error Source: " << e.getSource();
		return 1;
    } catch (std::exception &e) {
		SessionLogger::err("std::exception") << "Uncaught std::exception: " 
			<< e.what();
		return 2;
    } catch (...) {
		SessionLogger::err("unidentifiable exception") << "Uncaught unidentifiable exception";
		return 3;
    }
#endif
}

/*
typedef ConstraintsType<Passage>::type PassageConstraints;
typedef ConstraintsType<Passage>::ptr PassageConstraints_ptr;
typedef InstanceConstraintsType<Passage>::type PassageInstanceConstraints;
typedef InstanceConstraintsType<Passage>::ptr PassageInstanceConstraints_ptr;

void loadPassages(DataSet<Passage>& passages, unsigned int num_relation_types)
{
		string distillDocTableFile = 
			ParamReader::getRequiredParam("distill_doc_table");
		std::vector<DocIDName> w_doc_files = FileUtilities::readDocumentList(distillDocTableFile);
		PassageDescriptions_ptr passageDescriptions = 
			PassageDescription::loadPassageDescriptions(
					ParamReader::getRequiredParam("passage_list"));


		SessionLogger::info("docs_to_load") << "Loading " << w_doc_files.size()
			<< " documents";

		BOOST_FOREACH(DocIDName w_doc_file, w_doc_files) {
			// Load the SerifXML and get the DocTheory
			std::pair<Document*, DocTheory*> doc_pair = SerifXML::XMLSerializedDocTheory(w_doc_file.second.c_str()).generateDocTheory();
			if (!doc_pair.second)
				continue;

			wstring docid = doc_pair.first->getName().to_string();

			PassageDescriptions::const_iterator probe =
				passageDescriptions->find(docid);

			if (probe!=passageDescriptions->end()) {
				probe->second->attachDocTheory(doc_pair.second);
				SessionLogger::info("got_doc") << L"Using " << probe->first
					<< L" over range " << probe->second->startSentence()
					<< L", " << probe->second->endSentence();
				passages.addGraph(Passage::create(probe->second, 
						doc_pair.second, num_relation_types));
			} else {
				SessionLogger::info("no_doc") << L"Skipping " << docid;
			}

			// Clean up
			delete doc_pair.second;
		}
}

void createConstraints(const GraphicalModel::DataSet<Passage>& data,
		const PassageConstraints_ptr& constraints,
		const ProblemDefinition& problem) 
{
	typedef FeatureImpliesClassConstraint<RelationDocWordsConstraintView> FICC;

	std::vector<wstring> classParts;
	std::vector<wstring> featureParts;
	std::vector<wstring> lineParts;
	std::vector<unsigned int> classIDs;

	BOOST_FOREACH(const ConstraintDescription& desc, problem.constraintDescriptions()) {
		if (L"HeadWordImpliesClass" == desc.constraintType) {
			lineParts.clear();
			classParts.clear();
			featureParts.clear();

			split(lineParts, desc.constraintData, is_any_of(L"\t"));
			if (lineParts.size() != 2) {
				wstringstream err;
				err << L"String needed 2 parts, but has " << lineParts.size()
					<< L": " << desc.constraintData;
				throw UnexpectedInputException("createConstraints", err);
			}

			split(classParts, lineParts[0], is_any_of(L","));
			split(featureParts, lineParts[1], is_any_of(L","));

			classIDs.clear();

			BOOST_FOREACH(const wstring& cls, classParts) {
				classIDs.push_back(problem.classNumber(cls));
			}

			constraints->push_back(make_shared<FICC>(data, featureParts, classIDs,
						desc.weight, RelationDocWordsConstraintView()));
		} else {
			wstringstream err;
			err << L"Unrecognized constraint type: '" << desc.constraintType
				<< L"'";
			throw UnexpectedInputException("createConstraints", err);
		}
	}

	wcout << L"Loaded " << constraints->size() << L" constraints" << endl;
}


void passageMode() {
		ProblemDefinition_ptr problem =
			ProblemDefinition::load(ParamReader::getRequiredParam("problem_definition"));
		DataSet<Passage> passages;

		unsigned int num_relations = problem->classNames().size();
		loadPassages(passages, num_relations);

		shared_ptr<SymmetricDirichlet> wordPrior =
			make_shared<SymmetricDirichlet>(1.0);

		Passage::finishedLoading(num_relations, wordPrior);
		PassageConstraints_ptr constraints = make_shared<PassageConstraints>();
		createConstraints(passages, constraints, *problem);
		PassageInstanceConstraints_ptr instanceConstraints 
			= make_shared<PassageInstanceConstraints>(
					passages.nGraphs(), constraints->size());

		GraphicalModel::PREM<Passage> prModel(passages, num_relations,
				constraints, instanceConstraints,
				GraphicalModel::PREM<Passage>::SGD, true);

		throw UnexpectedInputException("passageMode()", "Must implement reporter before you can run this");
		prModel.em(passages, 10, Reporter());

		prModel.e(passages);

		wcout << L"Dumping output..." << endl;

		throw UnexpectedInputException("passageMode()", "Fix me to use passage mode");
		PassageDumper dumper(L"Unsupervised Events Dump", 
				"passages.unsupervised_events", problem->classNames());
		//dumper.dumpAll(passages.graphs);
}*/

