#pragma once
#include <boost/noncopyable.hpp>
#include <boost/weak_ptr.hpp>
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(ActiveLearningData)
BSP_DECLARE(ActiveLearningStrategy)
typedef boost::weak_ptr<ActiveLearningData> ActiveLearningData_wptr;

class ActiveLearningStrategy : boost::noncopyable {
public:
	virtual int nextFeature() = 0;
	virtual void label(int k) = 0;
	virtual void modelUpdate();
protected:
	ActiveLearningStrategy(ActiveLearningData_ptr al_data);
protected:
	ActiveLearningData_ptr _alData;
};
