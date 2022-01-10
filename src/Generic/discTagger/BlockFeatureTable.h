// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef BLOCK_FEATURE_TABLE_H
#define BLOCK_FEATURE_TABLE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTFeature.h"
//#include "Generic/discTagger/PWeight.h"

class BlockFeatureTable {
private:
	class DTFeatureBlock {
	public:

		struct WeightBlock {
			double weight;
			int tag_index;
			WeightBlock *next;
		};

		WeightBlock *_weights;
	
		DTFeatureBlock() : _weights(0) {}; 

		~DTFeatureBlock() { 
			while (_weights != 0) {
				WeightBlock *head = _weights;
				_weights = head->next;
				delete head;
			}
		}

		void set(int index, double value) {
			// let's just assume there won't be any duplicates for now...
			WeightBlock *w = _new WeightBlock;
			w->weight = value;
			w->tag_index = index;
			w->next = _weights;
			_weights = w;
		}

		// void load(PWeight *parray) {
		// 	WeightBlock *iter = _weights;
		// 	while (iter != 0) {
		// 		parray[iter->tag_index] = *(iter->weight);
		// 		iter = iter->next;
		// 	}
		// }

		void load(double *parray) {
			WeightBlock *iter = _weights;
			while (iter != 0) {
				parray[iter->tag_index] = iter->weight;
				iter = iter->next;
			}
		}
	 };

	struct FeatureBlockHash {
        size_t operator()(const DTFeature *feat) const {
            return feat->getHashCodeWithoutTag();
        }
    };

    struct FeatureBlockEquality {

        bool operator()(const DTFeature *f1, const DTFeature *f2) const {
			return ((*f1).equalsWithoutTag((*f2)));
        }
	};

	typedef serif::hash_map<DTFeature*, DTFeatureBlock*, FeatureBlockHash, FeatureBlockEquality> FeatureBlockTable;

	FeatureBlockTable *_table;

public:
	BlockFeatureTable(DTTagSet *tagSet);
	~BlockFeatureTable();

	bool insert(DTFeature *feature, double weight);
	//void load(DTFeature *feature, PWeight *parray);
	void load(DTFeature *feature, double *parray);

private:
	DTTagSet *_tagSet;
};

#endif
