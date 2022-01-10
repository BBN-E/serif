// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/PropTree/PropEdgeMatch.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <boost/foreach.hpp>

using namespace std;

#define ENUM_PREDS( container, it )										\
	for( PropNode::WeightedPredicates::const_iterator it = container.begin();		\
				it != container.end();  it++ )

// Because of boosting, we only want to add a parent/child predicate combination
// if the parent's predicate doesn't appear in the child's predicate set *at all*
#define TEST_ADD_EDGE															\
	if( !abort && !(ch_p_it->first == par_p_it->first) )							\
		_loc_edges.push_back( LocatedEdge(par_p_it->first, ch_p_it->first,			\
						role, par_p_it->second * ch_p_it->second, source_ind) );	\
	else																			\
		abort = true;

/*	if( !abort && !(ch_p_it->first == par_p_it->first) )						\
		_loc_edges.push_back( LocatedEdge(par_p_it->first, ch_p_it->first,		\
				_source_edges.size(), par_p_it->second * ch_p_it->second) );	\
	else																		\
		abort = true;*/


#define ADD_EDGE															\
	_loc_edges.push_back( LocatedEdge(par_p_it->first, ch_p_it->first,			\
						role, par_p_it->second * ch_p_it->second, source_ind) );
/*	_loc_edges.push_back( LocatedEdge(par_p_it->first, ch_p_it->first,		\
			_source_edges.size(), par_p_it->second * ch_p_it->second) )		\*/

#define ADD_ROOT																\
	_loc_roots.push_back(														\
				LocatedRoot(r_p_it->first, r_p_it->second, source_ind));

// Not a total-ordering; allows for quickly finding ranges of matching predicates

struct locatededge_sym_cmp {
	inline bool operator()(	const PropEdgeMatch::LocatedEdges::value_type & lhs,
							const PropEdgeMatch::LocatedEdges::value_type & rhs ) const
	{
		if( lhs.parent.pred() == rhs.parent.pred() ) return predicate_pred_cmp<Predicate>()( lhs.child, rhs.child );
		return predicate_pred_cmp<Predicate>()( lhs.parent, rhs.parent );
	}
	inline bool operator()(	const PropEdgeMatch::LocatedEdges::value_type & lhs,
		const std::pair<Predicate,Predicate> & rhs ) const
	{
		if( lhs.parent.pred() == rhs.first.pred() ) return predicate_pred_cmp<Predicate>()( lhs.child, rhs.second );
		return predicate_pred_cmp<Predicate>()( lhs.parent, rhs.first );
	}
	inline bool operator()(	const std::pair<Predicate,Predicate> & lhs,
							const PropEdgeMatch::LocatedEdges::value_type & rhs ) const
	{
		if( lhs.first.pred() == rhs.parent.pred() ) return predicate_pred_cmp<Predicate>()( lhs.second, rhs.child );
		return predicate_pred_cmp<Predicate>()( lhs.first, rhs.parent );
	}
};

// Not a total-ordering; allows for quickly finding ranges of matching roots 

struct locatedroot_sym_cmp {
	inline bool operator()(	const PropEdgeMatch::LocatedRoots::value_type & lhs,
							const PropEdgeMatch::LocatedRoots::value_type & rhs ) const
	{
		return predicate_pred_cmp<Predicate>()( lhs.pred, rhs.pred );
	}
	inline bool operator()(	const PropEdgeMatch::LocatedRoots::value_type & lhs,
		const Predicate & rhs ) const
	{
		return predicate_pred_cmp<Predicate>()( lhs.pred, rhs );
	}
	inline bool operator()(	const Predicate & lhs,
							const PropEdgeMatch::LocatedRoots::value_type & rhs ) const
	{
		return predicate_pred_cmp<Predicate>()( lhs, rhs.pred );
	}
};


