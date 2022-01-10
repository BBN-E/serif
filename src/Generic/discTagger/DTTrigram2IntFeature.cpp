// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTTrigram2IntFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTTrigram2IntFeature *DTTrigram2IntFeature::_freeList = 0;

void *DTTrigram2IntFeature::operator new(size_t) {
	DTTrigram2IntFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTTrigram2IntFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {
		//cout << "Allocating TrigramFeature block #" << ++n_blocks << "\n";

        DTTrigram2IntFeature *newBlock = static_cast<DTTrigram2IntFeature*>
			(::operator new(_block_size * sizeof(DTTrigram2IntFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTTrigram2IntFeature **next =
				reinterpret_cast<DTTrigram2IntFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTTrigram2IntFeature **next =
			reinterpret_cast<DTTrigram2IntFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTTrigram2IntFeature::operator delete(void *object) {
    DTTrigram2IntFeature *p = static_cast<DTTrigram2IntFeature*>(object);
	DTTrigram2IntFeature **next = reinterpret_cast<DTTrigram2IntFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

