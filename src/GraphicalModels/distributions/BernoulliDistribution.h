#ifndef _BERNOULLI_DIST_H_
#define _BERNOULLI_DIST_H_

#include <vector>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "../VectorUtils.h"
#include "Counts.h"

namespace GraphicalModel {

class NoPrior;

template <typename PriorType=NoPrior>
class BernoulliDistribution {
	public:
		typedef PriorType Prior;
		BernoulliDistribution(int n_items, const boost::shared_ptr<PriorType>& prior) 
			: _probs(n_items), _n_instances(0), _all_negative(0), _prior(prior) {}

		void reset() {
			zero(_probs);
			_n_instances = 0;
			_all_negative = 0;
		}

		void observeInstance(const std::vector<unsigned int>& observations,  
				double observation_weight = 1.0)
		{
			std::vector<unsigned int>::const_iterator it = observations.begin();
			_n_instances += observation_weight;
			for (; it!=observations.end(); ++it) {
				_probs[*it] += observation_weight;
			}
		}

		void digestObservations() {
			unsigned int i = 0;
			std::vector<double>::iterator it = _probs.begin();
			for (; it!=_probs.end(); ++it) {
				double& p = *it;
				// this is not right...
				double prior = _prior->pseduoCount(i);
				p = (p+prior) / (_n_instances + 2.0 *prior);
				++i;
			}
		}

		void toLogSpace() {
			std::vector<double>::iterator it = _probs.begin();
			for (; it!=_probs.end(); ++it) {
				double& p = *it;
				double log1MinusP = log(1.0 - p);
				_all_negative += log1MinusP;
				p = log(p) - log1MinusP;
			}
		}

		double operator()(unsigned int i) const {
			return _probs[i];
		}

		double probOfObservations(const std::vector<unsigned int>& obs) {
			double val = _all_negative;
			std::vector<unsigned int>::const_iterator it = obs.begin();
			for (; it!=obs.end(); ++it) {
				val += _probs[*it];
			}
			return val;
		}
	private:
		boost::shared_ptr<PriorType> _prior;
		std::vector<double> _probs;
		double _n_instances;
		double _all_negative;
};

}

#endif

