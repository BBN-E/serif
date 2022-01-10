// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTQuintgramStringFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTQuintgramStringFeature *DTQuintgramStringFeature::_freeList = 0;

void *DTQuintgramStringFeature::operator new(size_t) {
	DTQuintgramStringFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTQuintgramStringFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {

        DTQuintgramStringFeature *newBlock = static_cast<DTQuintgramStringFeature*>
			(::operator new(_block_size * sizeof(DTQuintgramStringFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTQuintgramStringFeature **next =
				reinterpret_cast<DTQuintgramStringFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTQuintgramStringFeature **next =
			reinterpret_cast<DTQuintgramStringFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTQuintgramStringFeature::operator delete(void *object) {
    DTQuintgramStringFeature *p = static_cast<DTQuintgramStringFeature*>(object);
	DTQuintgramStringFeature **next = reinterpret_cast<DTQuintgramStringFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

