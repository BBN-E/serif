#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#pragma warning(disable:4996) // disable Microsoft's warnings about boost

#include <iostream>
#include "RosenbrocksFunction.h"

using namespace boost::numeric::ublas;

void calc_rosenbrock(const Eigen::VectorXd& params, double& value, 
					 Eigen::VectorXd& gradient) 
{
	const double a = params(0);
	const double b = params(1);

	value = 100.0 * pow(b - a*a, 2) + pow(1-a, 2);
	gradient(0) = -400.0*a*(b-a*a) - 2 * (1-a);
	gradient(1) = 200 * (b-a*a);
}

void RosenbrocksFunction::recalc(const Eigen::VectorXd& params, double& value,
								 Eigen::VectorXd& gradient) const
{
	calc_rosenbrock(params, value, gradient);
	_lastVal = value;
}

void NegativeRosenbrocksFunction::recalc(const Eigen::VectorXd& params, double& value,
										 Eigen::VectorXd& gradient) const
{
	calc_rosenbrock(params, value, gradient);
	value = -value;
	gradient *= -1.0;
	_lastVal = value;
}

bool RosenbrocksFunction::postIterationHook() {
	SessionLogger::info("rosenbrock_progress") 
		<< "f(<" << parameters()(0) << ", " << parameters()(1) << ">) = " << _lastVal 
		<< "; f'=<" << gradient()(0) << ", " << gradient()(1) << ")" << std::endl;

	return false;
}

bool NegativeRosenbrocksFunction::postIterationHook() {
	SessionLogger::info("rosenbrock_progress") 
		<< "f(<" << parameters()(0) << ", " << parameters()(1) << ">) = " << _lastVal
		<< "; f'=<" << gradient()(0) << ", " << gradient()(1) << ")" << std::endl;

	return false;
}
