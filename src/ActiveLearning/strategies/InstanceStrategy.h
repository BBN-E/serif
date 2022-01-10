#ifndef _INSTANCE_STRATEGY_H_
#define _INSTANCE_STRATEGY_H_

#include <vector>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(DataView)

class InstanceStrategy {
public:
	InstanceStrategy(DataView_ptr data);
	std::vector<int> clientInstances(int feat, size_t max_instances) const;
private:
	const DataView_ptr _data;
};

#endif
