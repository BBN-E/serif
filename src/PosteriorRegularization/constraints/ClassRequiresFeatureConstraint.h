#ifndef _CLASS_REQUIRES_FEATURE_CONSTRAINT_H_
#define _CLASS_REQUIRES_FEATURE_CONSTRAINT_H_

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <Generic/common/bsp_declare.h>
#include "../../GraphicalModels/pr/Constraint.h"
#include "../PosteriorRegularization.h"

namespace GraphicalModel {
	template <typename GraphType> class DataSet;
};

class ClassRequiresFeatureConstraint  : public GraphicalModel::SingleWeightConstraint<Document> {
	public:
		ClassRequiresFeatureConstraint(const GraphicalModel::DataSet<Document>& graphs,
				const std::vector<std::wstring>& features,
				const std::vector<unsigned int>& classes, double threshold);
		void modifyFactors(Document& graph) const;
		void updateBoundAndExpectation(const Document& graph);
	private:
		std::vector<bool> _marked;
		std::vector<unsigned int> _classes;

		void cache(const GraphicalModel::DataSet<Document>& graphs, 
				const std::vector<unsigned int>& features, double threshold);
		double phi(unsigned int feature_id) const;
};

#endif

