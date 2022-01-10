#include "Generic/common/leak_detection.h"
#include "FeatureMap.h"

#include <sstream>
#include <boost/foreach.hpp>
#include "Generic/common/UnexpectedInputException.h"
#include "features/TemporalFeature.h"
#include "features/TemporalFeatureFactory.h"

using std::wstringstream;
using std::endl;

FeatureMap::FeatureMap() {}

unsigned int FeatureMap::index(TemporalFeature_ptr feature) {
	MapType::left_map::const_iterator probe = _data.left.find(feature);

	if (probe != _data.left.end()) {
		return probe->second;
	} else {
		int idx = _nextIdx++;
		_data.insert(MapType::value_type(feature, idx));
		return idx;
	}
}

TemporalFeature_ptr FeatureMap::feature(unsigned int idx) {
	MapType::right_map::const_iterator probe =
		_data.right.find(idx);

	if (probe != _data.right.end()) {
		return probe->second;
	} else {
		wstringstream err;
		err << "Cannot find feature " << idx;
		throw UnexpectedInputException("FeatureMap::featureString", err);
	}
}

FeatureMap::left_const_iterator FeatureMap::left_begin() const {
	return _data.left.begin();
}

FeatureMap::left_const_iterator FeatureMap::left_end() const {
	return _data.left.end();
}

FeatureMap::right_const_iterator FeatureMap::right_begin() const {
	return _data.right.begin();
}

FeatureMap::right_const_iterator FeatureMap::right_end() const {
	return _data.right.end();
}

void FeatureMap::load(const std::vector<std::pair<std::wstring, unsigned int> >& values) {
	typedef std::pair<std::wstring, unsigned int> EntryType;
	BOOST_FOREACH(const EntryType& entry, values) {
		right_const_iterator probe = _data.right.find(entry.second);
		TemporalFeature_ptr feature = TemporalFeatureFactory::create(entry.first);
		feature->setIdx(entry.second);

		if (probe == _data.right.end()) {
			_data.left.insert(std::make_pair(feature, entry.second));
		} else {
			if (probe->second->equals(feature)) {
				// this entry is already present, not a problem
			} else {
				wstringstream err;
				err << "Mismatch during load. For index " << entry.first 
					<< " current feature map value is '" << probe->second->dump()
					<< "' but new value is '" << feature->dump() << "'" << endl;
				throw UnexpectedInputException("FeatureMap::load", err);
			}
		}
	}
	updateNextIdx();
}

void FeatureMap::updateNextIdx() {
	_nextIdx = 0;
	for (right_const_iterator it = _data.right.begin(); it!= _data.right.end(); ++it) {
		unsigned int x = it->first;
		if ((x + 1) > _nextIdx) {
			_nextIdx = x + 1;
		}
	}
	assertDense();
}

void FeatureMap::assertDense() {
	if (_nextIdx != _data.size()) {
		wstringstream err;
		err << "Corrupt feature map: next index of " << _nextIdx << 
			" does not match map size of " << _data.size() << endl;
		throw UnexpectedInputException("FeatureMap::assertDense", err);
	}
}

size_t FeatureMap::size() const {
	return _data.size();
}
