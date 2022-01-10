#include "SmoothedL1.h"

using namespace Eigen;

const double SmoothedL1::DEFAULT_EPSILON = 0.001;

double SmoothedL1::SmoothedL1ProbConstant(double prob, double constant,
		const Eigen::SparseVector<double>& featureVector,
		Eigen::VectorXd& gradient, double weight,
		double epsilon)
{
	/*double diff = prob - constant;
	double loss = sqrt(diff * diff + epsilon);
	double neg_prob = 1.0 - prob;

	double coeff = weight * diff/loss * prob * neg_prob;
	
	SparseVector<double>::InnerIterator it(featureVector);
	for (; it; ++it) {
		gradient(it.index()) -= coeff*it.value();
	}
	
	return -weight * loss;*/
	double diff = prob - constant;
	double loss = pow(diff * diff + epsilon, 0.75);
	double denom = pow(diff * diff + epsilon, 0.25);
	double neg_prob = 1.0 - prob;

	double coeff = weight * 1.5*diff/denom * prob * neg_prob;
	
	SparseVector<double>::InnerIterator it(featureVector);
	for (; it; ++it) {
		gradient(it.index()) -= coeff*it.value();
	}
	
	return -weight * loss;
}

double SmoothedL1::SmoothedL1ProbProb(double alpha, double beta,
		const Eigen::SparseVector<double>& alphaFV,
		const Eigen::SparseVector<double>& betaFV,
		Eigen::VectorXd& gradient, double weight,
		double epsilon)
{
	/*double diff = alpha - beta;
	double loss = sqrt(diff * diff + epsilon);
	double common_coeff = weight * diff / loss;

	double alpha_coeff =common_coeff * alpha * (1.0-alpha);
	double beta_coeff = -common_coeff * beta * (1.0-beta);

	for (SparseVector<double>::InnerIterator itA(alphaFV); itA; ++itA) {
		gradient(itA.index()) -= alpha_coeff * itA.value();
	}

	for (SparseVector<double>::InnerIterator itB(betaFV); itB; ++itB) {
		gradient(itB.index()) -= beta_coeff * itB.value();
	}

	return -weight * loss;*/
	double diff = alpha - beta;
	double loss = pow(diff * diff + epsilon, 0.75);
	double denom = pow(diff * diff + epsilon, 0.25);
	double common_coeff = weight * 1.5*diff / denom;

	double alpha_coeff =common_coeff * alpha * (1.0-alpha);
	double beta_coeff = -common_coeff * beta * (1.0-beta);

	for (SparseVector<double>::InnerIterator itA(alphaFV); itA; ++itA) {
		gradient(itA.index()) -= alpha_coeff * itA.value();
	}

	for (SparseVector<double>::InnerIterator itB(betaFV); itB; ++itB) {
		gradient(itB.index()) -= beta_coeff * itB.value();
	}

	return -weight * loss;
}
