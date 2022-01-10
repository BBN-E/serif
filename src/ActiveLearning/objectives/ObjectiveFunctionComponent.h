#ifndef _OBJECTIVE_FUNCTION_COMPONENT_H_
#define _OBJECTIVE_FUNCTION_COMPONENT_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include "LearnIt/Eigen/Core"
#define EIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET
#include "LearnIt/Eigen/Sparse"
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(ObjectiveFunctionComponent)

class ObjectiveFunctionComponent {
public:
	virtual double operator()(Eigen::VectorXd& gradient, size_t thread_idx = 0, 
			size_t n_threads = 1) const = 0;
	virtual void snapshot() = 0;
	virtual std::wstring status() const = 0;
};

BSP_DECLARE(InstancewiseObjectiveFunctionComponent)
class InstancewiseObjectiveFunctionComponent : public ObjectiveFunctionComponent {
public:
	virtual double forInstance(int inst, Eigen::SparseVector<double>& gradient) const = 0;
};

#endif
