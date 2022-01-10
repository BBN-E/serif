#pragma warning( disable: 4996 )
#include "Generic/common/leak_detection.h"

#include <iostream>
#include <set>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "ActiveLearning/ActiveLearningData.h"
#include "ActiveLearning/DataView.h"
#include "ActiveLearning/strategies/WeightedUncertainty.h"
#include "ActiveLearning/AnnotatedFeatureRecord.h"
#include "ActiveLearning/alphabet/FeatureAlphabet.h"

using namespace Eigen;
using std::wstring;
using std::vector;
using boost::split; using boost::is_any_of;
using boost::trim;
using boost::lexical_cast;
using boost::wregex; using boost::wsmatch; using boost::regex_match;

// Weighted uncertainty selection strategy
// see Druck, Settles, and McCallum. "Active Learning by Labeling Features."
// EMNLP 2009

WeightedUncertainty::WeightedUncertainty(ActiveLearningData_ptr al_data, 
			InferenceDataView_ptr data, FeatureAlphabet_ptr alphabet) :
MaxScoreStrategy(al_data, data->nFeatures()), _data(data), _alphabet(alphabet),
	_log_frequencies(VectorXd::Zero(_data->nFeatures())), 
	_entropy(VectorXd::Zero(_data->nFeatures())), 
	_freq_modifiers(_data->nFeatures(), 1.0)
{
	SessionLogger::info("LEARNIT") << "Remember to cite Druck, Settles, and McCallum. \"Active Learning "
		<< "by Labeling Features\". EMNLP 2009." << std::endl;
	load_frequency_modifiers();

	BOOST_FOREACH(AnnotatedFeatureRecord_ptr ann, _alData->annotatedFeatures()) {
		_labelled.insert(ann->idx);
	}
	gather_frequencies();
	gather_entropies();

	for (unsigned int i=0; i < _data->nFeatures(); ++i) {
		if (!labelled(i)) {
			push_queue(i, score(i));
		}
	}
}

// TODO: refactor this with identical code in coverage strategy
void WeightedUncertainty::load_frequency_modifiers() {
	const std::string sData =
		ParamReader::getRequiredParam("weighted_uncertainty_frequency_modifiers");
	wstring data(sData.begin(), sData.end());

	trim(data);
	wregex name_and_val(L"(\\w+)\\(([0-9.]+)\\)");
	wsmatch matches;
	std::vector<wstring> entries;
	split(entries, data, is_any_of(","));
	BOOST_FOREACH(const std::wstring& entry, entries) {
		if (regex_match(entry, matches, name_and_val)) {
			const wstring feature_class_name(matches[1].first, matches[1].second);
			double val = lexical_cast<double>(wstring(matches[2].first, matches[2].second));
			SessionLogger::info("freq_weight") << L"For active learning, modifying the"
				<< L" frequency of " << feature_class_name << L" by " << val;
			bool marked=false;
			BOOST_FOREACH(int i, _alphabet->featureIndicesOfClass(feature_class_name, true)) {
				_freq_modifiers[i] = val;
				marked=true;
			}
			if (!marked) {
				SessionLogger::warn("active_learning") << L"When setting coverage strategy "
					<< "active learning frequency modifiers, failed to find any "
					"features of class " << feature_class_name;
			}
		}
	}
}

void WeightedUncertainty::gather_frequencies() {
	// we store not straight frequencies but rather log frequencies
	for (unsigned int i=0; i<_data->nFeatures(); ++i) {
		// we add 1 so that features occuring only once
		// do not get zero log freq
		double freq = (1 + _data->frequency(i)) * _freq_modifiers[i];
		if (freq > 0.0) {
			_log_frequencies(i) = log(freq);
		} else {
			_log_frequencies(i) = 0;
		}
	}
}

void WeightedUncertainty::gather_entropies() {
	static double ln_2 = log(2.0);
	for (unsigned int i=0; i<_data->nFeatures(); ++i) {
		_entropy[i] = 0.0;
	}

	for (unsigned int i=0; i<_data->nInstances(); ++i) {
		double prob = _data->prediction(i);
		double entropy;

		if (prob == 0.0 || prob == 1.0) {
			entropy = 0.0;
		} else {
			// we used base 2 logarithms to make the numbers easier to
			// interpret
			entropy = -(prob * log(prob)/ln_2 + (1.0 - prob) * log(1.0 - prob)/ln_2);
		}

		SparseVector<double>::InnerIterator instIt(_data->features(i));

		for (; instIt; ++instIt) {
			_entropy[instIt.index()] += entropy;
		}
	}
}


double WeightedUncertainty::score(int k) {
	return _log_frequencies(k) * _entropy(k)/_data->frequency(k);
}

void WeightedUncertainty::modelUpdate() {
	gather_entropies();
	for (unsigned int i=0; i<_data->nFeatures(); ++i) {
		if (!labelled(i)) {
			update_queue(i, score(i));
		}
	}
}

bool WeightedUncertainty::labelled(int feat) const {
	return _labelled.find(feat) != _labelled.end();
}

void WeightedUncertainty::label(int k) {
	_labelled.insert(k);
}
