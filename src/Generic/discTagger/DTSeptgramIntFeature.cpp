// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTSeptgramIntFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTSeptgramIntFeature *DTSeptgramIntFeature::_freeList = 0;

void *DTSeptgramIntFeature::operator new(size_t) {
	DTSeptgramIntFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTSeptgramIntFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {
		//cout << "Allocating DTSexgramFeature block #" << ++n_blocks << "\n";

        DTSeptgramIntFeature *newBlock = static_cast<DTSeptgramIntFeature*>
			(::operator new(_block_size * sizeof(DTSeptgramIntFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTSeptgramIntFeature **next =
				reinterpret_cast<DTSeptgramIntFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTSeptgramIntFeature **next =
			reinterpret_cast<DTSeptgramIntFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTSeptgramIntFeature::operator delete(void *object) {
    DTSeptgramIntFeature *p = static_cast<DTSeptgramIntFeature*>(object);
	DTSeptgramIntFeature **next = reinterpret_cast<DTSeptgramIntFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