PropEdgeMatch::PropEdgeMatch(const PropNodes & nodes,
							 const std::vector<float> & weights /*= std::vector<float>()*/)
 :
	PropMatch(nodes, weights)
{
	PropNodes roots;
	
	for(size_t p_it = 0; p_it != nodes.size(); p_it++){
		const PropNode & parent(*nodes[p_it]);
		
		for( size_t c_it = 0; c_it != parent.getNChildren(); c_it++ ){
			const PropNode & child(*parent.getChildren()[c_it]);
			
			size_t source_ind = 0;
			{
				// find the location of child in nodes
				// children must occur before parents, so
				// we stop prior to the parent's index
				PropNodes::const_iterator c_loc = lower_bound(
					nodes.begin(), nodes.begin() + p_it,
					child, PropNode::propnode_id_cmp());
				
				// if the child is not present in nodes, continue
				if( c_loc == nodes.end() || 
					(*c_loc)->getPropNodeID() != child.getPropNodeID())
				{
					continue;
				}
				// index of child in nodes	
				source_ind = distance(nodes.begin(), c_loc);
			}
			Symbol role = parent.getRoles()[c_it];

			// parent regular preds & stems ...
			ENUM_PREDS(parent.getPredicates(), par_p_it){
				size_t commit_size = _loc_edges.size(); bool abort = false;
				
				ENUM_PREDS(child.getPredicates(),         ch_p_it){ TEST_ADD_EDGE; }
				//ENUM_PREDS(child.getExtendedPredicates(), ch_p_it){ ADD_EDGE; }
				
				// if a parent predicate appears amoung the child's predicates,
				// don't add *any* loc-edges with that parent predicate
				if(abort) 
					_loc_edges.erase(_loc_edges.begin() + commit_size, _loc_edges.end());
			}
			
			// ... and parent synonyms
			//ENUM_PREDS(parent.getExtendedPredicates(), par_p_it){
			//	size_t commit_size = _loc_edges.size(); bool abort = false;
			//	
			//	ENUM_PREDS(child.getExtendedPredicates(), ch_p_it){ TEST_ADD_EDGE; }
			//	ENUM_PREDS(child.getPredicates(),         ch_p_it){ ADD_EDGE; }
			//	
			//	// if a parent extended predicate appears amoung the child's extended
			//	// predicates, don't add *any* loc-edges with that parent predicate
			//	if(abort)
			//		_loc_edges.erase(_loc_edges.begin() + commit_size, _loc_edges.end());
			//}
			
			// children of nodes with no predicates act as disjoint roots
			if(parent.getPredicates().empty()) { // && parent.getExtendedPredicates().empty())
				roots.push_back(parent.getChildren()[c_it]);
			}
		}
		
		// nodes with no predicates contribute zero weight
		if(parent.getPredicates().empty()) { // && parent.getExtendedPredicates().empty())
			this->_weights[p_it] = 0;
		}
	}
	
	// Step 2: add entries for sequence roots
	// RPB: After discussions with Liz, we decided to only add these if there are no located edges.
	if (_loc_edges.size() == 0) {
		// identify root nodes of disjoint subtrees in 'nodes'
		// enumRootNodes preserves nodes's sorted order
		PropNode::enumRootNodes(nodes, roots);
		
		for(size_t c_it = 0; c_it != roots.size(); c_it++){
			const PropNode & root(*roots[c_it]);
			
			size_t source_ind = distance(nodes.begin(),
				lower_bound(nodes.begin(), nodes.end(), root, PropNode::propnode_id_cmp()));
			
			ENUM_PREDS(root.getPredicates(),         r_p_it){ ADD_ROOT; }
			//ENUM_PREDS(root.getExtendedPredicates(), r_p_it){ ADD_ROOT; }
		}
	}
	//MRF: For Edge Match, some nodes are completely ignored, we dont want to include these as 0s when computing scores.  Mark them as invalid
	std::set<size_t> positions;
	for(size_t i = 0; i < _loc_edges.size(); i++){
		positions.insert(_loc_edges[i].source_ind);
	}
	//mrf: currently, there will never be roots if there are edge, but check both together just to be sure
	for(size_t i = 0; i < _loc_roots.size(); i++){
		positions.insert(_loc_roots[i].source_ind);
	}
	for(size_t i = 0; i < _covered.size(); i++){
		if(positions.find(i) == positions.end()){
			_valid[i] = false;
		}
	}

	// Sort located edges & roots for fast lookup
	sort(_loc_edges.begin(), _loc_edges.end(), locatededge_sym_cmp());
	sort(_loc_roots.begin(), _loc_roots.end(), locatedroot_sym_cmp());
	return;
}

