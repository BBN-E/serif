// Suppress a Visual Studio warning that using the standard library routine
// tmpnam exposes you to certain security risks.  
#pragma warning( disable: 4996 )
#include "Generic/common/leak_detection.h"
#include "InstanceLoader.h"

#include <vector>
#include <set>
#include <iostream>
#include <fstream>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "Generic/common/UnexpectedInputException.h"
#include "ActiveLearning/InstanceHashes.h"
#include "ActiveLearning/DataView.h"
#include "ActiveLearning/alphabet/MultiAlphabet.h"
#include "LearnIt/LearnIt2.h"

using std::string;
using std::wstring;
using boost::lexical_cast;
using boost::make_shared;

LoadInstancesReturn InstanceLoader::load_instances(MultiAlphabet_ptr alphabet,
	const string& feature_vectors_file, int nFeatures, InstanceHashes& instance_hashes)
{
	std::ifstream inp(feature_vectors_file.c_str());

	// the first line of an instances file stores the total number of instances
	// we expect to read
	string line;
	getline(inp, line);
	boost::trim(line);
	int nInstances = lexical_cast<int>(line);
	
	InferenceDataView_ptr slotView =
		make_shared<InferenceDataView>(nInstances, nFeatures);
	InferenceDataView_ptr sentenceView = 
		make_shared<InferenceDataView>(nInstances, nFeatures);
	DataView_ptr combinedView =
		make_shared<DataView>(nInstances, nFeatures);
	
	std::set<unsigned int> features_in_instance;

	// each instance is stored as three lines
	int i = 0;
	std::vector<string> parts;
	while (std::getline(inp, line)) {
		parts.clear();
		// the first line contains the instance metadata
		boost::split(parts, line, boost::is_any_of("\t"));
		// the first part of the metadata is a unique id for each instance
		// consisting of the docid, the sentence number, and the argument tuple
		// rather than store it all, we just store a hash of it to use in
		// identifying instances for annotation
		instance_hashes.registerInstance(i, parts[0]);
		// for now we ignore the rest of the information on the line

		// slot view features on next line
		std::getline(inp, line);
		boost::trim(line);
		
		features_in_instance.clear();
		if (!line.empty()) {
			boost::split(parts, line, boost::is_any_of("\t"));
			BOOST_FOREACH(const string& slot_feature, parts) {
				unsigned int feat_idx = alphabet->indexFromAlphabet(
					atoi(slot_feature.c_str()), LearnIt2::SLOT_ALPHABET_INDEX);
				features_in_instance.insert(feat_idx);
			}
			slotView->observeFeaturesForInstance(i, features_in_instance);
			combinedView->observeFeaturesForInstance(i, features_in_instance);
		}
		// sentence view features on last line
		std::getline(inp, line);
		boost::trim(line);

		features_in_instance.clear();
		if (!line.empty()) {
			boost::split(parts, line, boost::is_any_of(L"\t"));
			BOOST_FOREACH(const string& sentence_feature, parts) {
				unsigned int feat_idx = alphabet->indexFromAlphabet(
					atoi(sentence_feature.c_str()), LearnIt2::SENTENCE_ALPHABET_INDEX);
				features_in_instance.insert(feat_idx);
			}
			sentenceView->observeFeaturesForInstance(i, features_in_instance);
			combinedView->observeFeaturesForInstance(i, features_in_instance);
		}

		++ i;
	}
	sentenceView->finishedLoading();
	slotView->finishedLoading();
	combinedView->finishedLoading();

	if (i != nInstances) {
		throw UnexpectedInputException("LearnIt2Trainer.cpp load_instances",
			"Number of instances actually read in feature vectors file does "
			"not match header.");
	}

	return LoadInstancesReturn(slotView, sentenceView, combinedView);
}


