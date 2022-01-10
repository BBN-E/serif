#pragma once

#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/BoostUtil.h"
#include "ActiveLearning/strategies/ActiveLearningStrategy.h"

BSP_DECLARE(ActiveLearningData)
BSP_DECLARE(DataView)

class RandomStrategy : public ActiveLearningStrategy {
public:
	virtual int nextFeature();
	virtual void label(int k);
private:
	RandomStrategy(ActiveLearningData_ptr alData, DataView_ptr data, int tries = 1000);
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(RandomStrategy, ActiveLearningData_ptr,
		DataView_ptr);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(RandomStrategy, ActiveLearningData_ptr,
		DataView_ptr, int);
	
	DataView_ptr _data;
	int _max_tries;
};
