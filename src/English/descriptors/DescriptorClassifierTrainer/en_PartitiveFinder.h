// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_PARTITIVE_FINDER_H
#define EN_PARTITIVE_FINDER_H

#include "Generic/common/Symbol.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/DescriptorClassifierTrainer/PartitiveFinder.h"
#include "Generic/common/SymbolListMap.h"

class SynNode;


class EnglishPartitiveFinder : public PartitiveFinder {
private:
	friend class EnglishPartitiveFinderFactory;

public:
	static const SynNode* getPartitiveHead(const SynNode* node);


private:

	static bool isPartitiveHeadWord(Symbol sym);

	static int _n_partitive_headwords;
	static Symbol _partitiveHeadwords[];

	static void initializeSymbols();
};

class EnglishPartitiveFinderFactory: public PartitiveFinder::Factory {
	virtual const SynNode* getPartitiveHead(const SynNode* node) {  return EnglishPartitiveFinder::getPartitiveHead(node); }
};



#endif