PropEdgeMatch::~PropEdgeMatch(){
	_loc_edges.clear();
	_loc_roots.clear();
}


// Must be a full enumeration (eg, all children are explicitly present)

void PropEdgeMatch::computeCoverage(const PropNodes & nodes){
	
	for(size_t p_it = 0; p_it != nodes.size(); p_it++){
		const PropNode & parent(*nodes[p_it]);
		
		// Step 1: Look for matching edges...
		for(size_t c_it = 0; c_it != parent.getNChildren(); c_it++){
			const PropNode & child(*parent.getChildren()[c_it]);
			Symbol role = parent.getRoles()[c_it];
			
			ENUM_PREDS(parent.getPredicates(), par_p_it){
				ENUM_PREDS(child.getPredicates(), ch_p_it){
					
					// find all LocatedEdges with this parent/child predicate combination
					std::pair<LocatedEdges::iterator, LocatedEdges::iterator> match_range =
						equal_range( _loc_edges.begin(), _loc_edges.end(),
						std::make_pair(par_p_it->first, ch_p_it->first),
  						locatededge_sym_cmp());
					
					while(match_range.first != match_range.second){
						const LocatedEdge & loc_edge = *(match_range.first++);
						
						float prob = predicateTypeConfusionProb(loc_edge.parent, par_p_it->first) *
									 predicateTypeConfusionProb(loc_edge.child, ch_p_it->first) *
									 roleConfusionProb(loc_edge.role, role) *
									 par_p_it->second * ch_p_it->second * loc_edge.prior_prob;
						
						if(prob > this->_covered[loc_edge.source_ind] ){
							this->_covered[ loc_edge.source_ind] = prob;
							this->_covering[loc_edge.source_ind] = nodes[p_it];
						}
					}
					
					/* uncomment to enable symmetric matching * /
					// find all LocatedEdges with this child/parent predicate sym combination
					match_range = equal_range( _loc_edges.begin(), _loc_edges.end(),
											   make_pair(ch_p_it->first, par_p_it->first),
											   locatededge_sym_cmp());
					
					while(match_range.first != match_range.second){
						const LocatedEdge & loc_edge = *(match_range.first++);
						
						float prob = predicateTypeConfusionProb(loc_edge.parent, ch_p_it->first) *
									 predicateTypeConfusionProb(loc_edge.child, par_p_it->first) *
									 roleConfusionProb(loc_edge.role, role) *
									 par_p_it->second * ch_p_it->second * loc_edge.prior_prob;
						
						if(prob > _covered[loc_edge.source_ind] ){
							_covered[ loc_edge.source_ind] = prob;
							_covering[loc_edge.source_ind] = nodes[p_it];
						}
					}
					/ * */
				}
			}
		}
		
		// Step 2: Look for matching roots...
		ENUM_PREDS(parent.getPredicates(), root_p_it){
			
			std::pair<LocatedRoots::iterator, LocatedRoots::iterator> match_range =
				equal_range( _loc_roots.begin(), _loc_roots.end(),
							root_p_it->first, locatedroot_sym_cmp());
			
			while(match_range.first != match_range.second){
				const LocatedRoot & loc_root = *(match_range.first++);
				
				float prob = predicateTypeConfusionProb(root_p_it->first, loc_root.pred) *
							 root_p_it->second * loc_root.prior_prob;
				
				if(prob > this->_covered[loc_root.source_ind]){
					this->_covered[loc_root.source_ind] = prob;
					this->_covering[loc_root.source_ind] = nodes[p_it];
				}
			}
		}
		
		// Next target node...
	}
	
	return;
}

