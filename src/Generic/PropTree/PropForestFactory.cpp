// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/PropTree/PropForestFactory.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/SynNode.h"

// Class variable parameterizing a modifier-collapsing special case

bool PropForestFactory::COLLAPSE_REDUNDANT_MODIFIERS = 1;


// Definitional propositions (noun-phrases)
inline bool is_def( const Proposition::PredType & t ){
	return	t == Proposition::NOUN_PRED		||
			t == Proposition::PRONOUN_PRED	||
			t == Proposition::SET_PRED		||
			t == Proposition::LOC_PRED		||
			t == Proposition::SET_PRED;
}
inline bool is_noun( const Proposition::PredType & t ){
	return	t == Proposition::NOUN_PRED;
}
// Actions
inline bool is_verb( const Proposition::PredType & t ){
	return	t == Proposition::VERB_PRED		||
			t == Proposition::COPULA_PRED	||
			t == Proposition::COMP_PRED;
}
// Modifiers of definitional propositions
inline bool is_mod( const Proposition::PredType & t ){
	return t == Proposition::MODIFIER_PRED;
}
// Modifiers of definitional propositions
inline bool is_poss( const Proposition::PredType & t ){
	return t == Proposition::POSS_PRED;
}
// Name spans (not used)
inline bool is_name( const Proposition::PredType & t ){
	return t == Proposition::NAME_PRED;
}

// Constructor for an entire sentence

void PropForestFactory::constructFromSentence(const DocTheory * doc_theory)
{
	resolved_mention.resize(mset.getNMentions(), 0);
	resolved_proposition.resize(pset.getNPropositions(), 0);
	mention_resolutions.resize(mset.getNMentions());
	proposition_resolutions.resize(pset.getNPropositions());

	// enumerate all verb props
	std::vector<const Proposition*> vprops;
	for(int i = 0; i != pset.getNPropositions(); i++){
		if(is_verb(pset.getProposition(i)->getPredType()))
			vprops.push_back(pset.getProposition(i));
	}
	
	// remove vprops appearing as children of other props
	for(int i = 0; i != pset.getNPropositions(); i++){
		const Proposition * p = pset.getProposition(i);
		// enumerate children
		for(int c = 0; c != p->getNArgs(); c++){
			if(p->getArg(c)->getType() != Argument::PROPOSITION_ARG) continue;
			const Proposition * cp = p->getArg(c)->getProposition();
			
			// set to null in vprops, if present
			std::vector<const Proposition*>::iterator it = find(
				vprops.begin(), vprops.end(), cp);
			if(it != vprops.end()) *it = 0;
		}
	}
	
	// resolve all root-most vprops
	for(size_t i = 0; i != vprops.size(); i++){
		if(!vprops[i]) continue;
		
		const PNodes & res = resolveProposition(vprops[i]);
		_forest_roots.insert(_forest_roots.end(), res.begin(), res.end());
	}
	
	// walk through the mention-set, and resolve any that haven't been
	for(int i = 0; i != mset.getNMentions(); i++){
		const Mention * ment = mset.getMention(i);
		while(ment->getParent()) ment = ment->getParent();
		if(ment->getMentionType() == Mention::NONE) continue;
		if(resolved_mention[ment->getIndex()])      continue;
		
		const PNodes & res = resolveMention(ment);
		_forest_roots.insert(_forest_roots.end(), res.begin(), res.end());
	}
	return;
}


DocPropForest::ptr PropForestFactory::getDocumentForest(
		const DocTheory* dt) {
	DocPropForest::ptr ret(_new DocPropForest(dt));

	for (int i=0; i<dt->getNSentences(); ++i) {
		PropForestFactory sentFact(dt, i);
		(*ret)[i]=PropNodes_ptr(_new PropNodes(sentFact.getForestRoots().begin(),
			sentFact.getForestRoots().end()));
	}

	return ret;
}


// Constructor for a single Mention

void PropForestFactory::constructFromMention(const DocTheory * doc_theory,  const Mention * ment)
{
	// prepare pre-sized arrays
	resolved_mention.resize(this->mset.getNMentions(), 0);
	resolved_proposition.resize(this->pset.getNPropositions(), 0);
	mention_resolutions.resize(this->mset.getNMentions());
	proposition_resolutions.resize(this->pset.getNPropositions());
	
	const PNodes & res = resolveMention(ment);
	this->_forest_roots.insert(this->_forest_roots.end(), res.begin(), res.end());
}

// Constructor for a single Proposition

