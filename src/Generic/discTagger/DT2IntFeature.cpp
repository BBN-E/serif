// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DT2IntFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

DT2IntFeature *DT2IntFeature::_freeList = 0;

void *DT2IntFeature::operator new(size_t) {
	DT2IntFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DT2IntFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {
        DT2IntFeature *newBlock = static_cast<DT2IntFeature*>
			(::operator new(_block_size * sizeof(DT2IntFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DT2IntFeature **next =
				reinterpret_cast<DT2IntFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DT2IntFeature **next =
			reinterpret_cast<DT2IntFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DT2IntFeature::operator delete(void *object) {
    DT2IntFeature *p = static_cast<DT2IntFeature*>(object);
	DT2IntFeature **next = reinterpret_cast<DT2IntFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

