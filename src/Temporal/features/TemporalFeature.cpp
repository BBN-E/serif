#include "Generic/common/leak_detection.h"
#include "TemporalFeature.h"

#include <cmath>
#include <boost/functional/hash.hpp>
#include <boost/foreach.hpp>
#include "Generic/common/UnexpectedInputException.h"
#include "Temporal/TemporalInstance.h"
#include "PredFinder/elf/ElfRelation.h"

using std::wstringstream;

const Symbol& TemporalFeature::relation() const {
	return _relation;
}

const Symbol& TemporalFeature::type() const {
	return _type;
}

size_t TemporalFeature::hash() const {
	if (!_init_hash) {
		_init_hash = true;
		_hash = calcHash();
	}
	return _hash;
}

TemporalFeature::TemporalFeature(const Symbol& type, const Symbol& relation)
	: _type(type), _relation(relation), _init_hash(false)
{
	if (_type.is_null()) {
		throw UnexpectedInputException("TemporalFeature::TemporalFeature",
				"Type must not be Symbol()!");
	}
}

size_t TemporalFeature::topHash() const {
	std::size_t ret = 0;
	if (!_relation.is_null()) {
		boost::hash_combine(ret, _relation.to_string());
	} else {
		boost::hash_combine(ret, 0);
	}
	boost::hash_combine(ret, _type.to_string());
	return ret;
}

bool TemporalFeature::topEquals(const TemporalFeature_ptr& other) const {
	return other->relation() == relation() && 
		other->type() == type();
}

void TemporalFeature::metadata(wstringstream& str) const {
	str << type().to_string() << L"\t";

	if (!relation().is_null()) {
		str << L"1\t" << relation().to_string();
	} else {
		str << L"0";
	}
}

TemporalFeature::~TemporalFeature() {
}

bool TemporalFeature::relationNameMatches(TemporalInstance_ptr inst) const {
	if (_relation.is_null()) {
		return true;
	} else {
		return _relation.to_string() == inst->relation()->get_name();
	}
}

void TemporalFeature::setIdx(unsigned int val) {
	_idx = val;
}

unsigned int TemporalFeature::idx() const {
	return _idx;
}

void TemporalFeature::setWeight(unsigned int alphabet, double weight) {
	if (alphabet >= _weights.size()) {
		_weights.resize(alphabet + 1, 0.0);
	}
	_weights[alphabet] = weight;
}

double TemporalFeature::weight(unsigned int alphabet) const {
	if (alphabet < _weights.size()) {
		return _weights[alphabet];
	} else {
		wstringstream err;
		err << L"Alphabet " << alphabet << L" out of range";
		throw UnexpectedInputException("TemporalFeature::weight", err);
	}
}

bool TemporalFeature::passesThreshold(double threshold) const {
	BOOST_FOREACH(double weight, _weights) {
		if (fabs(weight) >= threshold) {
			return true;
		}
	}
	return false;
}

