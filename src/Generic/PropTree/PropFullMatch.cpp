// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/PropTree/PropFullMatch.h"
#include "common/UnexpectedInputException.h"
#include "Generic/linuxPort/serif_port.h" // Needed for std::min and std::max
#include "Generic/PropTree/Predicate.h"
#include <algorithm>


PropFullMatch::PropFullMatch(const PropNodes & nodes,
							 bool compare_roots_only,
							 const std::vector<float> & weights /* = vector<float>() */)
 :
	PropMatch(nodes, weights),
	_nodes(nodes.size()),
	_compare_roots_only(compare_roots_only)
{
	PropNodes roots, dir_nodes;
	// extract roots which best-cover some token in the span
	PropNode::enumSpanRootNodes(nodes, roots);
	// extract nodes which are direct descendants of roots
	PropNode::enumAllNodes(roots, dir_nodes);
	
	// nodes & dir_nodes are sorted, we do a merge to determine
	//  if a node is a root or descendant of one
	PropNodes::const_iterator root_it = roots.begin(), dir_it = dir_nodes.begin();

	for(size_t n_ind = 0; n_ind != nodes.size(); ++n_ind)
	{
		const PropNode & node = *nodes[n_ind];
		node_state & ns = _nodes[n_ind];
		ns.id=node.getPropNodeID(); 
		ns.exact_role_match=node.matchRoleExactly();

		// determine if this node is a descendant of a root
		while(dir_it != dir_nodes.end() && PropNode::propnode_id_cmp()(*dir_it, node)) ++dir_it;
		if(   dir_it == dir_nodes.end() || PropNode::propnode_id_cmp()(node, *dir_it))
		{
			// this node is unreachable; ignore it
			this->_weights[n_ind] = 0.0f;
			continue;
		}
		
		// determine if this node is a root, marking the index if so
		while(root_it != roots.end() &&  PropNode::propnode_id_cmp()(*root_it, node)) ++root_it;
		if(   root_it != roots.end() && !PropNode::propnode_id_cmp()(node, *root_it))
		{
			_roots.push_back(n_ind);
		}
		
		// extract predicates from node, tracking ranges
		ns.wpreds_begin = _wpreds.size();
		
		_wpreds.insert( _wpreds.end(),
						node.getPredicates().begin(),
						node.getPredicates().end());
		
		ns.wpreds_end = _wpreds.size();
		
		// Sort the elements in the range we just added
		sort(	_wpreds.begin() + ns.wpreds_begin,
				_wpreds.begin() + ns.wpreds_end,
				predicate_cmp<Predicate>());
		
		// if this node has no predicates, it contributes no weight
		if( ns.wpreds_begin == ns.wpreds_end )
			this->_weights[n_ind] = 0.0f;
		
		// extract children from node, tracking ranges
		ns.rchildren_begin = _rchildren.size();
		for(size_t c_ind = 0; c_ind != node.getNChildren(); ++c_ind)
		{
			const PropNode & c_node = *node.getChildren()[c_ind];
			
			// find c_node's location in nodes
			PropNodes::const_iterator c_it = lower_bound(
				nodes.begin(), nodes.end(), c_node, PropNode::propnode_id_cmp() );
			
			if(c_it == nodes.end() || (*c_it)->getPropNodeID() != c_node.getPropNodeID())
				continue;
			
			int dist = static_cast<int>(distance(nodes.begin(), c_it));
			_rchildren.push_back(std::make_pair(node.getRoles()[c_ind], dist));
		}
		ns.rchildren_end = _rchildren.size();
	}
}

float PropFullMatch::compareToTarget(const PropNodes& target_nodes, bool multiplicative) {
	if( !this->_covered.size())
		return 0.0f;

	// Get the nodes we will compare against.  These could either be just the roots or all the nodes.
	PropNodes trg_compare_nodes; 
	
	if (_compare_roots_only) { // if we are only comparing roots against roots
		PropNode::enumSpanRootNodes(target_nodes, trg_compare_nodes);

		// Occasionally, parent & child have identical extents;
		//   we want to allow either to match
		for( size_t i = 0; i != trg_compare_nodes.size(); i++ ) {
			for( PropNodes::const_iterator c_it = trg_compare_nodes[i]->getChildren().begin();
				c_it != trg_compare_nodes[i]->getChildren().end(); ++c_it ) {
				if( (*c_it)->getStartToken() == trg_compare_nodes[i]->getStartToken() &&
					(*c_it)->getEndToken()   == trg_compare_nodes[i]->getEndToken() ) {
					trg_compare_nodes.push_back(*c_it);
				}
			}
		}
	} else { // if we are attempting to instantiate the pattern at all nodes
		PropNode::enumAllNodes(target_nodes, trg_compare_nodes);
	}

	// Loop over the nodes we want to compare against
	float best_score = 0.0f;
	for(PropNodes::const_iterator t_it = trg_compare_nodes.begin(); t_it != trg_compare_nodes.end(); t_it++) {

		// Clear statistics so we don't get credit for another target node's match.
		fill(this->_covered.begin(),  this->_covered.end(),  0.0f);
		fill(this->_covering.begin(), this->_covering.end(), PropNode_ptr());

		// Loop over our roots
		for(size_t r_ind = 0; r_ind != _roots.size(); ++r_ind) {
			internalCompare(_roots[r_ind], *t_it, Symbol(), Symbol());
		}

		// Get our score for matching against this target node.  Update our best score if it was exceeded.
		float score = PropMatch::computeScoreFromCoverage(multiplicative);
		if (score > best_score) { best_score = score; }			
	}
	return best_score;	
}

