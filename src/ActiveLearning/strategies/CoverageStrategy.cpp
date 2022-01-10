#pragma warning( disable: 4996 )
#include "Generic/common/leak_detection.h"

// the following is needed to get around a VC++ bug
// see http://lists.boost.org/MailArchives/ublas/2005/08/0674.php
#define BOOST_UBLAS_TYPE_CHECK 0 
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
#include "ActiveLearning/strategies/CoverageStrategy.h"
#include "ActiveLearning/AnnotatedFeatureRecord.h"
#include "ActiveLearning/alphabet/FeatureAlphabet.h"

using namespace Eigen;
using std::wstring;
using std::vector;
using boost::split; using boost::is_any_of;
using boost::trim;
using boost::lexical_cast;
using boost::wregex; using boost::wsmatch; using boost::regex_match;

// Coverage selection strategy
// see Druck, Settles, and McCallum. "Active Learning by Labeling Features."
// EMNLP 2009

CoverageStrategy::CoverageStrategy(ActiveLearningData_ptr al_data, 
			DataView_ptr data, FeatureAlphabet_ptr alphabet) :
MaxScoreStrategy(al_data, data->nFeatures()), _data(data), _alphabet(alphabet),
	_frequencies(VectorXd::Zero(_data->nFeatures())), 
	_feature_totals(_data->nInstances()), _labelled(),
	_phi(VectorXd::Zero(_data->nFeatures())), _n_instances(_data->nFeatures(), 0.0),
	_freq_modifiers(_data->nFeatures(), 1.0)
{
	SessionLogger::info("LEARNIT") << "Remember to cite Druck, Settles, and McCallum. \"Active Learning "
		<< "by Labeling Features\". EMNLP 2009." << std::endl;
	load_frequency_modifiers();

	BOOST_FOREACH(AnnotatedFeatureRecord_ptr ann, _alData->annotatedFeatures()) {
		_labelled.insert(ann->idx);
	}
	gather_feature_data();

	for (unsigned int i=0; i < _data->nFeatures(); ++i) {
		if (!labelled(i)) {
			push_queue(i, score(i));
		}
	}
}

void CoverageStrategy::load_frequency_modifiers() {
	const std::string sData =
		ParamReader::getRequiredParam("coverage_strategy_frequency_modifiers");
	std::wstring data(sData.begin(), sData.end());

	trim(data);
	wregex name_and_val(L"(\\w+)\\(([0-9.]+)\\)");
	wsmatch matches;
	std::vector<std::wstring> entries;
	split(entries, data, is_any_of(","));
	BOOST_FOREACH(const wstring& entry, entries) {
		if (regex_match(entry, matches, name_and_val)) {
			const wstring feature_class_name(matches[1].first, matches[1].second);
			double val = lexical_cast<double>(wstring(matches[2].first, matches[2].second));
			bool marked=false;
			BOOST_FOREACH(int i, _alphabet->featureIndicesOfClass(feature_class_name)) {
				_freq_modifiers[i] = val;
				marked=true;
			}
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

void CoverageStrategy::gather_feature_data() {
	// we store not straight frequencies but rather log frequencies
	for (unsigned int i=0; i<_data->nFeatures(); ++i) {
		double freq = _data->frequency(i) * _freq_modifiers[i];
		if (freq > 0.0) {
			_frequencies(i) = log(freq);
		} else {
			_frequencies(i) = -1000000.0;
		}
	}

	// feature_totals the total number of times each instance is seen with
	// an annotated feature
	BOOST_FOREACH(int feat, _labelled) { 
		_feature_totals += _data->instancesWithFeature(feat);
	}

	// store number of instances each feature appears in
	for (unsigned int k=0; k<_data->nFeatures(); ++k) {
		_n_instances[k] = _data->instancesWithFeature(k).nonZeros();
	}

	// phi_k is the dot product of feature_totals with the instance vector
	// for feature k. This is the sum over the number of shared instances
	// of feature k with each annotated feature.  So if instance x is shared
	// by feature k with three annotated features, it will be counted
	// three times.
	for (unsigned int k=0; k<_data->nFeatures(); ++k) {
		_phi(k) = _data->instancesWithFeature(k).dot(_feature_totals);
	}
}

double CoverageStrategy::score(int k) {
	if (_labelled.size() == 0) {
		return _frequencies(k);
	} else {
		if (_n_instances[k] > 0) {
			return _frequencies(k) * (1.0 - _phi(k)/(_labelled.size()*_n_instances[k]));
		} else {
			return -10000.0;
		}
	} 
}

void CoverageStrategy::label(int l) {
}

bool CoverageStrategy::labelled(int feat) const {
	return _labelled.find(feat) != _labelled.end();
}

void CoverageStrategy::registerPop(int l) {
	// this can be made more efficient if needed, though it may
	// require implementing a custom priority queue
	_labelled.insert(l);

	std::set<int> dirty;

	for (SparseVector<double>::InnerIterator inst_it(_data->instancesWithFeature(l)); inst_it; ++inst_it) {
		int inst = inst_it.index();
		for (SparseVector<double>::InnerIterator it(_data->features(inst)); it; ++it) {
			dirty.insert(it.index());
		}
	}

	BOOST_FOREACH(int feat, dirty) {
		_phi(feat) += _data->instancesWithFeature(feat).dot(_data->instancesWithFeature(l));
	}

	for (unsigned int feat = 0; feat < _data->nFeatures(); ++feat) {
		if (!labelled(feat)) {
			update_queue(feat, score(feat));
		}
	}
}
