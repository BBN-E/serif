// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTQuadgramFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTQuadgramFeature *DTQuadgramFeature::_freeList = 0;

void *DTQuadgramFeature::operator new(size_t) {
	DTQuadgramFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTQuadgramFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {
		//cout << "Allocating DTQuadgramFeature block #" << ++n_blocks << "\n";

        DTQuadgramFeature *newBlock = static_cast<DTQuadgramFeature*>
			(::operator new(_block_size * sizeof(DTQuadgramFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTQuadgramFeature **next =
				reinterpret_cast<DTQuadgramFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTQuadgramFeature **next =
			reinterpret_cast<DTQuadgramFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTQuadgramFeature::operator delete(void *object) {
    DTQuadgramFeature *p = static_cast<DTQuadgramFeature*>(object);
	DTQuadgramFeature **next = reinterpret_cast<DTQuadgramFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

