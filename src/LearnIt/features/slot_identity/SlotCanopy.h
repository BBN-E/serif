#pragma once
#include <map>
#include <vector>
#include <string>
#include <set>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "LearnIt/SlotFillerTypes.h"
#include "LearnIt/features/CanopyUtils.h"

class DocTheory;

BSP_DECLARE(SlotCanopy)
BSP_DECLARE(SlotConstraints)
BSP_DECLARE(SlotsIdentityFeature)

class SlotCanopy {
public:
	SlotCanopy(const SlotConstraints& constraints, int slot, 
		const std::vector<SlotsIdentityFeature_ptr>& features);
	bool unconstraining() const;
	void potentialMatches(const SlotFillers& fillers,
		const DocTheory* docTheory, std::set<int>& output);
private:
	typedef CanopyUtils<std::wstring> StringCanopy;
	void setUnconstraining(const SlotConstraints& constraints, int slot);
	StringCanopy::KeyToFeatures _name_to_feature;
	bool _unconstraining;
};
