#ifndef _SINGLE_FEATURE_DISTRIBUTION_FACTOR_H_
#define _SINGLE_FEATURE_DISTRIBUTION_FACTOR_H_

#include <vector>
#include <cmath>
#include <boost/make_shared.hpp>
#include "Generic/common/bsp_declare.h"
#include "Factor.h"
#include "Alphabet.h"
#include "VectorUtils.h"
#include "distributions/MultinomialDistribution.h"

namespace GraphicalModel {

template <typename GraphType, typename FactorType, 
		 typename VariableType, typename Prior>
class SingleFeatureDistributionFactor 
	: public ModifiableUnaryFactor<GraphType,FactorType> 
{
	public:
		typedef MultinomialDistribution<Prior> Distribution;
		typedef boost::shared_ptr<Distribution> Distribution_ptr;
		typedef std::vector<Distribution_ptr> Distributions;

		SingleFeatureDistributionFactor(unsigned int n_classes,
				const std::wstring& featureVal) : _feature_val(alphabet.feature(featureVal)),
			 GraphicalModel::ModifiableUnaryFactor<GraphType, FactorType>(n_classes) {}

		/*const std::vector<unsigned int>& features() const {
			return static_cast<const FactorType*>(this)->featuresImpl();
		}*/

		static void finishedLoading(unsigned int num_var_vals,
				const boost::shared_ptr<typename Distribution::Prior>& prior)
		{
			featureProbs = Distributions(num_var_vals);
			typename Distributions::iterator it = featureProbs.begin();

			for (; it!=featureProbs.end(); ++it) {
				*it = boost::make_shared<Distribution>(alphabet.size(), prior);
			}
		}

		static void clearCounts() {
			typename Distributions::iterator distIt = featureProbs.begin();
			for (; distIt!=featureProbs.end(); ++distIt) {
				(*distIt)->reset();
			}
		}

		void observe(const VariableType& v) 
		{
			for (size_t cls =0; cls < featureProbs.size(); ++cls) {
				featureProbs[cls]->observeInstance(_feature_val, v.marginals()[cls]);
			}
		}

		template <typename Dumper>
		static void updateParametersFromCounts(Dumper& dumper) {
			typename Distributions::iterator distIt = featureProbs.begin();
			for (; distIt!=featureProbs.end(); ++distIt) {
				(*distIt)->digestObservations();
			}

			Distribution::dumpDistributions(dumper.stream(), distributions(),
					alphabet);

			for (distIt=featureProbs.begin(); distIt!=featureProbs.end(); ++distIt) {
				(*distIt)->toLogSpace();
			}
		}

		static Distributions& distributions() { return featureProbs; }

		static unsigned int featureIndex(const std::wstring& feature) {
			return alphabet.featureAlreadyPresent(feature);
		}

		static std::wstring featureName(size_t idx) {
			return alphabet.name(idx);
		}

		bool hasFeature(unsigned int idx) const {
			return _feature_val == idx;
		}

		unsigned int feature() const {
			return _feature_val;
		}

		virtual void dumpOutgoingMessagesToHTML(const std::wstring& rowTitle,
					std::wostream& out, bool verbose=false) const 
			{
				out << L"<tr>";
				out << L"<td><b>" << rowTitle;
				if (verbose) {
					out << L" (" << alphabet.name(_feature_val) << L")";
				}
				out << L"</b></td>";
				MessageVector::const_iterator msg = this->outgoingMessages().begin();
				for (; msg!=this->outgoingMessages().end(); ++msg) {
					(*msg)->toHTMLTDs(out);
				}
				out << L"</tr>";
			}

	private:
		static Distributions featureProbs;
		unsigned int _feature_val;
		static GraphicalModel::Alphabet alphabet;

		friend class GraphicalModel::ModifiableUnaryFactor<GraphType,FactorType>;
		double factorPreModImpl(unsigned int assignment) const {
			return (*featureProbs[assignment])(_feature_val);
		}
};


template <typename GraphType, typename FactorType, typename VariableType, typename Prior>
std::vector<boost::shared_ptr<MultinomialDistribution<Prior> > > 
	SingleFeatureDistributionFactor<GraphType, FactorType,VariableType, Prior>::featureProbs;
template <typename GraphType, typename FactorType, typename VariableType, typename Prior>
Alphabet
	SingleFeatureDistributionFactor<GraphType, FactorType,VariableType, Prior>::alphabet;


template <typename GraphType, typename FactorType, typename VariableType>
class SingleFeatureDistributionFactorSymmetricPrior 
: public SingleFeatureDistributionFactor<GraphType, FactorType,VariableType, SymmetricDirichlet> 
{
	public:
		SingleFeatureDistributionFactorSymmetricPrior(unsigned int n_classes,
				const std::wstring& feature_val) 
			: SingleFeatureDistributionFactor<GraphType, FactorType,VariableType,SymmetricDirichlet>
			  (n_classes, feature_val) {}
		static void finishedLoading(unsigned int num_var_vals, double smoothing) 
		{
			boost::shared_ptr<SymmetricDirichlet> prior = 
				boost::make_shared<SymmetricDirichlet>(smoothing);
			SingleFeatureDistributionFactor<GraphType, FactorType,VariableType,
				SymmetricDirichlet >::finishedLoading(
						num_var_vals, prior);
		}
};

};
#endif

