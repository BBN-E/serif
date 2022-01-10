#include "trainer.h"
#include "ProbChart.h"
#include "ActiveLearning/DataView.h"

using std::string;
using std::vector;
using std::stringstream;

ProbChart::ProbChart(const InferenceDataView_ptr view1, const InferenceDataView_ptr view2) : 
_view1(view1), _view2(view2), _probs1(101, 0.0), _probs2(101, 0.0) {}

string ProbChart::json() const {
	boost::mutex::scoped_lock l(_prob_chart_mutex);
	stringstream out;
	out << "[[";
	bool first = true;
	for (size_t i=0; i<_probs1.size(); ++i) {
		if (first) {
			first = false;
		} else {
			out << ", ";
		}
		out << "[" << i/100.0 << ", " << _probs1[i] << "]";
	}
	out << "], [";
	first = true;
	for (size_t i=0; i<_probs2.size(); ++i) {
		if (first) {
			first = false;
		} else {
			out << ", ";
		}
		out << "[" << i/100.0 << ", " << _probs2[i] << "]";
	}
	out << "]]";
	return out.str();
}

void ProbChart::observe() {
	boost::mutex::scoped_lock l(_prob_chart_mutex);
	
	fill(_probs1.begin(), _probs1.end(), 0.0);
	fill(_probs2.begin(), _probs2.end(), 0.0);

	for (unsigned int i=0; i < _view1->nInstances(); ++i) {
		++_probs1[(int)(100*_view1->prediction(i))];
	}
	
	for (unsigned int i=0; i< _view2->nInstances(); ++i) {
		++_probs2[(int)(100*_view2->prediction(i))];
	}

	double cumul1 = 0.0;
	double cumul2 = 0.0;

	for (int i=100; i>=0; --i) {
		double new_cumul = cumul1 + _probs1[i];
		_probs1[i] += cumul1;
		cumul1 = new_cumul;

		new_cumul = cumul2 + _probs2[i];
		_probs2[i] += cumul2;
		cumul2 = new_cumul;
	}
}
