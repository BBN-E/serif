#ifndef _AT_LEAST_N_OF_CLASS_IF_FEATURE_CONSTRAINT_H_
#define _AT_LEAST_N_OF_CLASS_IF_FEATURE_CONSTRAINT_H_

#include "../../GraphicalModels/pr/Constraint.h"
#include "../PosteriorRegularization.h"
#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"

namespace GraphicalModel {
	template <typename GraphType> class DataSet;
};

class AtLeastNOfClassIfFeatureConstraint 
	: public GraphicalModel::SingleWeightConstraint<Document> 
{
public:
	AtLeastNOfClassIfFeatureConstraint(const GraphicalModel::DataSet<Document>& graphs,
			const std::vector<unsigned int> classes,
			const std::vector<std::wstring>& features, double N);
	void modifyFactors(Document& graph) const;
	void updateBoundAndExpectation(const Document& graph);
protected:
	std::vector<bool> _marked;
	std::vector<unsigned int> _classes;
	std::vector<double> _cachedBounds;

	double _N;
	void cache(const GraphicalModel::DataSet<Document>& graphs, 
			const std::vector<unsigned int>& features,
			double threshold);
};

#endif

