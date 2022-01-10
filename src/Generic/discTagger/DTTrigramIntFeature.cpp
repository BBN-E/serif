// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTTrigramIntFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTTrigramIntFeature *DTTrigramIntFeature::_freeList = 0;

void *DTTrigramIntFeature::operator new(size_t) {
	DTTrigramIntFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTTrigramIntFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {

        DTTrigramIntFeature *newBlock = static_cast<DTTrigramIntFeature*>
			(::operator new(_block_size * sizeof(DTTrigramIntFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTTrigramIntFeature **next =
				reinterpret_cast<DTTrigramIntFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTTrigramIntFeature **next =
			reinterpret_cast<DTTrigramIntFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTTrigramIntFeature::operator delete(void *object) {
    DTTrigramIntFeature *p = static_cast<DTTrigramIntFeature*>(object);
	DTTrigramIntFeature **next = reinterpret_cast<DTTrigramIntFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

