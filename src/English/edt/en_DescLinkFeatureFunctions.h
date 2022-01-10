// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_DESCLINK_FEATURE_FUNCTIONS_H
#define EN_DESCLINK_FEATURE_FUNCTIONS_H

#include "Generic/edt/xx_DescLinkFeatureFunctions.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

class EnglishDescLinkFeatureFunctions: public GenericDescLinkFeatureFunctions {
public:
	// See DescLinkFeatureFunctions.h for method documentation.
	static const SynNode* getNumericMod(const Mention *ment);
	
	static std::vector<const Mention*> getModNames(const Mention *ment);
	
	static bool hasMod(const Mention *ment);
	
	static std::vector<Symbol> getMods(const Mention* ment);

	// looking specifically at the descriptor mention
	static bool _isGenericMention(const Mention* ment, const EntitySet* ents);

	// definite things start with a definite article
	static bool _isDefinite(const SynNode* node);
private:
	// core npa is the root of an np->npa type of node
	static const SynNode* _getCoreNPA(const SynNode* node);
	// npish = np, npa, nppos, nppro, npp
	static bool _isNPish(Symbol sym);
	// numeric symbol has numbers and up to one decimal point, 
	// and maybe a leading negative.
	static bool _isNumeric(Symbol sym);
	// trace up looking for subjunctivity
	static bool _isInSubjunctive(const SynNode* node);
	// if node is the object of a pp that is the object of a node with a specific
	// mention, the original node are said to be "contagious" to that one.
	// return 0 if no contagion is found
	static const SynNode* _getContagious(const SynNode* node, int sentNum, const EntitySet* ents);
	// find verb node near the given node
	// sentnum and ents are for determining partitive in one special case
	static const SynNode* _getPredicate(const SynNode* node, int sentNum, const EntitySet* ents); 
	// do the work for the above
	static const SynNode* _getPredicateDriver(const SynNode* node, bool goingDown, int sentNum, const EntitySet* ents);
	// vish = vb, vbd, vbg, vbn, vbp, vbz
	static bool _isVish(Symbol sym);
	// sish = s, sinv, sq
	static bool _isSish(Symbol sym);

};
#endif
