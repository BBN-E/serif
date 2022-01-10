// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTVariableSizeFeature.h"

using namespace std;


#ifdef ALLOCATION_POOLING

static int n_blocks = 0;
DTVariableSizeFeature *DTVariableSizeFeature::_freeList = 0;

void *DTVariableSizeFeature::operator new(size_t) {
	DTVariableSizeFeature *p = _freeList;
    if (p) {
        _freeList = reinterpret_cast<DTVariableSizeFeature*>(
						const_cast<DTFeatureType*>(p->_featureType));
    } else {
		//cout << "Allocating VariableSizeFeature block #" << ++n_blocks << "\n";

        DTVariableSizeFeature *newBlock = static_cast<DTVariableSizeFeature*>
			(::operator new(_block_size * sizeof(DTVariableSizeFeature)));

		for (size_t i = 1; i < _block_size - 1; i++) {
			DTVariableSizeFeature **next =
				reinterpret_cast<DTVariableSizeFeature**>(
					const_cast<DTFeatureType**>(&newBlock[i]._featureType));
			*next = &newBlock[i + 1];
		}
		DTVariableSizeFeature **next =
			reinterpret_cast<DTVariableSizeFeature**>(
				const_cast<DTFeatureType**>(
					&newBlock[_block_size - 1]._featureType));
		*next = 0;

        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}

void DTVariableSizeFeature::operator delete(void *object) {
    DTVariableSizeFeature *p = static_cast<DTVariableSizeFeature*>(object);
	DTVariableSizeFeature **next = reinterpret_cast<DTVariableSizeFeature**>(
								const_cast<DTFeatureType**>(&p->_featureType));
	*next = _freeList;
    _freeList = p;
}

#endif

