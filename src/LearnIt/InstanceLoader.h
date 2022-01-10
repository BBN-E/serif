#include <string>
#include "Generic/common/bsp_declare.h"
BSP_DECLARE(InferenceDataView)
BSP_DECLARE(DataView)
BSP_DECLARE(MultiAlphabet)
class InstanceHashes;
	
struct LoadInstancesReturn {
	LoadInstancesReturn(InferenceDataView_ptr slotView,
		InferenceDataView_ptr sentenceView, DataView_ptr combinedView)
	: slotView(slotView), sentenceView(sentenceView), combinedView(combinedView)
	{}
	InferenceDataView_ptr slotView;
	InferenceDataView_ptr sentenceView;
	DataView_ptr combinedView;
};

class InstanceLoader {
public:
	static LoadInstancesReturn load_instances(MultiAlphabet_ptr alphabet,
		const std::string& feature_vectors_file, int nFeatures, 
		InstanceHashes& instance_hashes);
};

