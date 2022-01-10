// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTSeptgramFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTSeptgramFeature *DTSeptgramFeature::_freeList = 0;

void *DTSeptgramFeature::operator new(size_t) {
	DTSeptgramFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTSeptgramFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {
		//cout << "Allocating DTQuintFeature block #" << ++n_blocks << "\n";

        DTSeptgramFeature *newBlock = static_cast<DTSeptgramFeature*>
			(::operator new(_block_size * sizeof(DTSeptgramFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTSeptgramFeature **next =
				reinterpret_cast<DTSeptgramFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTSeptgramFeature **next =
			reinterpret_cast<DTSeptgramFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTSeptgramFeature::operator delete(void *object) {
    DTSeptgramFeature *p = static_cast<DTSeptgramFeature*>(object);
	DTSeptgramFeature **next = reinterpret_cast<DTSeptgramFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

