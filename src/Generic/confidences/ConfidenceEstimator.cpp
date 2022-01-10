#include "Generic/common/leak_detection.h"

#include "Generic/confidences/ConfidenceEstimator.h"
#include "Generic/confidences/xx_ConfidenceEstimator.h"

boost::shared_ptr<ConfidenceEstimator::Factory> &ConfidenceEstimator::_factory() {
	static boost::shared_ptr<ConfidenceEstimator::Factory> factory(new NullConfidenceEstimatorFactory());
	return factory;
}
