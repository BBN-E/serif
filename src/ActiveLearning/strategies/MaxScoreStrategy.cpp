#include "Generic/common/leak_detection.h"
#include "MaxScoreStrategy.h"

#include <boost/property_map/property_map.hpp>
#include "ActiveLearning/exceptions/CouldNotChooseFeatureException.h"

MaxScoreStrategy::MaxScoreStrategy(ActiveLearningData_ptr al_data, int nFeatures) : 
ActiveLearningStrategy(al_data), 
_scores(_new std::vector<double>(nFeatures, 0.0)), _score_comp(_scores), 
_queue(nFeatures, _score_comp, boost::identity_property_map())
{
}

int MaxScoreStrategy::nextFeature() {
	if (!_queue.empty()) {
		int ret = _queue.front();
		_queue.pop();
		registerPop(ret);
		return ret;
	} else {
		throw CouldNotChooseFeatureException("MaxScoreStrategy::nextFeature",
			"All features exhausted, queue is empty.");
	}
}

MaxScoreStrategy::ScoreComp::ScoreComp(boost::shared_ptr<std::vector<double> > scores) 
: scores(scores) 
{}

bool MaxScoreStrategy::ScoreComp::operator()(int left, int right) const {
	return (*scores)[left] > (*scores)[right];
}

void MaxScoreStrategy::update_queue(int feature, double score) {
	(*_scores)[feature] = score;
	_queue.update(feature);
}

void MaxScoreStrategy::push_queue(int feature, double score) {
	(*_scores)[feature] = score;
	_queue.push(feature);
}

void MaxScoreStrategy::registerPop(int feature) {}
