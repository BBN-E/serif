#ifndef _BAG_OF_WORDS_FACTOR_H_
#define _BAG_OF_WORDS_FACTOR_H_

#include <vector>
#include <cmath>
#include <algorithm>
#include <boost/make_shared.hpp>
#include "Generic/common/bsp_declare.h"
#include "Factor.h"
#include "Alphabet.h"
#include "VectorUtils.h"
#include "distributions/BernoulliDistribution.h"

namespace GraphicalModel {

template <typename GraphType, typename FactorType, 
		 typename VariableType, typename Distribution>
class BagOfWordsFactor : public ModifiableUnaryFactor<GraphType, FactorType> {
	public:
		typedef boost::shared_ptr<Distribution> Distribution_ptr;
		typedef std::vector<Distribution_ptr> Distributions;

		BagOfWordsFactor(unsigned int n_classes)
			: GraphicalModel::ModifiableUnaryFactor<GraphType, FactorType>(n_classes) {}

		const std::vector<unsigned int>& features() const {
			return _features;
		}

		static void finishedLoading(unsigned int num_var_vals,
				bool normalize_by_length,
				const boost::shared_ptr<typename Distribution::Prior>& prior)
		{
			_normalize_by_length = normalize_by_length;
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
				featureProbs[cls]->observeInstance(
						features(), v.marginals()[cls]);
			}
		}

		template <typename Dumper>
		static void updateParametersFromCounts(Dumper& dumper) {
			typename Distributions::iterator distIt = featureProbs.begin();
			for (; distIt!=featureProbs.end(); ++distIt) {
				(*distIt)->digestObservations();
			}

			Distribution::dumpDistributions(dumper.stream(), featureProbs,
					alphabet);

			for (distIt = featureProbs.begin(); distIt != featureProbs.end(); ++distIt) {
				(*distIt)->toLogSpace();
			}
		}

		static Distributions& distributions() { return featureProbs; }

		static unsigned int featureIndex(const std::wstring& feature) {
			return alphabet.featureAlreadyPresent(feature);
		}

		static const std::wstring& featureName(unsigned int idx) {
			return alphabet.name(idx);
		}

		void addFeature(const std::wstring& feature) {
			_features.push_back(alphabet.feature(feature));
		}

		bool hasFeature(unsigned int idx) const {
			return std::find(_features.begin(), _features.end(), idx)
				!= _features.end();
		}

		virtual void dumpOutgoingMessagesToHTML(const std::wstring& rowTitle,
					std::wostream& out, bool verbose=false) const 
			{
				out << L"<tr>";
				out << L"<td><b>" << rowTitle << L"</b></td>";
				MessageVector::const_iterator msg = this->outgoingMessages().begin();
				for (; msg!=this->outgoingMessages().end(); ++msg) {
					(*msg)->toHTMLTDs(out);
				}
				out << L"</tr>";
				if (verbose) {
					std::vector<unsigned int>::const_iterator it =
						_features.begin();
					for (; it!=_features.end(); ++it) {
						out << L"<tr><td><i>" << HTMLEscape(alphabet.name(*it)) 
							<< L"</i></td>";
						for (size_t i=0; i<featureProbs.size(); ++i) {
							out << L"<td><i>" << (*featureProbs[i])(*it)
								<< L"</i></td>";
						}
						out << L"</tr>\n";
					}
				}
			}

	private:
		static Distributions featureProbs;
		static bool _normalize_by_length;
		static GraphicalModel::Alphabet alphabet;
		std::vector<unsigned int> _features;

		friend class GraphicalModel::ModifiableUnaryFactor<GraphType, FactorType>;
		double factorPreModImpl(unsigned int assignment) const {
			double ret = featureProbs[assignment]->probOfObservations(features());
			if (_normalize_by_length && features().size() > 0) {
				ret /= features().size();
			}
			return ret;
		}

		static std::wstring HTMLEscape(const std::wstring& s) {
			std::wstring ret = s;

			for (size_t i = 0; i<ret.length(); ++i) {
				if (ret[i] == L'<') {
					ret[i] = L'[';
				} else if (ret[i] == L'>') {
					ret[i] = L']';
				} else if (ret[i] == L'\'') {
					ret[i] = L' ';
				} else if (ret[i] == L'"') {
					ret[i] = L' ';
				} else if (ret[i] == L'&') {
					ret[i] = L' ';
				}
			}

			return ret;
		}
};


template <typename GraphType, typename FactorType, 
		 typename VariableType, typename Distribution>
std::vector<boost::shared_ptr<Distribution > > 
	BagOfWordsFactor<GraphType, FactorType,VariableType, Distribution>::featureProbs;

template <typename GraphType, typename FactorType, typename VariableType, typename Distribution>
bool BagOfWordsFactor<GraphType, FactorType,VariableType, Distribution>::_normalize_by_length;

template <typename GraphType, typename FactorType, typename VariableType, typename Distribution>
Alphabet BagOfWordsFactor<GraphType, FactorType,VariableType, Distribution>::alphabet;
};

#endif

