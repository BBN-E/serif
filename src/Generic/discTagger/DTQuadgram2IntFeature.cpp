// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "Generic/discTagger/DTQuadgram2IntFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTQuadgram2IntFeature *DTQuadgram2IntFeature::_freeList = 0;

void *DTQuadgram2IntFeature::operator new(size_t) {
	DTQuadgram2IntFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTQuadgram2IntFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {

        DTQuadgram2IntFeature *newBlock = static_cast<DTQuadgram2IntFeature*>
			(::operator new(_block_size * sizeof(DTQuadgram2IntFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTQuadgram2IntFeature **next =
				reinterpret_cast<DTQuadgram2IntFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTQuadgram2IntFeature **next =
			reinterpret_cast<DTQuadgram2IntFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTQuadgram2IntFeature::operator delete(void *object) {
    DTQuadgram2IntFeature *p = static_cast<DTQuadgram2IntFeature*>(object);
	DTQuadgram2IntFeature **next = reinterpret_cast<DTQuadgram2IntFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