void PropForestFactory::constructFromProposition(const DocTheory * doc_theory, const Proposition * prop)
{
	// prepare pre-sized arrays
	resolved_mention.resize(this->mset.getNMentions(), 0);
	resolved_proposition.resize(this->pset.getNPropositions(), 0);
	mention_resolutions.resize(this->mset.getNMentions());
	proposition_resolutions.resize(this->pset.getNPropositions());
	
	const PNodes & res = resolveProposition(prop);
	this->_forest_roots.insert(this->_forest_roots.end(), res.begin(), res.end());
}


const PropForestFactory::PNodes & PropForestFactory::resolveProposition(const Proposition * p,
																		const Roles  & opt_roles    /* = Roles() */,
																		const PNodes & opt_children /* = PNodes() */,
																		const PropNode::WeightedPredicates & opt_preds /* = WeightedPredicates() */,
																		const Mention * defined_ment /* = 0 */)
{
	// already been here?
	if( resolved_proposition[ p->getIndex() ] )
		return proposition_resolutions[ p->getIndex() ];
	
	bool is_p_verb = is_verb(p->getPredType());
	bool is_p_mod  = is_mod(p->getPredType());
	
	// Step 1: Gather predicates & recursive resolutions of children
	
	PropNode::WeightedPredicates preds(opt_preds);
	// if we're a verb or modifier, our proposition captures predicates.
	if( (is_p_verb || is_p_mod) && !p->getPredSymbol().is_null() ){
		
		bool neg = p->getNegation() != NULL;
		Symbol type = is_p_verb ? Predicate::VERB_TYPE : Predicate::MOD_TYPE;

		if (Predicate::validPredicate(p->getPredSymbol()))
			preds[Predicate(type, p->getPredSymbol(), neg)] = 1.0f;
	}
	
	Roles roles(opt_roles); PNodes children(opt_children);
	
	// add an attached particle
	if( p->getParticle() ){
		PropNode::WeightedPredicates wpreds;
		wpreds[Predicate(Symbol(L"particle"), synnodeString(&this->tseq, p->getParticle()), false)]=1.0f;
		
		roles.push_back(Symbol(L"<particle>"));
		children.push_back( buildPropNode(this->sent_no, wpreds, PNodes(), Roles(), 0, 0, p->getParticle()) );
	}
	
	// add an attached adverb
	if( p->getAdverb() ){
		PropNode::WeightedPredicates wpreds;
		wpreds[Predicate(Symbol(L"adv"), synnodeString(&this->tseq, p->getAdverb()), false)]=1.0f;
		
		roles.push_back(Symbol(L"<adv>"));
		children.push_back( buildPropNode(this->sent_no, wpreds, PNodes(), Roles(), 0, 0, p->getAdverb()) );
	}
	
	// add an attached modal
	if( p->getModal() ){
		PropNode::WeightedPredicates wpreds;
		wpreds[Predicate(Predicate::MODAL_TYPE, synnodeString(&this->tseq, p->getModal()), false)]=1.0f;
		
		roles.push_back(Symbol(L"<modal>"));
		children.push_back( buildPropNode(this->sent_no, wpreds, PNodes(), Roles(), 0, 0, p->getModal()) );
	}

	// recursively construct children & roles
	for(int a_it = 0; a_it != p->getNArgs(); a_it++){
		const Argument & a(*p->getArg(a_it));
		
		// avoid no-op recursive call on mod <ref> args; not neccesary for correctness
		if( is_p_mod && a.getRoleSym() == Argument::REF_ROLE ) continue;
		
		if( a.getType() == Argument::MENTION_ARG ){
			Symbol role = extractCompoundRole(a.getMention(&this->mset)->getNode(), a.getRoleSym());
			addResolutions(children, resolveMention(a.getMention(&this->mset)), &roles, role);
		}
		if( a.getType() == Argument::PROPOSITION_ARG ){
			Symbol role = extractCompoundRole(a.getProposition()->getPredHead(), a.getRoleSym());
			addResolutions(children, resolveProposition(a.getProposition()), &roles, role);
		}
	}
	
	// Step 2: Construct representatitve PropNodes
	
	PNodes & prop_res = proposition_resolutions[p->getIndex()];
	
	// if we're a modifier, and there's a mention with the
	// same head, prefer to use that mention's resolution,
	// passing any children of the modifier proposition
	if( is_p_mod ){
		
		for(int i = 0; i != this->mset.getNMentions(); i++){
			const Mention * ment = this->mset.getMention(i);
			if(ment->getHead() != p->getPredHead()) continue;
			
			const PNodes & ment_res = resolveMention(ment, roles, children);
			prop_res.insert(prop_res.end(), ment_res.begin(), ment_res.end());
		}
	}
	
	// if we've got material & we haven't yet,
	// create a default PropNode for this Proposition
	if( prop_res.empty() && (!preds.empty() || !children.empty()) ){
		prop_res.push_back(
			buildPropNode(this->sent_no, preds, children, roles, p, defined_ment));
	}
	
	resolved_proposition[p->getIndex()] = true;
	return prop_res;
}


