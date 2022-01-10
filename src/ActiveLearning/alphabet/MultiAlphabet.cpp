#include "Generic/common/leak_detection.h"

#include <sstream>
#include <boost/foreach.hpp>
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "ActiveLearning/AnnotatedFeatureRecord.h"
#include "MultiAlphabet.h"

using boost::make_shared;
using boost::dynamic_pointer_cast;
using std::wstringstream;
using namespace Eigen;

MultiAlphabet::MultiAlphabet(const std::vector<FeatureAlphabet_ptr>& alphabets) 
: _alphabets(alphabets.begin(), alphabets.end()), _offsets(alphabets.size())
{
	_offsets[0] = 0;
	for (size_t i=1; i<_alphabets.size(); ++i) {
		_offsets[i] = _offsets[i-1] + _alphabets[i-1]->size();
	}
}

int MultiAlphabet::dbNumForIdx(unsigned int idx) const {
    // This unusual loop with the decrement in the conditional avoids an
    // unsigned >= 0 error; based on a design pattern found on Stack Overflow
	for (size_t alphabet = _alphabets.size() - 1; alphabet--; ) {
		if (idx >= _offsets[alphabet]) {
			return static_cast<int>(alphabet);
		}
	}

	throw UnrecoverableException("MultiAlphabet::dbNumForIdx",
		"End of function should be unreachable!");
}

FeatureAlphabet_ptr MultiAlphabet::dbForIdx(int idx) {
	return _alphabets[dbNumForIdx(idx)];
}

const FeatureAlphabet_ptr MultiAlphabet::dbForIdx(int idx) const {
	return _alphabets[dbNumForIdx(idx)];
}


int MultiAlphabet::trans(unsigned int idx) const {
	for (int alphabet = static_cast<int>(_alphabets.size()) - 1; alphabet>=0; --alphabet) {
		if (idx >= _offsets[alphabet]) {
			return idx - _offsets[alphabet];
		}
	}

	throw UnexpectedInputException("MultiAlphabet::trans", "Cannot translate index");
}

int MultiAlphabet::size() const {
	int sz = 0;
	BOOST_FOREACH(const FeatureAlphabet_ptr& alphabet, _alphabets) {
		sz += alphabet->size();
	}
	return sz;
}

double MultiAlphabet::firstFeatureWeightByName(const std::wstring& feature_name) const {
	return featureWeightByIndex(firstFeatureIndexByName(feature_name));
}

double MultiAlphabet::featureWeightByIndex(int idx) const {
	return dbForIdx(idx)->featureWeightByIndex(trans(idx));
}

int MultiAlphabet::firstFeatureIndexByName(const std::wstring& feature_name) const {
	int found_in = -1;
	int idx;

	for(size_t alphabet=0; alphabet<_alphabets.size(); ++alphabet) {
		try {
			idx = _alphabets[alphabet]->firstFeatureIndexByName(feature_name);
			found_in = static_cast<int>(alphabet);
			break;
		} catch (UnexpectedInputException) {}
	}	
	
	if (found_in != -1) {
		return idx + _offsets[found_in];
	} else {
		wstringstream err;
		err << "Feature with name \"" << feature_name << "\" found in no "
			<< "sub-alphabet.";
		throw UnexpectedInputException("MultiAlphabet::firstFeatureIndexByName",
			err);
	}
}

std::vector<AnnotatedFeatureRecord_ptr> MultiAlphabet::annotations() const {
	std::vector<AnnotatedFeatureRecord_ptr> ret;
	std::vector<AnnotatedFeatureRecord_ptr> tmp;

	for (size_t i=0; i<_alphabets.size(); ++i) {
		tmp.clear();
		tmp = _alphabets[i]->annotations();
		BOOST_FOREACH(AnnotatedFeatureRecord_ptr afr, tmp) {
			afr->idx += _offsets[i];
		}
		ret.insert(ret.begin(), tmp.begin(), tmp.end());
	}

	return ret;
}

void MultiAlphabet::recordAnnotations(
				const std::vector<AnnotatedFeatureRecord_ptr>& annotations)
{
	std::vector<std::vector<AnnotatedFeatureRecord_ptr> > byAlphabet(_alphabets.size());

	BOOST_FOREACH(AnnotatedFeatureRecord_ptr afr, annotations) {
		int dbNum = dbNumForIdx(afr->idx);
		afr->idx = trans(afr->idx);
		byAlphabet[dbNum].push_back(afr);
	}

	for (size_t i=0; i<byAlphabet.size(); ++i) {
		_alphabets[i]->recordAnnotations(byAlphabet[i]);
	}
}

