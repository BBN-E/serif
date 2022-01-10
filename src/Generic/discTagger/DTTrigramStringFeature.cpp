// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTTrigramStringFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTTrigramStringFeature *DTTrigramStringFeature::_freeList = 0;

void *DTTrigramStringFeature::operator new(size_t) {
	DTTrigramStringFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTTrigramStringFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {
		//cout << "Allocating DTQuadgramFeature block #" << ++n_blocks << "\n";

        DTTrigramStringFeature *newBlock = static_cast<DTTrigramStringFeature*>
			(::operator new(_block_size * sizeof(DTTrigramStringFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTTrigramStringFeature **next =
				reinterpret_cast<DTTrigramStringFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTTrigramStringFeature **next =
			reinterpret_cast<DTTrigramStringFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTTrigramStringFeature::operator delete(void *object) {
    DTTrigramStringFeature *p = static_cast<DTTrigramStringFeature*>(object);
	DTTrigramStringFeature **next = reinterpret_cast<DTTrigramStringFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

