#ifndef _AT_MOST_N_OF_CLASS_CONSTRAINT_
#define _AT_MOST_N_OF_CLASS_CONSTRAINT_

#include "../../GraphicalModels/pr/Constraint.h"
#include "../PosteriorRegularization.h"
#include <vector>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(AtMostNOfClassConstraint)

class AtMostNOfClassConstraint 
	: public GraphicalModel::SingleWeightConstraint<Document> 
{
public:
	AtMostNOfClassConstraint(const std::vector<unsigned int>& classes, double N);
	void modifyFactors(Document& graph) const;
	void updateBoundAndExpectation(const Document& graph);
protected:
	double _N;
	std::vector<unsigned int> _classes;
};

#endif



