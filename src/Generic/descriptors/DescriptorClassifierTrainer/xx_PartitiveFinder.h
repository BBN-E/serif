// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_PARTITIVEFINDER_H
#define XX_PARTITIVEFINDER_H

#include "Generic/descriptors/DescriptorClassifierTrainer/PartitiveFinder.h"

class SynNode;

class GenericPartitiveFinder : public PartitiveFinder {
private:
	friend class GenericPartitiveFinderFactory;

public: 

	static const SynNode* getPartitiveHead(const SynNode* node) { return 0; }

};

class GenericPartitiveFinderFactory: public PartitiveFinder::Factory {
	virtual const SynNode* getPartitiveHead(const SynNode* node) {  return GenericPartitiveFinder::getPartitiveHead(node); }
};


#endif
