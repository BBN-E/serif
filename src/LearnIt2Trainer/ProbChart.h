#ifndef _PROBCHART_H_
#define _PROBCHART_H_

#include <string>
#include <vector>
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(ProbChart)
BSP_DECLARE(InferenceDataView)

class ProbChart {
public:
	ProbChart(const InferenceDataView_ptr view1, const InferenceDataView_ptr view2);
	std::string json() const;
	void observe();
private:
	const InferenceDataView_ptr _view1;
	const InferenceDataView_ptr _view2;

	std::vector<double> _probs1;
	std::vector<double> _probs2;
	mutable boost::mutex _prob_chart_mutex;
};

#endif
