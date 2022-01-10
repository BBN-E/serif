#ifndef _TEMPORAL_TYPES_H_
#define _TEMPORAL_TYPES_H_
#include <vector>
#include <boost/shared_ptr.hpp>

typedef std::vector<unsigned int> TemporalFV;
typedef boost::shared_ptr<TemporalFV> TemporalFV_ptr;

#endif
