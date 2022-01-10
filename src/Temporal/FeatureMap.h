#include <string>
#include <vector>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include "Generic/common/bsp_declare.h"
#include "Temporal/features/TemporalFeature.h"

BSP_DECLARE(FeatureMap)
BSP_DECLARE(TemporalFeature)

class FeatureMap {
private:
	typedef boost::bimaps::bimap<
		boost::bimaps::unordered_set_of<TemporalFeature_ptr, 
			TemporalFeatureHash, TemporalFeatureEquality>, 
			boost::bimaps::unordered_set_of<unsigned int> > MapType;
public:
	FeatureMap();
	unsigned int index(const TemporalFeature_ptr feature);
	TemporalFeature_ptr feature(unsigned int idx);

	size_t size() const;

	typedef MapType::left_map::const_iterator left_const_iterator;
	typedef MapType::right_map::const_iterator right_const_iterator;
	typedef MapType::left_map::value_type left_value_type;
	typedef std::vector<left_value_type> left_value_list;
	typedef MapType::right_map::value_type right_value_type;
	typedef std::vector<right_value_type> right_value_list;

	left_const_iterator left_begin() const;
	left_const_iterator left_end() const;
	
	right_const_iterator right_begin() const;
	right_const_iterator right_end() const;

	void load(const std::vector<std::pair<std::wstring, unsigned int> >& vals);
private:
	void checkFeatureMapIsConsistentWithDBContents(FeatureMap_ptr featureMap);
	void updateNextIdx();
	void assertDense();

	MapType _data;
	unsigned int _nextIdx;
public:
	typedef MapType::left_const_reference ConstEntryRef;
};
