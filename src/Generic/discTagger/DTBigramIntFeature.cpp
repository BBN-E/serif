// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTBigramIntFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTBigramIntFeature *DTBigramIntFeature::_freeList = 0;

void *DTBigramIntFeature::operator new(size_t) {
	DTBigramIntFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTBigramIntFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {

        DTBigramIntFeature *newBlock = static_cast<DTBigramIntFeature*>
			(::operator new(_block_size * sizeof(DTBigramIntFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTBigramIntFeature **next =
				reinterpret_cast<DTBigramIntFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTBigramIntFeature **next =
			reinterpret_cast<DTBigramIntFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTBigramIntFeature::operator delete(void *object) {
    DTBigramIntFeature *p = static_cast<DTBigramIntFeature*>(object);
	DTBigramIntFeature **next = reinterpret_cast<DTBigramIntFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

