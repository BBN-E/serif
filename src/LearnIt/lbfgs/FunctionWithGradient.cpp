#include "Generic/common/leak_detection.h"
// AJF: Inserted the following line in order to silence a bogus warning:
//   warning C4996: 'std::copy': Function call with parameters that may be unsafe - this call relies on the caller 
//   to check that the passed values are correct. To disable this warning, use -D_SCL_SECURE_NO_WARNINGS. See 
//   documentation on how to use Visual C++ 'Checked Iterators'
// Rather than use the suggested "-D_SCL_SECURE_NO_WARNINGS", I used "#pragma warning(disable:4996)", since we
// do that elsewhere in the codebase.
#pragma warning(disable:4996) 
#include "FunctionWithGradient.h"

using namespace Eigen;

FunctionWithGradient::FunctionWithGradient(int n_params) 
: _dirty(true), _params(VectorXd::Zero(n_params)),
_gradient(VectorXd::Zero(n_params))
{
}

size_t FunctionWithGradient::nParams() const {
	return _params.size();
}

void FunctionWithGradient::parameters(VectorXd& dest) const { 
	dest = _params; 
}

const VectorXd& FunctionWithGradient::parameters() const { 
	return _params; 
}

void FunctionWithGradient::setParams(const VectorXd& newParams) { 
	_params = newParams;
	_dirty = true;
}

void FunctionWithGradient::setParam(unsigned int idx, double val) {
	_params[idx] = val;
	_dirty = true;
}

void FunctionWithGradient::incrementParams(const Eigen::SparseVector<double>& inc,
										   double eta) {
	SparseVector<double>::InnerIterator it(inc);
	for (; it; ++it) {
		_params(it.index()) += eta * it.value();
	}
	_dirty = true;
}

void FunctionWithGradient::incrementParam(int param, double amount) {
	_params.coeffRef(param) += amount;
	_dirty = true;
}

double FunctionWithGradient::value() const {
	updateIfNeeded();
	return _value;
}

void FunctionWithGradient::gradient(VectorXd& dest) const 
{ 
	updateIfNeeded(); dest = _gradient;
}

const VectorXd& FunctionWithGradient::gradient() const {
	updateIfNeeded();
	return _gradient;
}

void FunctionWithGradient::updateIfNeeded() const { 
	if (_dirty) { 
		recalc(_params, _value, _gradient); 
		_dirty = false; 
	} 
}

void FunctionWithGradient::setDirty()  {
	_dirty = true;
}

void FunctionWithGradient::_test_clear_params() {
	_dirty = true;
	_params.setZero();
}
