// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTBigramStringFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTBigramStringFeature *DTBigramStringFeature::_freeList = 0;

void *DTBigramStringFeature::operator new(size_t) {
	DTBigramStringFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTBigramStringFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {
		//cout << "Allocating TrigramFeature block #" << ++n_blocks << "\n";

        DTBigramStringFeature *newBlock = static_cast<DTBigramStringFeature*>
			(::operator new(_block_size * sizeof(DTBigramStringFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTBigramStringFeature **next =
				reinterpret_cast<DTBigramStringFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTBigramStringFeature **next =
			reinterpret_cast<DTBigramStringFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTBigramStringFeature::operator delete(void *object) {
    DTBigramStringFeature *p = static_cast<DTBigramStringFeature*>(object);
	DTBigramStringFeature **next = reinterpret_cast<DTBigramStringFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

