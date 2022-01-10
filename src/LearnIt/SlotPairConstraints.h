#pragma once

#include <algorithm>
#include <boost/shared_ptr.hpp>

class SlotPairConstraints;
typedef boost::shared_ptr<SlotPairConstraints> SlotPairConstraints_ptr;

/** A set of constraints on the values for a par of slots of a target relation.
  * Currently the only supported constraint is that two slots must belong to
  * the same entity.
  */
class SlotPairConstraints : private boost::noncopyable {
    public:
        SlotPairConstraints(int a, int b, bool must_corefer, bool must_not_corefer) :
            _first((std::min)(a,b)), _second((std::max)(a,b)), 
            _must_corefer(must_corefer), _must_not_corefer(must_not_corefer) {} 

        size_t first() { return _first; }
        size_t second() { return _second; }
        bool mustCorefer() { return _must_corefer; }
		bool mustNotCorefer() { return _must_not_corefer; }
    private:
        size_t _first, _second;
        bool _must_corefer;
		bool _must_not_corefer;
};
