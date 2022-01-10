// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTQuadgramIntFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTQuadgramIntFeature *DTQuadgramIntFeature::_freeList = 0;

void *DTQuadgramIntFeature::operator new(size_t) {
	DTQuadgramIntFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTQuadgramIntFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {

        DTQuadgramIntFeature *newBlock = static_cast<DTQuadgramIntFeature*>
			(::operator new(_block_size * sizeof(DTQuadgramIntFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTQuadgramIntFeature **next =
				reinterpret_cast<DTQuadgramIntFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTQuadgramIntFeature **next =
			reinterpret_cast<DTQuadgramIntFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTQuadgramIntFeature::operator delete(void *object) {
    DTQuadgramIntFeature *p = static_cast<DTQuadgramIntFeature*>(object);
	DTQuadgramIntFeature **next = reinterpret_cast<DTQuadgramIntFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

