#pragma once

#include <vector>
#include <boost/shared_ptr.hpp>

template<typename T>
class FeatureCollectionIterator {
public:
	FeatureCollectionIterator(const std::vector<boost::shared_ptr<T> >& features, 
		const std::set<int>& good_indices, bool constraining)
		: _features(features), _good_indices(good_indices), _constraining(constraining),
		_goodIt(_good_indices.begin()), _featIt(_features.begin()) {}

	operator bool() const {
		if (_constraining) {
			return _goodIt!=_good_indices.end();
		} else {
			return _featIt!=_features.end();
		}
	}
	FeatureCollectionIterator& operator++() {
		if (_constraining) {
			if (_goodIt!=_good_indices.end()) {
				++_goodIt;
			}
		} else {
			if (_featIt!=_features.end()) {
				++_featIt;
			}
		}
		return *this;
	}
	const boost::shared_ptr<T>& operator*() const {
		if (_constraining) {
			return _features[*_goodIt];
		} else {
			return *_featIt;
		}
	}
private:
	bool _constraining;
	const std::vector<boost::shared_ptr<T> >& _features;
	const std::set<int>& _good_indices;
	std::set<int>::const_iterator _goodIt;
	typename std::vector<boost::shared_ptr<T> >::const_iterator _featIt;
};
