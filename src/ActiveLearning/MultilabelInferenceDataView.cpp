#include "Generic/common/leak_detection.h"
#include "MultilabelInferenceDataView.h"

MultilabelInferenceDataView::MultilabelInferenceDataView(
	unsigned int nInstances, unsigned int nFeatures) 
: InferenceDataView(nInstances, nFeatures), _types(nInstances) 
{}

void MultilabelInferenceDataView::observeFeaturesForInstance(unsigned int inst,
		 unsigned int type, const std::set<unsigned int>& fv) {
	DataView::observeFeaturesForInstance(inst, fv);
	_types[inst] = type;
}

unsigned int MultilabelInferenceDataView::type(unsigned int inst) const {
	return _types[inst];
}
