// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTBigramFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTBigramFeature *DTBigramFeature::_freeList = 0;

void *DTBigramFeature::operator new(size_t) {
	DTBigramFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTBigramFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {
//		cout << "Allocating BigramFeature block #" << ++n_blocks << "\n";

        DTBigramFeature *newBlock = static_cast<DTBigramFeature*>
			(::operator new(_block_size * sizeof(DTBigramFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTBigramFeature **next =
				reinterpret_cast<DTBigramFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTBigramFeature **next =
			reinterpret_cast<DTBigramFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTBigramFeature::operator delete(void *object) {
    DTBigramFeature *p = static_cast<DTBigramFeature*>(object);
	DTBigramFeature **next = reinterpret_cast<DTBigramFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

