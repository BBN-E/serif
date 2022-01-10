#ifndef xx_CONFIDENCE_ESTIMATOR_H
#define xx_CONFIDENCE_ESTIMATOR_H

#include "Generic/confidences/ConfidenceEstimator.h"

class NullConfidenceEstimator : public ConfidenceEstimator {
private:
	friend class NullConfidenceEstimatorFactory;

public:
	void process (DocTheory* docTheory) {}

private:
	NullConfidenceEstimator() {}

};

class NullConfidenceEstimatorFactory: public ConfidenceEstimator::Factory {
	virtual ConfidenceEstimator *build() { return _new NullConfidenceEstimator(); }
};

#endif
