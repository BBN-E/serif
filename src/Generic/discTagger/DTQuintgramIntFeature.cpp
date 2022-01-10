// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTQuintgramIntFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTQuintgramIntFeature *DTQuintgramIntFeature::_freeList = 0;

void *DTQuintgramIntFeature::operator new(size_t) {
	DTQuintgramIntFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTQuintgramIntFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {
		//cout << "Allocating DTQuintgramFeature block #" << ++n_blocks << "\n";

        DTQuintgramIntFeature *newBlock = static_cast<DTQuintgramIntFeature*>
			(::operator new(_block_size * sizeof(DTQuintgramIntFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTQuintgramIntFeature **next =
				reinterpret_cast<DTQuintgramIntFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTQuintgramIntFeature **next =
			reinterpret_cast<DTQuintgramIntFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTQuintgramIntFeature::operator delete(void *object) {
    DTQuintgramIntFeature *p = static_cast<DTQuintgramIntFeature*>(object);
	DTQuintgramIntFeature **next = reinterpret_cast<DTQuintgramIntFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

