// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DT6gramFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DT6gramFeature *DT6gramFeature::_freeList = 0;

void *DT6gramFeature::operator new(size_t) {
	DT6gramFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DT6gramFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {

        DT6gramFeature *newBlock = static_cast<DT6gramFeature*>
			(::operator new(_block_size * sizeof(DT6gramFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DT6gramFeature **next =
				reinterpret_cast<DT6gramFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DT6gramFeature **next =
			reinterpret_cast<DT6gramFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DT6gramFeature::operator delete(void *object) {
    DT6gramFeature *p = static_cast<DT6gramFeature*>(object);
	DT6gramFeature **next = reinterpret_cast<DT6gramFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

