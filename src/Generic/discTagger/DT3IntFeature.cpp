// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DT3IntFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

DT3IntFeature *DT3IntFeature::_freeList = 0;

void *DT3IntFeature::operator new(size_t) {
	DT3IntFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DT3IntFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {
        DT3IntFeature *newBlock = static_cast<DT3IntFeature*>
			(::operator new(_block_size * sizeof(DT3IntFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DT3IntFeature **next =
				reinterpret_cast<DT3IntFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DT3IntFeature **next =
			reinterpret_cast<DT3IntFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DT3IntFeature::operator delete(void *object) {
    DT3IntFeature *p = static_cast<DT3IntFeature*>(object);
	DT3IntFeature **next = reinterpret_cast<DT3IntFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

