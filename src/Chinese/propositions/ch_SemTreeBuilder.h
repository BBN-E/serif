#ifndef CH_SEM_TREE_BUILDER_H
#define CH_SEM_TREE_BUILDER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/propositions/SemTreeBuilder.h"


class SynNode;
class SemMention;
class Symbol;



class ChineseSemTreeBuilder : public SemTreeBuilder {
private:
	friend class ChineseSemTreeBuilderFactory;

public:
	void initialize();

	SemNode *buildSemTree(const SynNode *synNode,
								 const MentionSet *mentionSet);

	bool isCopula(const SynNode &node);

	bool isNegativeAdverb(const SynNode &node);
	bool isModalVerb(const SynNode &node);
	bool canBeAuxillaryVerb(const SynNode &node);
	bool isTemporalNP(const SynNode &node);
	bool isLocationNP(const SynNode &node); 

	SemOPP *findAssociatedPredicateInNP(SemLink &link);

	bool uglyBranchArgHeuristic(SemOPP &parent, SemBranch &child);

	int mapSArgsToLArgs(SemNode **largs, SemOPP &opp,
							   SemNode *ssubj, SemNode *sarg1, SemNode *sarg2,
							   int n_links, SemLink **links);

private:
	SemNode *buildSemSubtree(const SynNode *synNode,
									const MentionSet *mentionSet);

	SemNode *analyzeChildren(const SynNode *synNode,
									const MentionSet *mentionSet);

	bool isList(Symbol memberTag, const SynNode &node);

	void applyNameNounNameHeuristic(SemMention &mentionNode,
										   SemNode *children,
										   const MentionSet *mentionSet);

    //static int analyzeChildrenStackDepth;  // We use this to avoid stack overflows from pathalogical input sentences
    //static int maxAnalyzeChildrenStackDepth;
};

class ChineseSemTreeBuilderFactory: public SemTreeBuilder::Factory {
	ChineseSemTreeBuilder* utils;
	virtual SemTreeBuilder *get() { 
		if (utils == 0) {
			utils = _new ChineseSemTreeBuilder(); 
		} 
		return utils;
	}


public:
	ChineseSemTreeBuilderFactory() { utils = 0;}

};



#endif
