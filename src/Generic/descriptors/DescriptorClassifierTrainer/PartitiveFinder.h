// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PARTITIVEFINDER_H
#define PARTITIVEFINDER_H

#include <boost/shared_ptr.hpp>

class SynNode;

class PartitiveFinder {
public:
	/** Create and return a new PartitiveFinder. */
	static const SynNode* getPartitiveHead(const SynNode* node) { return _factory()->getPartitiveHead(node); }
	/** Hook for registering new PartitiveFinder factories */
	struct Factory { virtual const SynNode* getPartitiveHead(const SynNode* node) = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~PartitiveFinder() {}

//public: 

//	static const SynNode* getPartitiveHead(const SynNode* node);

private:
	static boost::shared_ptr<Factory> &_factory();
};

//#ifdef ENGLISH_LANGUAGE
//	#include "English/descriptors/DescriptorClassifierTrainer/en_PartitiveFinder.h"
//#else
//	#include "Generic/descriptors/DescriptorClassifierTrainer/xx_PartitiveFinder.h"
//#endif

#endif