const PropForestFactory::PNodes & PropForestFactory::resolveMention(const Mention * m,
																	const Roles  & opt_roles    /* = Roles() */,
																	const PNodes & opt_children /* = PNodes() */)
{
	// already been here?
	if( resolved_mention[m->getIndex()] )
		return mention_resolutions[m->getIndex()];
	
	// guard, as we may recursively visit this mention again
	// as a definitional proposition <ref> argument;
	resolved_mention[m->getIndex()] = true;
	
	PropNode::WeightedPredicates preds; extractMentionPredicates(m, preds);
	Roles roles(opt_roles); PNodes children(opt_children);
	
	// find & resolve all mention modifiers, which
	// resolve as children playing a <mod> role
	for( int m_it = 0; m_it != this->pset.getNPropositions(); m_it++ ){
		const Proposition * mprop = this->pset.getProposition(m_it);
		
		// look for propositions modifying this mention.
		// if it's not an mprop or it doesn't touch this mention then skip it
		if (mprop->getNArgs() < 1 || mprop->getArg(0)->getType() != Argument::MENTION_ARG)
			continue;
		if( (!is_mod(mprop->getPredType()) && !is_poss(mprop->getPredType())) ||
			mprop->getArg(0)->getMentionIndex() != m->getIndex() )
		{
			continue;
		}
			
		// avoid modifiers which share heads with the mention, *if* they have no
		// other children. I've never seen one w/ children, but just to be safe...
		if( mprop->getPredHead() != 0 &&
			mprop->getPredHead() == m->getHead() && mprop->getNArgs() == 1 )
		{
			continue;
		}
		
		// We sometimes see modifiers w/ prepositions as their heads,
		// with single children which have the same role as the head.
		// Eg: of<modifier>(<ref>:e4, of:e6), in<modifier>(<ref>:e6, in:e8)
		// We prefer to descend directly to the child, and
		// omit the modifier itself.
		if( PropForestFactory::COLLAPSE_REDUNDANT_MODIFIERS &&
			mprop->getPredHead() != 0 && mprop->getNArgs() == 2 &&
			(mprop->getArg(1)->getRoleSym() == mprop->getPredSymbol() ||
			 mprop->getArg(1)->getRoleSym() == Argument::POSS_ROLE))
		{
			if(      mprop->getArg(1)->getType() == Argument::MENTION_ARG )
			{
				const Mention * mprop_child_ment = mprop->getArg(1)->getMention(&this->mset);
				Symbol role = mprop->getArg(1)->getRoleSym();
				if (role != Argument::POSS_ROLE)
					role = extractCompoundRole(mprop_child_ment->getNode(), mprop->getPredSymbol());
				addResolutions(children, resolveMention(mprop_child_ment), &roles, role);
			}
			else if( mprop->getArg(1)->getType() == Argument::PROPOSITION_ARG )
			{
				const Proposition * mprop_child_prop = mprop->getArg(1)->getProposition();
				Symbol role = mprop->getArg(1)->getRoleSym();
				if (role != Argument::POSS_ROLE)
					role = extractCompoundRole(mprop_child_prop->getPredHead(), mprop->getPredSymbol());
				addResolutions(children, resolveProposition(mprop_child_prop), &roles, role);
			}
			
			continue;
		}
		
		addResolutions(children, resolveProposition(mprop), &roles, Symbol(L"<mod>"));
	}

	// find titles and add any arguments to the main name
	const Entity *ent = this->eset.getEntityByMention(m->getUID());
	for( int t_it = 0; t_it != this->pset.getNPropositions(); t_it++ ){
		const Proposition * tprop = this->pset.getProposition(t_it);

		if (!is_noun(tprop->getPredType()) || tprop->getNArgs() == 0 ||
			tprop->getArg(0)->getType() != Argument::MENTION_ARG)
			continue;

		// we only do this for persons
		const Mention *refMent = tprop->getArg(0)->getMention(&this->mset);
		if (!refMent->getEntityType().matchesPER())
			continue;

		// for a classic title, the title's parent will be the full mention
		// TODO: this won't work for "British leader, Gordon Brown, who..."
		if (refMent->getNode()->getParent() != m->getNode())
			continue;

		// the entities must be the same
		const Entity *refEnt = this->eset.getEntityByMention(refMent->getUID());
		if (refEnt != ent)
			continue;

		// the title must precede the head of the full mention
		if (refMent->getNode()->getEndToken() >= m->getHead()->getStartToken())
			continue;

		// we do not add in the title words themselves, since they might be misleading (e.g. 'president')
		for ( int a = 1; a < tprop->getNArgs(); a++) {
			// we only take mention arguments
			if( tprop->getArg(a)->getType() != Argument::MENTION_ARG )
				continue;

			const Mention * child_ment = tprop->getArg(a)->getMention(&this->mset);
			Symbol role = tprop->getArg(a)->getRoleSym();
			addResolutions(children, resolveMention(child_ment), &roles, role);
		}	

		// now tell the system not to expand the title proposition on its own, we're good with just this
		resolved_mention[refMent->getIndex()] = true;

	}
	
	PNodes def_resolutions;
	if( m->mentionType == Mention::LIST || m->mentionType == Mention::APPO ){
		// For these mention types, directly construct children from
		// child mentions. Any definitional propositions are ignored
		
		for(Mention * mc = m->getChild(); mc; mc = mc->getNext())
			addResolutions(children, resolveMention(mc), &roles, Symbol(L"<member>"));
		
		def_resolutions.push_back( buildPropNode(this->sent_no, PropNode::WeightedPredicates(), children, roles, 0, m, m->getNode()) );
		
	} else {
		// find & resolve any definitional propositions, passing
		// all resolved mention predicates & mention modifiers
		
		for(int d_it = 0; d_it != this->pset.getNPropositions(); d_it++){
			const Proposition * dprop = this->pset.getProposition(d_it);
			
			if( is_def(dprop->getPredType()) && dprop->getArg(0)->getMentionIndex() == m->getIndex() )
				addResolutions( def_resolutions, resolveProposition(dprop, roles, children, preds, m) );
		}
		if(def_resolutions.empty() && !preds.empty()){
			// no definitional propositions; fake it
			def_resolutions.push_back( buildPropNode(this->sent_no, preds, children, roles, 0, m, m->getNode()) );
		}
	}
	
	mention_resolutions[m->getIndex()].swap(def_resolutions);
	return mention_resolutions[m->getIndex()];
}


