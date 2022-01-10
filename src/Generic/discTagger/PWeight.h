// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P_WEIGHT_H
#define P_WEIGHT_H

/** The purpose of this is to wrap the weight in a class that guarantees
  * that it will be initialized to 0 when created in a hash_map.
  */

class PWeight {
private:
	double _value;
	double _sum; // used only in training
public:
	PWeight() : _value(0), _sum(0) {}
	PWeight(double value) : _value(value), _sum(0) {}
	void resetToZero() { _value = 0; _sum = 0; }

	double &operator*() { return _value; }

	double getSum() { return _sum; }
	void addToSum() { _sum += _value; }
	void addToSum(double weight) { _sum += weight*_value; }

	// use for computing lazy summation
	double& getUpdate() { return _sum; }
	double getLazySum(long n_hypotheses) const { return _value * n_hypotheses - _sum; }
};

#endif

