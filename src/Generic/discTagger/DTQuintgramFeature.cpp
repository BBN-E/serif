// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTQuintgramFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTQuintgramFeature *DTQuintgramFeature::_freeList = 0;

void *DTQuintgramFeature::operator new(size_t) {
	DTQuintgramFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTQuintgramFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {
		//cout << "Allocating DTQuintFeature block #" << ++n_blocks << "\n";

        DTQuintgramFeature *newBlock = static_cast<DTQuintgramFeature*>
			(::operator new(_block_size * sizeof(DTQuintgramFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTQuintgramFeature **next =
				reinterpret_cast<DTQuintgramFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTQuintgramFeature **next =
			reinterpret_cast<DTQuintgramFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTQuintgramFeature::operator delete(void *object) {
    DTQuintgramFeature *p = static_cast<DTQuintgramFeature*>(object);
	DTQuintgramFeature **next = reinterpret_cast<DTQuintgramFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

