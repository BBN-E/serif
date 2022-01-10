// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_GENERICS_FILTER_H
#define en_GENERICS_FILTER_H

#include "Generic/generics/GenericsFilter.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/DebugStream.h"

class SymbolHash;

/**
 * Identify entities as generic, based on a set of syntactic 
 * and link-based rules.
 */
class EnglishGenericsFilter : public GenericsFilter {
private:
  	friend class EnglishGenericsFilterFactory;
public:
	~EnglishGenericsFilter();
	void filterGenerics(DocTheory* docTheory);
private:
	EnglishGenericsFilter();
	// the master algorithm to determine this
	bool _isGeneric(Entity* ent, const EntitySet* ents, 
								const RelationSet* rels);
	// looking specifically at the descriptor mention
	bool _isGenericMention(Mention* ment, const EntitySet* ents);

	// for filling the hash of commonly specific words
	void _loadCommonSpecifics(const char* file);

	////////////////////////////////////////////
	//// These methods are somewhat generic to SynNodes or other things, 
	//// so they may be moved somewhere more common

	// core npa is the root of an np->npa type of node
	const SynNode* _getCoreNPA(const SynNode* node) const;

	// definite things start with a definite article
	bool _isDefinite(const SynNode* node) const;

	// npish = np, npa, nppos, nppro, npp
	bool _isNPish(Symbol sym) const;

	// vish = vb, vbd, vbg, vbn, vbp, vbz
	bool _isVish(Symbol sym) const;

	// sish = s, sinv, sq
	bool _isSish(Symbol sym) const;

	// numeric symbol has numbers and up to one decimal point, 
	// and maybe a leading negative.
	bool _isNumeric(Symbol sym) const;

	// find verb node near the given node
	// sentnum and ents are for determining partitive in one special case
	const SynNode* _getPredicate(const SynNode* node, int sentNum, const EntitySet* ents) const; 
	// do the work for the above
	const SynNode* _getPredicateDriver(const SynNode* node, bool goingDown, int sentNum, const EntitySet* ents) const;

	// routines for returning boolean value and printing info 
	// to the debugstream
	bool _debugReturn(bool decision, const char* reason = "reason unknown", const SynNode* decNode=0, const SynNode* reasonNode=0);

	// arrays of booleans signalling qualities of a sentence. These are
	// created and deleted for each filter run
	bool* _is_sent_hypothetical;
	// i know, it's a weird name, but i wanted to preserve the "is" ness 
	bool* _is_sent_name_bearing;

	// filling methods for the two arrays
	void _fillHypothetical(bool* arr, DocTheory* thry);
	bool _fillHypotheticalSearcher(const SynNode* node);
	void _fillNameBearing(bool* arr, DocTheory* thry);

	// trace up looking for subjunctivity
	bool _isInSubjunctive(const SynNode* node);

	// if node is the object of a pp that is the object of a node with a specific
	// mention, the original node are said to be "contagious" to that one.
	// return 0 if no contagion is found
	const SynNode* _getContagious(const SynNode* node, int sentNum, const EntitySet* ents);

	DebugStream _debug;
	bool _usingCommonSpecifics;
	SymbolHash* _commonSpecifics;
	bool _remove_singleton_descriptors;
	bool FILTER_GENERICS;
};


class EnglishGenericsFilterFactory: public GenericsFilter::Factory {
	virtual GenericsFilter *build() { return _new EnglishGenericsFilter(); } 
};

#endif
