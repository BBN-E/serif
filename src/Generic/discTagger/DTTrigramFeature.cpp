// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTTrigramFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTTrigramFeature *DTTrigramFeature::_freeList = 0;

void *DTTrigramFeature::operator new(size_t) {
	DTTrigramFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTTrigramFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {
		//cout << "Allocating TrigramFeature block #" << ++n_blocks << "\n";

        DTTrigramFeature *newBlock = static_cast<DTTrigramFeature*>
			(::operator new(_block_size * sizeof(DTTrigramFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTTrigramFeature **next =
				reinterpret_cast<DTTrigramFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTTrigramFeature **next =
			reinterpret_cast<DTTrigramFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTTrigramFeature::operator delete(void *object) {
    DTTrigramFeature *p = static_cast<DTTrigramFeature*>(object);
	DTTrigramFeature **next = reinterpret_cast<DTTrigramFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

