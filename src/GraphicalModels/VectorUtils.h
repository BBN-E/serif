#ifndef _VECTOR_UTILS_H_
#define _VECTOR_UTILS_H_

#include <vector>
#include <math.h>

inline void zero(std::vector<double>& vec) {
	std::vector<double>::iterator it = vec.begin();
	for (; it!=vec.end(); ++it) {
		*it = 0.0;
	}
}

inline void normalizeLogProbs(std::vector<double>& scores) {
	double mx = *max_element(scores.begin(), scores.end());
	double sum = 0.0;
	std::vector<double>::iterator it = scores.begin();
	for (; it!=scores.end(); ++it) {
		double& x= *it;
		sum += (x=exp(x-mx));
	}
	for (it = scores.begin(); it!=scores.end(); ++it) {
		*it /= sum;
	}
}

inline double logSumExp(const std::vector<double>& v) {
	double mx = *max_element(v.begin(), v.end());
	double sum = 0.0;
	std::vector<double>::const_iterator it = v.begin();
	for (; it!=v.end(); ++it) {
		sum += exp(*it-mx);
	}
	return log(sum) + mx;
}

inline void divide(std::vector<double>& v, double denom) {
	std::vector<double>::iterator it = v.begin();
	for (; it!=v.end(); ++it) {
		*it /= denom;
	}
}

#endif

