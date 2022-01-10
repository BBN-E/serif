#ifndef _TEMPORAL_TYPE_TABLE_H_
#define _TEMPORAL_TYPE_TABLE_H_

#include <vector>
#include <string>
#include <boost/bimap.hpp>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/BoostUtil.h"
BSP_DECLARE(TemporalTypeTable)

class TemporalTypeTable {
public:
	typedef std::pair<unsigned int, std::wstring> AttributePair;
	typedef std::vector<AttributePair> AttributePairs;

	const std::vector<unsigned int>& ids() const;
	const std::wstring& name(unsigned int idx) const;
private:
	typedef boost::bimap<unsigned int, std::wstring> MapType;
	TemporalTypeTable(const AttributePairs& attributePairs);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(TemporalTypeTable, const AttributePairs&);

	MapType _data;
	std::vector<unsigned int> _ids;
};

#endif
