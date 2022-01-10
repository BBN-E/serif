#ifndef _EM_MODEL_H_
#define _EM_MODEL_H_

#include <vector>
#include "Generic/common/bsp_declare.h"
#include "../../GraphicalModels/DataSet.h"

namespace GraphicalModel {
	template <typename GraphType>
	class SupportsEM {
		public:
			SupportsEM() {}
			void initializeRandomly() {
				static_cast<GraphType*>(this)->initializeRandomlyImpl();
			}
			void observe() {
				static_cast<GraphType*>(this)->observeImpl();
			}
			static void clearCounts() {
				GraphType::clearCountsImpl();
			}
	};

	BSP_DECLARE(EMClusteringModel)

template <typename EMAlgorithm, typename GraphType>
class EM {
	public:
		EM() {};

		template <class ReporterClass>
		double em(DataSet<GraphType>& data, unsigned int max_iterations,
				const ReporterClass& reporter) 
		{
			randomInitialization(data, reporter);
			double LL = 0.0;
			for (unsigned int i = 0; i < max_iterations; ++i) {
				LL = e(data);
				reporter.postE(i, LL, data);
				m(data, *reporter.modelDumper(i));
			}
			return LL;
		}

		double e(DataSet<GraphType>& data) {
			return static_cast<EMAlgorithm*>(this)->eImpl(data);
		}

	private:
		double eImpl(DataSet<GraphType> & data) {
			double LL = 0.0;
			for (typename DataSet<GraphType>::GraphVector::iterator gIt = data.graphs.begin();
					gIt != data.graphs.end(); ++gIt)
			{
				(*gIt)->inference();
			}
			return LL;
		}
	
		template <typename ReporterClass>
		void randomInitialization(DataSet<GraphType>& data, const ReporterClass& reporter) {
			for (typename DataSet<GraphType>::GraphVector::iterator gIt = data.graphs.begin();
					gIt != data.graphs.end(); ++gIt)
			{
				(*gIt)->initializeRandomly();
			}
			m(data, *reporter.modelDumper("init"));
		}

		template <typename Dumper>
		void m(DataSet<GraphType>& data, Dumper& dumper) {
			GraphType::clearCounts();
			for (typename DataSet<GraphType>::GraphVector::iterator gIt = data.graphs.begin();
					gIt != data.graphs.end(); ++gIt)
			{
				(*gIt)->observe();
			}
			GraphType::updateParametersFromCounts(dumper);
		}
};
};
#endif

