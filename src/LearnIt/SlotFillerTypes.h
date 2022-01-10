#pragma once
#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(SlotFiller)
BSP_DECLARE(Target)
typedef std::vector<SlotFiller_ptr> SlotFillers;
typedef boost::shared_ptr<SlotFillers> SlotFillers_ptr;
typedef std::vector<SlotFillers_ptr> SlotFillersVector;

typedef std::map<int, SlotFiller_ptr> SlotFillerMap;
typedef std::pair<int, SlotFiller_ptr> SlotNumAndFiller;

struct targetCompare {
	bool operator() (const Target_ptr lhs, const Target_ptr rhs) const;
};

typedef std::map<Target_ptr, std::map<std::wstring, SlotFillersVector > , targetCompare > TargetToFillers;
