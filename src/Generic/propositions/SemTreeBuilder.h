// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SEM_TREE_BUILDER_H
#define SEM_TREE_BUILDER_H

#include <boost/shared_ptr.hpp>

class SynNode;
class SemNode;
class SemBranch;
class SemOPP;
class SemLink;
class MentionSet;

class SemTreeBuilder {
public:
	/** Create and return a new SemTreeBuilder. */
	static SemTreeBuilder *get() { return _factory()->get(); }
	/** Hook for registering new SemTreeBuilder factories */
	struct Factory { virtual SemTreeBuilder *get() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

public:
	virtual ~SemTreeBuilder() {}

	virtual void initialize() =0;

	virtual SemNode *buildSemTree(const SynNode* synNode,
								 const MentionSet *mentionSet) =0;

	virtual bool isNegativeAdverb(const SynNode &node) =0;
	virtual bool isCopula(const SynNode &node) =0;
	virtual bool isModalVerb(const SynNode &node) =0;
	virtual bool canBeAuxillaryVerb(const SynNode &node) =0;

	virtual SemOPP *findAssociatedPredicateInNP(SemLink &link) =0;

	virtual bool uglyBranchArgHeuristic(SemOPP &opp, SemBranch &branch) =0;

	virtual int mapSArgsToLArgs(SemNode **largs, SemOPP &opp,
							   SemNode *ssubj, SemNode *sarg1, SemNode *sarg2,
							   int n_links, SemLink **links) =0;
private:
	static boost::shared_ptr<Factory> &_factory();
};


//#if defined(ENGLISH_LANGUAGE)
//	#include "English/propositions/en_SemTreeBuilder.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/propositions/ch_SemTreeBuilder.h"
//#else
//	#include "Generic/propositions/xx_SemTreeBuilder.h"
//#endif


#endif
