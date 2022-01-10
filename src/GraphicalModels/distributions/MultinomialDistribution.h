#ifndef _MULTINOMIAL_DIST_H_
#define _MULTINOMIAL_DIST_H_

#include <vector>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "../VectorUtils.h"
#include "../Alphabet.h"
#include "Counts.h"

namespace GraphicalModel {

template <typename PriorType=NoPrior>
class MultinomialDistribution {
	public:
		typedef PriorType Prior;
		typedef std::vector<boost::shared_ptr<MultinomialDistribution<PriorType > > > 
			Distributions;
		MultinomialDistribution(int n_items, const boost::shared_ptr<PriorType>& prior) 
			: _probs(n_items), _prior(prior) {}

		void reset() {
			zero(_probs);
		}

		void observeInstance(const std::vector<unsigned int>& observations,  
				double observation_weight = 1.0)
		{
			std::vector<unsigned int>::const_iterator it = observations.begin();
			for (; it!=observations.end(); ++it) {
				observeInstance(*it, observation_weight);
			}
		}

		void observeInstance(unsigned int observation, double observation_weight = 1.0) {
			_probs[observation] += observation_weight;
		}

		void digestObservations() {
			double total = 0.0;
			for (unsigned int i =0; i< _probs.size(); ++i) {
				double & p = _probs[i];
				p += _prior->pseudoCount(i);
				total += p;
			}
			for (unsigned int i=0; i<_probs.size(); ++i) {
				_probs[i] /= total;
			}
		}

		void toLogSpace() {
			std::vector<double>::iterator it = _probs.begin();
			for (; it!=_probs.end(); ++it) {
				*it = log(*it);
			}
		}

		double operator()(unsigned int i) const {
			return _probs[i];
		}

		double probOfObservations(const std::vector<unsigned int>& obs) {
			double val = 0.0;
			std::vector<unsigned int>::const_iterator it = obs.begin();
			for (; it!=obs.end(); ++it) {
				val += _probs[*it];
			}
			return val;
		}

		unsigned int size() const { return _probs.size(); }

		static void dumpDistributions(std::wostream& out, 
				const Distributions& distributions,
				const Alphabet& alphabet)
		{
			out << L"<table>" << std::endl;
			assert(!distributions.empty());
			for (size_t i =0; i<distributions[0]->size(); ++i) {
				out << L"<tr>";
				out << L"<td>" << alphabet.name(i) << L"</td>";
				double biggest = 0.0;
				for (size_t j=0; j<distributions.size(); ++j) {
					if ((*distributions[j])(i) > biggest) {
						biggest = (*distributions[j])(i);
					}
				}
				for (size_t j =0; j<distributions.size(); ++j) {
					double val = (*distributions[j])(i);
					bool big = val == biggest;
					out << L"<td>";
					if (big) {
						out << L"<b>";
					}
					out << 100.0*val;
					if (big) {
						out << L"</b>";
					}
					out << L"</td>";
				}
				out << L"</tr>" << std::endl;
			}
			out << L"</table>" << std::endl;
		}

		void toHTML(std::wostream& out) const {
			out << L"<table>" << std::endl;

			for (size_t i=0; i<_probs.size(); ++i) {
				out << L"<tr><td>" << i << L"</td><td>" << 100.0*_probs[i] 
					<< L"</td>" << std::endl;
			}

			out << L"</table>" << std::endl;
		}

	private:
		boost::shared_ptr<PriorType> _prior;
		std::vector<double> _probs;
};

typedef MultinomialDistribution<NoPrior> MultiNoPrior;
typedef MultinomialDistribution<SymmetricDirichlet> MultiSymPrior;
typedef MultinomialDistribution<AsymmetricDirichlet> MultiAsymPrior;
}

#endif

