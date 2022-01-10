#ifndef _COUNTS_H_
#define _COUNTS_H_

#include <vector>
#include "../VectorUtils.h"

namespace GraphicalModel {
	
class NoPrior {
public:
	double pseudoCount(unsigned int i) const {
		return 0;
	}
};

BSP_DECLARE(SymmetricDirichlet)
class SymmetricDirichlet {
	public:
		SymmetricDirichlet(double a) : _a(a) {}
		double pseudoCount(unsigned int i) const {return _a;}
	private:
		double _a;
};

// could refactor this with counting code in MultinomialDistribution
// and BernoulliDistribution
BSP_DECLARE(AsymmetricDirichlet)
class AsymmetricDirichlet {
	public:
		AsymmetricDirichlet(unsigned int sz) : _a(sz) {}
		AsymmetricDirichlet(const std::vector<double> a) : _a(a) {}
		double pseudoCount(unsigned int i) const {return _a[i];}
		void reset() {
			zero(_a);
		}
		void observeInstance(const std::vector<unsigned int>& observations,
				double observation_weight = 1.0)
		{
			std::vector<unsigned int>::const_iterator it = observations.begin();
			for (; it!=observations.end(); ++it) {
				_a[*it] += observation_weight;
			}
		}
		void rescaleToMax(double new_biggest) {
			std::vector<double>::const_iterator cur_biggest =
				std::max_element(_a.begin(), _a.end());
			if (cur_biggest != _a.end()) {
				double scale = new_biggest / *cur_biggest;
				std::vector<double>::iterator it = _a.begin();
				for (; it!=_a.end(); ++it) {
					*it *= scale;
				}
			}
		}
		const std::vector<double>& counts() const { return _a; }
	private:
		std::vector<double> _a;
};

}

#endif

