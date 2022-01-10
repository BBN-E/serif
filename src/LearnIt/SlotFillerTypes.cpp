#include "LearnIt/Target.h"
#include "LearnIt/SlotFillerTypes.h"

bool targetCompare::operator() (const Target_ptr lhs, const Target_ptr rhs) const {
	return *lhs < *rhs;
}
