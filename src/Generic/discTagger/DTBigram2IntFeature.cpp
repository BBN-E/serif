// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTBigram2IntFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTBigram2IntFeature *DTBigram2IntFeature::_freeList = 0;

void *DTBigram2IntFeature::operator new(size_t) {
	DTBigram2IntFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTBigram2IntFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {
		//cout << "Allocating TrigramFeature block #" << ++n_blocks << "\n";

        DTBigram2IntFeature *newBlock = static_cast<DTBigram2IntFeature*>
			(::operator new(_block_size * sizeof(DTBigram2IntFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTBigram2IntFeature **next =
				reinterpret_cast<DTBigram2IntFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTBigram2IntFeature **next =
			reinterpret_cast<DTBigram2IntFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTBigram2IntFeature::operator delete(void *object) {
    DTBigram2IntFeature *p = static_cast<DTBigram2IntFeature*>(object);
	DTBigram2IntFeature **next = reinterpret_cast<DTBigram2IntFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

