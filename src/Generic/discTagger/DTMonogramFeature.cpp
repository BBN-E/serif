// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTMonogramFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTMonogramFeature *DTMonogramFeature::_freeList = 0;

void *DTMonogramFeature::operator new(size_t) {
	DTMonogramFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTMonogramFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {

        DTMonogramFeature *newBlock = static_cast<DTMonogramFeature*>
			(::operator new(_block_size * sizeof(DTMonogramFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTMonogramFeature **next =
				reinterpret_cast<DTMonogramFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTMonogramFeature **next =
			reinterpret_cast<DTMonogramFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTMonogramFeature::operator delete(void *object) {
    DTMonogramFeature *p = static_cast<DTMonogramFeature*>(object);
	DTMonogramFeature **next = reinterpret_cast<DTMonogramFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

