// Copyright 2014 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CONFIDENCE_ESTIMATOR_H
#define CONFIDENCE_ESTIMATOR_H

#include <boost/shared_ptr.hpp>

class DocTheory;

class ConfidenceEstimator {
public:
	/** Create and return a new ConfidenceEstimator. */
	static ConfidenceEstimator *build() { return _factory()->build(); }
	/** Hook for registering new ConfidenceEstimator factories */
	struct Factory { virtual ConfidenceEstimator *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual void process(DocTheory *docTheory) = 0;
	virtual ~ConfidenceEstimator() {}
protected:
	ConfidenceEstimator() {}

private:
	static boost::shared_ptr<Factory> &_factory();
};

#endif
