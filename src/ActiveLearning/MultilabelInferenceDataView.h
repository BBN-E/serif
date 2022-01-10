#ifndef _MULTILABEL_INFERENCE_DATA_VIEW_H_
#define _MULTILABEL_INFERENCE_DATA_VIEW_H_

#include "Generic/common/bsp_declare.h"
#include "DataView.h"

BSP_DECLARE(MultilabelInferenceDataView)

class MultilabelInferenceDataView : public InferenceDataView {
public:
	MultilabelInferenceDataView(unsigned int nInstances, unsigned int nFeatures);
	void observeFeaturesForInstance(unsigned int inst, unsigned int type,
		const std::set<unsigned int>& fv);
	unsigned int type(unsigned int inst) const;
private:
	std::vector<unsigned int> _types;
};

#endif
