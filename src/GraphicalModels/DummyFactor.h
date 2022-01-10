#ifndef _DUMMY_FACTOR_H_
#define _DUMMY_FACTOR_H_

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

template <typename GraphType, typename FactorType, typename VariableType>
class DummyFactor : public ModifiableUnaryFactor<GraphType, FactorType> {
	public:
		DummyFactor(unsigned int n_classes)
			: GraphicalModel::ModifiableUnaryFactor<GraphType, FactorType>(n_classes) {}

		static void finishedLoading() { }
		static void clearCounts() { }
		void observe(const VariableType& v) { }

		template <typename Dumper>
		static void updateParametersFromCounts(Dumper& dumper) { }

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
			}
	private:
		friend class GraphicalModel::ModifiableUnaryFactor<GraphType, FactorType>;
		double factorPreModImpl(unsigned int assignment) const {
			return 0.0;
		}
};
}

#endif