PropFullMatch::PatternPredicateBounds
PropFullMatch::getWeightedPredicateRange(size_t idx) const {
	return make_pair(_wpreds.begin()+_nodes[idx].wpreds_begin,
		_wpreds.begin()+_nodes[idx].wpreds_end);
}

float PropFullMatch::computeLocalMatchProb(const PropNode_ptr & trg, const Symbol & s_role, 
										  const Symbol & t_role, size_t n_ind) const
{
	const node_state & ns = _nodes[n_ind];
	float match_prob=0.0;

	if (ns.exact_role_match && s_role!=t_role) {
		return 0.0f;
	}

	for(PropNode::WeightedPredicates::const_iterator pr_it = trg->getPredicates().begin();
		pr_it != trg->getPredicates().end(); ++pr_it)
	{
		match_prob=(std::max)(match_prob, matchPredicate(ns.id, _wpreds.begin()+ns.wpreds_begin,
			_wpreds.begin()+ns.wpreds_end, *pr_it, s_role, t_role));
	}
	return match_prob;
}

// s_role, t_role, pr_it
float PropFullMatch::matchPredicate(size_t pat_node_id, 
					const wpreds_t::const_iterator& startIt, 
					 const wpreds_t::const_iterator& endIt, 
					 const PropNode::WeightedPredicate& docPred,
					 const Symbol& s_role, const Symbol& t_role) const {
	float best_val=0.0f;

	std::pair<wpreds_t::const_iterator, wpreds_t::const_iterator> rng = equal_range(
		startIt, endIt,
		docPred.first, predicate_pred_cmp<Predicate>());

	while(rng.first != rng.second)
	{
		// independently combine predicate type confusion, node
		// role confusion, and predicate prior probabilities
		best_val = (std::max)(best_val,
			predicateTypeConfusionProb(rng.first->first, docPred.first) *
			PropMatch::roleConfusionProb(s_role, t_role) *
			rng.first->second * docPred.second);

		rng.first++;
	}

	return best_val;
}

void PropFullMatch::internalCompare(size_t n_ind,  
									const PropNode_ptr & trg,
	const Symbol & s_role, const Symbol & t_role)
{
	
	// determine node-to-node match
	float match_prob = computeLocalMatchProb(trg, s_role, t_role, n_ind);
	
	if(match_prob > this->_covered[n_ind])
	{
		this->_covered[ n_ind] = match_prob;
		this->_covering[n_ind] = trg;
	}
	
	const node_state & ns = _nodes[n_ind];

	// don't recurse on a miss, unless the source-node has no predicates
	if(!match_prob && (ns.wpreds_begin != ns.wpreds_end)) return;

	std::pair<rchildren_t::const_iterator, rchildren_t::const_iterator> bounds=getChildRange(n_ind);
	
	for(rchildren_t::const_iterator sc_it = bounds.first;
		sc_it != bounds.second; ++sc_it)
	{
		for(size_t tc_ind = 0; tc_ind != trg->getNChildren(); ++tc_ind)
		{
			internalCompare(sc_it->second,
							trg->getChildren()[tc_ind],
							sc_it->first,
							trg->getRoles()[tc_ind]);
		}
	}
}


void PropFullMatch::print(std::wostream& stream) const {
	int num_groups = 0;
	if (this->_covered.size() == 0)
		SessionLogger::warn("prop_trees") << "_covered has size 0 in full match\n";

	
	// Loop over our roots
	for(size_t r_ind = 0; r_ind != _roots.size(); ++r_ind) {
		printNode(stream, _roots[r_ind], 0, L"");
	}
}

void PropFullMatch::printNode(std::wostream& stream, size_t node_state_index, int indent, const std::wstring& role) const {
	const node_state& ns = _nodes[node_state_index];
	
	// Print our indent, role, and opening paren
	for (int i = 0; i < indent; i++) {
		stream << L" ";
	}
	stream << role << L" (";

	// Print our predicates
	wpreds_t::const_iterator startIt = _wpreds.begin() + ns.wpreds_begin;
	wpreds_t::const_iterator endIt = _wpreds.begin() + ns.wpreds_end;
	for (wpreds_t::const_iterator& it = startIt; it != endIt; it++) {
		stream << it->first.pred().to_string() << ", ";
	}
	stream << L"\n";

	// Print our children and their roles
	std::pair<rchildren_t::const_iterator, rchildren_t::const_iterator> bounds = getChildRange(node_state_index);
	for(rchildren_t::const_iterator sc_it = bounds.first; sc_it != bounds.second; ++sc_it) {
		printNode(stream, sc_it->second, indent+4, sc_it->first.to_string());
	}

	// Print our indent and closing paren
	for (int i = 0; i < indent; i++) {
		stream << L" ";
	}
	stream << L")\n";
}
