// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTSexgramIntFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTSexgramIntFeature *DTSexgramIntFeature::_freeList = 0;

void *DTSexgramIntFeature::operator new(size_t) {
	DTSexgramIntFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTSexgramIntFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {
		//cout << "Allocating DTSexgramFeature block #" << ++n_blocks << "\n";

        DTSexgramIntFeature *newBlock = static_cast<DTSexgramIntFeature*>
			(::operator new(_block_size * sizeof(DTSexgramIntFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTSexgramIntFeature **next =
				reinterpret_cast<DTSexgramIntFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTSexgramIntFeature **next =
			reinterpret_cast<DTSexgramIntFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTSexgramIntFeature::operator delete(void *object) {
    DTSexgramIntFeature *p = static_cast<DTSexgramIntFeature*>(object);
	DTSexgramIntFeature **next = reinterpret_cast<DTSexgramIntFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

