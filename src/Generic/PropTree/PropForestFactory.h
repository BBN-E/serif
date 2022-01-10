// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef PROPFORESTFACTORY_H
#define PROPFORESTFACTORY_H

#include "PropNode.h"
#include "PropFactory.h"
#include "theories/DocTheory.h"
#include "DocPropForest.h"
#include <list>

class Slot;

class PropForestFactory : public PropFactory {
public:
	PropForestFactory(const DocTheory * dt, int s)
		: PropFactory(dt,s) { }
	PropForestFactory(const DocTheory * dt, const Proposition * p, int s)
		: PropFactory(dt,p,s) { }
	PropForestFactory(const DocTheory * dt, const Mention * n)
		: PropFactory(dt,n) { }
	PropForestFactory(const DocTheory * dt, const SentenceTheory* st,
		int s) : PropFactory(dt, st, s) {}
	static DocPropForest::ptr getDocumentForest(const DocTheory* dt);

protected:
	void constructFromSentence(const DocTheory * doc_theory);
	void constructFromMention(const DocTheory * doc_theory, const Mention * ment);
	void constructFromProposition(const DocTheory * doc_theory, const Proposition * prop);

	typedef std::list< PropNode_ptr > PNodes; // PNodes is a list; PropNodes is a vector. :-/
	typedef std::list< Symbol       > Roles;
	
	const PNodes & resolveProposition(	const Proposition * p,
										const Roles  & opt_roles    = Roles(),
										const PNodes & opt_children = PNodes(),
										const PropNode::WeightedPredicates & opt_preds = PropNode::WeightedPredicates(), 
										const Mention * defined_ment = 0 );

	const PNodes & resolveMention(	const Mention * m,
									const Roles  & opt_roles    = Roles(),
								 const PNodes & opt_children = PNodes() );
		
	void addResolutions(  PNodes & res, const PNodes & new_res,
						 Roles * roles = 0, const Symbol & role = Symbol() );
	Symbol extractCompoundRole(const SynNode * node, Symbol role);
	// delegate constructor converts from PNodes => PropNodes, & allows tracking of construction history
	PropNode_ptr buildPropNode( size_t sent_no, const PropNode::WeightedPredicates & preds,
								const PNodes & children, const Roles & roles,
								const Proposition * prop = NULL, const Mention * ment = NULL, const SynNode * synnode = NULL );
public:
	static bool COLLAPSE_REDUNDANT_MODIFIERS; // =1
	// tracks resolutions of already seen mentions/propositions
	// indexed on mention ID/proposition ID
	std::vector< bool   > resolved_mention;
	std::vector< PNodes > mention_resolutions;
	
	std::vector< bool   > resolved_proposition;
	std::vector< PNodes > proposition_resolutions;
};


#endif // #ifndef PROPFORESTFACTORY_H
