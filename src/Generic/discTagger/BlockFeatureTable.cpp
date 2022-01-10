// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/BlockFeatureTable.h"
#include "Generic/discTagger/DTFeature.h"

using namespace std;


BlockFeatureTable::BlockFeatureTable(DTTagSet *tagSet) : _tagSet(tagSet) {
	_table = _new FeatureBlockTable(500009);
}

BlockFeatureTable::~BlockFeatureTable() {
	if (_table != 0) {
		FeatureBlockTable::iterator iter = _table->begin();
		while (iter != _table->end()) {
			DTFeatureBlock *block = (*iter).second;
			delete block;
			++iter;
		}
		delete _table;
	}
}

bool BlockFeatureTable::insert(DTFeature *feature, double weight) {
	FeatureBlockTable::iterator entryIter = _table->find(feature);

	if (entryIter == _table->end()) {
		DTFeatureBlock *entry = _new DTFeatureBlock();
		Symbol tag = feature->getTag();
		int tag_index = _tagSet->getTagIndex(tag);
		if (tag_index == -1) {
			for (int j = 0; j < _tagSet->getNTags(); j++) {	
				if (tag == _tagSet->getReducedTagSymbol(j) ||
					tag == _tagSet->getSemiReducedTagSymbol(j))
				{
					tag_index = j;
					entry->set(tag_index, weight);
				}
			}
			if (tag_index == -1) {
				string msg = "Could not find a corresponding tag index for ";
				msg.append(tag.to_debug_string());
				throw InternalInconsistencyException("BlockFeatureTable::insert()", msg.c_str());
			}
		}
		else {
			entry->set(tag_index, weight);
		}
		(*_table)[feature] = entry;
		return true;
	}
	else {
		DTFeatureBlock* entry = (*entryIter).second;
		Symbol tag = feature->getTag();
		int tag_index = _tagSet->getTagIndex(tag);
		if (tag_index == -1) {
			for (int j = 0; j < _tagSet->getNTags(); j++) {	
				if (tag == _tagSet->getReducedTagSymbol(j) ||
					tag == _tagSet->getSemiReducedTagSymbol(j))
				{
					tag_index = j;
					entry->set(tag_index, weight);
				}
			}
			if (tag_index == -1) {
				string msg = "Could not find a corresponding tag index for ";
				msg.append(tag.to_debug_string());
				throw InternalInconsistencyException("BlockFeatureTable::insert()", msg.c_str());
			}
		}
		else {
			entry->set(tag_index, weight);
		}
		return false;
	}
}

/* Note: parray must be at least _tagSet->getNTags() entries long */
// void BlockFeatureTable::load(DTFeature *feature, PWeight *parray) {
// 	for (int i = 0; i < _tagSet->getNTags(); i++) {
// 	    //parray[i] = PWeight(0);
// 	    parray[i].resetToZero();
// 	}
// 	FeatureBlockTable::iterator entryIter = _table->find(feature);
// 	if (entryIter != _table->end()) {
// 		DTFeatureBlock* entry = (*entryIter).second;
// 		entry->load(parray);
// 	}
// }

void BlockFeatureTable::load(DTFeature *feature, double *parray) {
	memset(parray, 0, _tagSet->getNTags()*sizeof(double));
	FeatureBlockTable::iterator entryIter = _table->find(feature);
	if (entryIter != _table->end()) {
		DTFeatureBlock* entry = (*entryIter).second;
		entry->load(parray);
	}
}