void PropForestFactory::addResolutions( PNodes & res,
									    const PNodes & new_res,
										Roles * roles /*= NULL*/,
										const Symbol & role /*= Symbol()*/ )
{
	for(PNodes::const_iterator rit = new_res.begin();
		rit != new_res.end(); ++rit)
	{
		res.push_back(*rit);
		if(roles) roles->push_back(role);
	}
	return;
}

// delegate constructor converts from PNodes => PropNodes, & allows tracking of construction history

inline PropNode_ptr PropForestFactory::buildPropNode(size_t sent_no,
														const PropNode::WeightedPredicates & preds,
														const PNodes & children,
														const Roles & roles,
														const Proposition * prop /*= NULL*/,
														const Mention * ment	 /*= NULL*/,
														const SynNode * synnode	 /*= NULL*/)
{
	if( !synnode && prop && prop->getPredHead() )
		synnode = prop->getPredHead();
	
	if( !synnode && ment && ment->getHead() )
		synnode = ment->getHead();
	
	PropNode_ptr ptr( _new PropNode(	&this->dt, sent_no, preds, 
										PropNodes(children.begin(), children.end()),
										std::vector<Symbol>(roles.begin(), roles.end()),
										prop, ment, synnode) );
	this->_forest_nodes.push_back(ptr);
	return ptr;
}


Symbol PropForestFactory::extractCompoundRole(const SynNode * node, Symbol role)
{
	if( !node) return role;
	
	const SynNode * crole = node;
	
	// Determine if node is subordinate to a PP node
	while(crole->getParent() && crole->getHeadWord() == node->getHeadWord())
		crole = crole->getParent();
	
	// If so, create a compound role
	if(crole->getTag() == Symbol(L"PP")){
		std::wstring prole;
		for(int crc_ind = 0; crc_ind != crole->getNChildren(); ++crc_ind){
			const SynNode * crchild = crole->getChild(crc_ind);
			
			if(crchild->getTag() != Symbol(L"IN") && crchild->getTag() != Symbol(L"TO"))
				continue;
			
			if(!prole.empty()) prole += L"_";
			prole += crole->getChild(crc_ind)->getHeadWord().to_string();
		}
		
		if(prole.size())
			role = Symbol(prole.c_str());
	}
	
	return role;
}
