#include "Generic/common/leak_detection.h"
#include <string>

#include "AnnotatedFeatureRecord.h"

using std::string;

AnnotatedFeatureRecord::AnnotatedFeatureRecord(int feat, bool positive, double expectation)
: idx(feat), positive_annotation(positive), /*frequency(0.0),*/
/*observed(0.0),*/ expectation(expectation)/*, scale(0.0), violated(false)*/ {}

/*void AnnotatedFeatureRecord::observeInSlotFV(int inst, double val) {
	inSlotFV.push_back(inst);
	frequency += val;
}

void AnnotatedFeatureRecord::observeInSentFV(int inst, double val) {
	inSentFV.push_back(inst);
	frequency += val;
}*/
