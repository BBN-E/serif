#include "Generic/common/leak_detection.h"
#include "TemporalTypeTable.h"

#include <sstream>
#include <boost/foreach.hpp>
#include "Generic/common/UnexpectedInputException.h"

using std::wstring;
using std::pair;
using std::vector;
using std::wstringstream;

TemporalTypeTable::TemporalTypeTable(const AttributePairs& attributePairs) {
	BOOST_FOREACH(const AttributePair& attPair, attributePairs) {
		_ids.push_back(attPair.first);
		_data.left.insert(attPair);
	}
}

const std::vector<unsigned int>& TemporalTypeTable::ids() const {
	return _ids;
}

const wstring& TemporalTypeTable::name(unsigned int idx) const {
	MapType::left_map::const_iterator probe = _data.left.find(idx);

	if (probe != _data.left.end()) {
		return probe->second;
	} else {
		wstringstream err;
		err << L"No attribute type with index " << idx;
		throw UnexpectedInputException("TemporalTypeTable::name",
			err);
	}
}