/*
	Use the SessionLogger as a way of printing the status with respect to _covered, _covering, _groups, _weights, and _valid 
	_covered and _covering are filled during computeCoverage(), so this makes most sense called after computeCoverage()
*/
void PropEdgeMatch::debugPrintValidityAndSourceEdges() const {
	std::set<size_t> positions;
	std::wstringstream debugstream;
	debugstream<<"Calculate Validity \n";
	std::map<size_t, std::set<Symbol> > parentMapEdge;
	std::map<size_t, std::set<Symbol> > childMapEdge;
	for(size_t i = 0; i < _loc_edges.size(); i++){
		parentMapEdge[_loc_edges[i].source_ind].insert(_loc_edges[i].parent.pred());
		childMapEdge[_loc_edges[i].source_ind].insert(_loc_edges[i].child.pred());
		positions.insert(_loc_edges[i].source_ind);
	}
	std::map<size_t, std::set<Symbol> > parentMapRoot;
	//mrf: currently, there will never be roots if there are edge, but check both together just to be sure
	for(size_t i = 0; i < _loc_roots.size(); i++){
		parentMapRoot[_loc_roots[i].source_ind].insert(_loc_roots[i].pred.pred());				
		positions.insert(_loc_roots[i].source_ind);
	}
	for(size_t i = 0; i < _valid.size(); i++){
		debugstream<<i<<": ";
		if(_valid[i]){
			debugstream<<"\tValid: ";
			debugstream<<"\tParents: ";
			BOOST_FOREACH(Symbol p, parentMapEdge[i]){
				debugstream<<p<<", ";
			}
			debugstream<<"\tChilderen: ";
			BOOST_FOREACH(Symbol p, childMapEdge[i]){
				debugstream<<p<<", ";
			}
			debugstream<<"\tRoots: ";
			BOOST_FOREACH(Symbol p, parentMapRoot[i]){
				debugstream<<p<<", ";
			}
		}
		else{
			debugstream<<"\tInvalid";
		}
		debugstream<<"\n";
	}
	SessionLogger::warn("prop_trees")<<debugstream.str().c_str()<<std::endl;
	
}
/*
	Use the SessionLogger as a way of printing the status with respect to _covered, _covering, _groups, _weights, and _valid 
	_covered and _covering are filled during computeCoverage(), so this makes most sense called after computeCoverage()
*/
void PropEdgeMatch::debugPrintValidityAndCoverage() const {
	std::wstringstream tempstream;
	tempstream<<"Covering Results\n";
	for(size_t i = 0; i < _covered.size(); i++){
		tempstream<<i<<": ";
		if(_valid[i]){
			if(_covered[i]){
				const Predicate* p = (*_covering[i]).getRepresentativePredicate();
				tempstream<<"VALID: "<<p->pred()<<" \t"<<_covered[i]<<" \t"<<_valid[i]<<" \t"<<_weights[i]<<" \t";
			}
			else{
				tempstream<<"VALID: EMPTY"<<_covered[i]<<" \t"<<_valid[i]<<" \t"<<_weights[i]<<" \t";
			}
		}
		else{
			tempstream<<"INVALID: "<<" \t"<<_valid[i]<<" \t"<<_weights[i]<<" \t";
		}
		tempstream<<"\n";
	}
	SessionLogger::warn("prop_trees")<<tempstream.str().c_str()<<std::endl;

}

void PropEdgeMatch::print(std::wostream& stream) const {
	if (this->_covered.size() == 0)
		SessionLogger::warn("prop_trees") << "_covered has size 0 in edge match\n";

	// Roots
	std::map<wstring,int> pred_counts;
	for (LocatedRoots::const_iterator it = _loc_roots.begin(); it != _loc_roots.end(); it++) {
		std::wstring pred_string = it->pred.pred().to_string();
		if (pred_counts.find(pred_string) == pred_counts.end()) {
			pred_counts[pred_string] = 1;
		} else {
			pred_counts[pred_string] = pred_counts[pred_string] + 1;
		}
	}
	for (std::map<wstring,int>::const_iterator it = pred_counts.begin(); it != pred_counts.end(); it++) {
		stream << it->first << L": " << it->second << L"\n";	
	}


	// Edges
	std::map<wstring,int> edge_counts;
	for (LocatedEdges::const_iterator it = _loc_edges.begin(); it != _loc_edges.end(); it++) {
		std::wstringstream edge_stream;
		edge_stream << it->parent.pred().to_string() << L" <" << it->role.to_string() << L"> " <<  it->child.pred().to_string();
		std::wstring edge_string = edge_stream.str();
		if (edge_counts.find(edge_string) == edge_counts.end()) {
			edge_counts[edge_string] = 1;
		} else {
			edge_counts[edge_string] = edge_counts[edge_string] + 1;
		}
	}
	for (std::map<wstring,int>::const_iterator it = edge_counts.begin(); it != edge_counts.end(); it++) {
		stream << it->first << L": " << it->second << L"\n";	
	}
}

