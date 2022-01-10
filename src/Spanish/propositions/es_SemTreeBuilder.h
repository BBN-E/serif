#ifndef ES_SEM_TREE_BUILDER_H
#define ES_SEM_TREE_BUILDER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/propositions/SemTreeBuilder.h"



class SynNode;
class SemMention;
class Symbol;


// For documentation of this class, please see http://speechweb/text_group/default.aspx
class SpanishSemTreeBuilder : public SemTreeBuilder {
private:
	friend class SpanishSemTreeBuilderFactory;

public:

	SpanishSemTreeBuilder() : _treat_nominal_premods_like_names(false) {} 

	// Loads a list of temporal head words and initializes a list of transitive verbs
	void initialize();

	SemNode *buildSemTree(const SynNode *synNode,
								 const MentionSet *mentionSet);

	bool isCopula(const SynNode &node); 
	bool isNegativeAdverb(const SynNode &node);
	bool isModalVerb(const SynNode &node);
	bool canBeAuxillaryVerb(const SynNode &node);


	/** Heuristic for attaching a link that is typically a
	  * prepositional phrase in an NP to a noun predicate in that
	  * NP. */
	SemOPP *findAssociatedPredicateInNP(SemLink &link);

	/** Decides whether a branch (clause) under an OPP is an argument
	  * (as in "I decided to leave BBN", where "to leave BBN" is an argument
	  * to "decided") or a separate clause. It currently works by checking
	  * for a comma. */
	bool uglyBranchArgHeuristic(SemOPP &parent, SemBranch &child);

	/** Map "syntactic" arguments to "logical" arguments. Depending on 
	  * whether we're looking at an active or passive verb, this takes
	  * the known arguments to the OPP in their syntactic order and
	  * populates largs as follows:
	  * largs[0]: logical subject (the "by X" of a passive verb)
	  * largs[1]: logical object
	  * largs[2]: logical "indirect" object ("gave X the book")
	  * largs[3, 4, ...]: the links, other than the "by X" of a passive
	  */
	int mapSArgsToLArgs(SemNode **largs, SemOPP &opp,
							   SemNode *ssubj, SemNode *sarg1, SemNode *sarg2,
							   int n_links, SemLink **links);

private:
	void initializeSymbols();
	bool _treat_nominal_premods_like_names;
	
	SemNode *buildSemSubtree(const SynNode *synNode,
									const MentionSet *mentionSet);

	SemNode *analyzeChildren(const SynNode *synNode,
									const MentionSet *mentionSet);
	SemNode *analyzeOtherChildren(const SynNode *synNode,
		const SynNode *exception, const MentionSet *mentionSet);

//	static SemNode *findBySynNode(const SynNode *goal, int max_depth);

	const SynNode* findAmongChildren(const SynNode *node,
											const Symbol *tags);

	bool isPassiveVerb(const SynNode &verb);
	bool isKnownTransitiveVerb(const SynNode &verb);
	static bool isLocativePronoun(const SynNode &synNode);

	/** is VP a conjunction of 2 or more VPs */
	bool isVPList(const SynNode &node);

	/** Extracts state/nation from "New Delhi, India" constructions,
	  * or 0 if the construction doesn't match the profile. */
	const SynNode *getStateOfCity(const SynNode *synNode,
										 const MentionSet *mentionSet);

	/** Process "get rid of" */
	SemNode *processRidOf(const SynNode *synNode,
								 const MentionSet *mentionSet);

	/** For "the Tokyo-based bank" */
	SemNode *applyBasedHeuristic(SemNode *children);

	/** This is invoked for all name mentions and some other mentions.
	  * It decides what to do with the other stuff in the mention and
	  * modifies mentionNode accordingly. For "IBM president Bob Smith",
	  * it applies president as a noun predicate, and attaches IBM to that
	  * predicate under a premod link.
	  */
	void applyNameNounNameHeuristic(SemMention &mentionNode,
										   SemNode *children,
										   bool contains_name,
										   const MentionSet *mentionSet);

	void moveObjectsUnderVerbs(SemNode* semChildren);
};

class SpanishSemTreeBuilderFactory: public SemTreeBuilder::Factory {
	SpanishSemTreeBuilder* utils;
	virtual SemTreeBuilder *get() { 
		if (utils == 0) {
			utils = _new SpanishSemTreeBuilder(); 
		} 
		return utils;
	}

public:
	SpanishSemTreeBuilderFactory() { utils = 0;}
};



#endif