Eigen::VectorXd MultiAlphabet::getWeights() const {
	int sz = size();
	Eigen::VectorXd ret(VectorXd::Zero(sz));

	for (size_t i=0; i<_alphabets.size(); ++i) {
		Eigen::VectorXd weights = _alphabets[i]->getWeights();
		int offset = _offsets[i];

		for (int j=0; j<weights.size(); ++j) {
			ret(j + offset) = weights(j);
		}
	}

	return ret;
}

std::vector<int> MultiAlphabet::featureIndicesOfClass(
		const std::wstring& feature_class_name, bool use_metadata_field) const
{
	std::vector<int> ret;

	for (size_t i=0; i<_alphabets.size(); ++i) {
		std::vector<int> tmp = 
			_alphabets[i]->featureIndicesOfClass(feature_class_name, use_metadata_field);
		int offset = _offsets[i];

		BOOST_FOREACH(int idx, tmp) {
			ret.push_back(idx + offset);
		}
	}

	return ret;
}

void MultiAlphabet::setFeatureWeight(int idx, double weight) {
	dbForIdx(idx)->setFeatureWeight(trans(idx), weight);
}

void MultiAlphabet::setFeatureWeights(const VectorXd& values) {
	if (values.size() != size()) {
		throw UnexpectedInputException("MultiAlphabet::setFeatureWeights",
			"Size of provided vector does not match number of features");
	}

	for (size_t i=0; i<_alphabets.size(); ++i) {
		VectorXd setVec(_alphabets[i]->size());
		int offset = _offsets[i];
		for (int j=0; j<setVec.size(); ++j) {
			setVec(j) = values(j+offset);
		}
		_alphabets[i]->setFeatureWeights(setVec);
	}
}

void MultiAlphabet::featureRegularizationWeights(VectorXd& regWeights,
		VectorXd& negRegWeights) const
{
	if (size() != regWeights.size()) {
		throw UnexpectedInputException("MultiAlphabet::featureRegularizationWeights",
			"Regularization weight vector size does not match number of features in DB");
	}

	for (size_t i=0; i<_alphabets.size(); ++i) {
		VectorXd weightsVec(_alphabets[i]->size());
		VectorXd negWeightsVec(_alphabets[i]->size());

		_alphabets[i]->featureRegularizationWeights(weightsVec, negWeightsVec);
		int offset = _offsets[i];

		for (int j=0; j<weightsVec.size(); ++j) {
			regWeights(j + offset) = weightsVec(j);
			negRegWeights(j + offset) = negWeightsVec(j);
		}
	}
}

std::pair<VectorXd, VectorXd> MultiAlphabet::featureRegularizationWeights() const {
	VectorXd regWeights(size());
	VectorXd negRegWeights(size());
	featureRegularizationWeights(regWeights, negRegWeights);
	return std::make_pair(regWeights, negRegWeights);
}

std::wstring MultiAlphabet::getFeatureName(int idx) const {
	return dbForIdx(idx)->getFeatureName(trans(idx));
}


unsigned int MultiAlphabet::indexFromAlphabet(unsigned int idx, unsigned int alphabet) const {
	if (alphabet < _offsets.size()) {
		return _offsets[alphabet] + idx;
	} else {
		wstringstream err;
		err << "No such alphabet " << alphabet;
		throw UnexpectedInputException("MultiAlphabet::indexFromAlphabet", err);
	}
}

DatabaseConnection::Table_ptr MultiAlphabet::getFeatureRows(double threshold, 
		bool include_negatives, const std::wstring& constraints) const 
{
	DatabaseConnection::Table_ptr ret = make_shared<DatabaseConnection::Table>();

	for (size_t i=0; i<_alphabets.size(); ++i) {
		DatabaseConnection::Table_ptr tmp = _alphabets[i]->getFeatureRows(threshold, 
				include_negatives, constraints);
		ret->insert(ret->end(), tmp->begin(), tmp->end());
	}

	return ret;
}

MultiAlphabet_ptr MultiAlphabet::create(FeatureAlphabet_ptr first,
										FeatureAlphabet_ptr second)
{
	std::vector<FeatureAlphabet_ptr> alphabets;
	alphabets.push_back(first);
	alphabets.push_back(second);

	return create(alphabets);
}

MultiAlphabet_ptr MultiAlphabet::create(std::vector<FeatureAlphabet_ptr> alphabets) {
	return MultiAlphabet_ptr(_new MultiAlphabet(alphabets));
}
