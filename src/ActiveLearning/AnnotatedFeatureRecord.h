#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(AnnotatedFeatureRecord)

// feature annotations
class AnnotatedFeatureRecord {
public:
	AnnotatedFeatureRecord(int feat, bool positive, double expectation);
	bool positive_annotation;
	int idx;
	//double frequency;
	/*double observed;*/
	double expectation;
/*	double scale;
	bool violated;*/
	/*std::vector<int> inSentFV;
	std::vector<int> inSlotFV;
	void observeInSlotFV(int inst, double val);
	void observeInSentFV(int inst, double val);*/
};
