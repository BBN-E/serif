#pragma once
#include <vector>
#include <set>
#include <boost/shared_ptr.hpp>
#include <boost/pending/mutable_queue.hpp>
#include "Generic/common/bsp_declare.h"
#include "ActiveLearning/strategies/ActiveLearningStrategy.h"

BSP_DECLARE(ActiveLearningData)

class MaxScoreStrategy : public ActiveLearningStrategy {
public:
	virtual int nextFeature();
protected:
	MaxScoreStrategy(ActiveLearningData_ptr al_data, int nFeatures);
	void push_queue(int feature, double score);
	void update_queue(int feature, double score);
	virtual void registerPop(int feature);
private:
	class ScoreComp {
	public:
		ScoreComp(boost::shared_ptr<std::vector<double> > scores);
		bool operator()(int left, int right) const;
		boost::shared_ptr<std::vector<double> > scores;
	};

	boost::shared_ptr<std::vector<double> > _scores;
	ScoreComp _score_comp;
	boost::mutable_queue<int, std::vector<int>, ScoreComp> _queue;
};
