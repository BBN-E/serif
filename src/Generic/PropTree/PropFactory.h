// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef PROPFACTORY_H
#define PROPFACTORY_H

#include "Generic/common/UnrecoverableException.h"
#include "Generic/PropTree/PropNode.h"
#include <boost/shared_ptr.hpp>
#include <string>

// Forward declarations.
class DocTheory;
class SentenceTheory;
class Mention;
class Proposition;
class SynNode;
class TokenSequence;
class PropositionSet;
class MentionSet;
class EntitySet;

class PropFactory {
public:	
	// constructor around sentence
	PropFactory(const DocTheory * doc_theory,
				int sent_no);

	// another sentence constructor, used
	// for slots. You need it because
	// for them, the sentence theory can't
	// necessarily be fetched by the sentence
	// number
	PropFactory(const DocTheory * doc_theory,
				const SentenceTheory* sent_theory,
				int sent_no);

	// constructor around mention
	PropFactory(const DocTheory * doc_theory,
				const Mention * m);
	
	// constructor around proposition
	PropFactory(const DocTheory * doc_theory,
				const Proposition * p,
				int sent_no);
	
	virtual ~PropFactory() {}
	
	// Note: virtual methods can't be called within
	//  a constructor, so we defer actual construction
	//  to the first time one of these is called
	// 
	// these used to be "const PropNodes", but that
	// just means the shared_ptr can't be changed to point
	// to something else, not that the nodes are const, which
	// I think was what was intended.
	// A whole parallel set of typedefs like PropNodes_const
	// should probably be created at one point. ~RMG
	PropNodes & getForestRoots();
	PropNodes & getAllNodes();
	
	// Utilities defining canonical ways to extract predicates from synnodes & a mention
	static std::wstring mentionPredicateString(const DocTheory * dt, const Mention * m,
		bool lowercase=false);
	static std::wstring synnodeString(const TokenSequence * tseq, const SynNode * node,
		bool lowercase=true);
	
protected:
	
	// These methods are available as a place to do heavy lifting
	virtual void constructFromSentence(const DocTheory * doc_theory){
		throw UnrecoverableException("PropFactory::constructFromSentence()",
			"This method must be implemented by subclasses");
	}
	virtual void constructFromMention(const DocTheory * doc_theory, const Mention * ment){
		throw UnrecoverableException("PropFactory::constructFromSentence()",
			"This method must be implemented by subclasses");
	}
	virtual void constructFromProposition(const DocTheory * doc_theory, const Proposition * prop){
		throw UnrecoverableException("PropFactory::constructFromSentence()",
			"This method must be implemented by subclasses");
	}
		
	// Utility for construction of predicates from Mentions
	void extractMentionPredicates(const Mention * m, 
		PropNode::WeightedPredicates & preds) const;
	
	// these are extremely
	// handy to have around
	const int              sent_no;
	const DocTheory      & dt;
	const SentenceTheory & st;
	
	const PropositionSet & pset;
	const MentionSet     & mset;
	const EntitySet      & eset;
	const TokenSequence  & tseq;
	
	// Subclasses write constructed
	// nodes into these
	PropNodes _forest_roots;
	PropNodes _forest_nodes;
	
private:
	const Mention     * _cMent;
	const Proposition * _cProp;
	
	bool  _init_done;
	void _init();
};


#endif
