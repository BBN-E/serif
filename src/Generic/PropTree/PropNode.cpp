// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/PropTree/PropNode.h"

#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>

#include "Generic/common/Symbol.h"
#include "common/UnrecoverableException.h"
#include "common/UTF8OutputStream.h"
#include "Generic/linuxPort/serif_port.h" // Needed for std::min and std::max
#include "theories/Mention.h"
#include "theories/Proposition.h"
#include "theories/SynNode.h"
#include "wordnet/xx_WordNet.h"


// output of debug representations
class UTF8OutputStream;
std::wostream & operator << (std::wostream & s, const PropNode & p);

inline std::wostream & operator << (std::wostream & s, const PropNode::ptr_t & p){ return (s << *p); }

UTF8OutputStream & operator << (UTF8OutputStream & s, const PropNode & p);

inline UTF8OutputStream & operator << (UTF8OutputStream & s, const PropNode::ptr_t & p){ return (s << *p); }

/// static initialization
size_t PropNode::__unused_PropNodeID = 0;

// Once created, PropNode structural relationships are immutable. Note:
// This asserts that children exist prior to parent construction
PropNode::PropNode( const DocTheory * dtheory, size_t sent_no,
				   const PropNode::WeightedPredicates & preds,
				   const PropNode::PropNodes & children,
				   const std::vector<Symbol> & roles,
				   const Proposition * prop /*= NULL*/, const Mention * ment /*= NULL*/, const SynNode * synnode /*= NULL*/,
				   bool exact_match) :
_propNodeID(__unused_PropNodeID++),
_dtheory(dtheory), _sent_no(sent_no),
_truepreds(preds.begin(), preds.end()),
_ext_preds(preds.begin(), preds.end()),
_children(children.begin(), children.end()),
_roles(roles.begin(), roles.end()),
_prop(prop), _ment(ment), _synnode(synnode),
_start_tok((size_t) -1), _end_tok(0),
_head_start_tok((size_t) -1), _head_end_tok(0),
_ref_cnt(0), _parents(), _exact_role_match(exact_match)
{	
	init(prop, ment, synnode);
}

// this is for initializing 
PropNode::PropNode( const DocTheory * dtheory, size_t sent_no,
				   const PropNode::WeightedPredicates & preds,
				   const PropNode::PropNodes & children,
				   const std::vector<Symbol> & roles,
				   int start_tok_idx, int end_tok_idx, bool exact_match) :
_propNodeID(__unused_PropNodeID++),
_dtheory(dtheory), _sent_no(sent_no),
_truepreds(preds.begin(), preds.end()),
_ext_preds(preds.begin(), preds.end()),
_children(children.begin(), children.end()),
_roles(roles.begin(), roles.end()),
_prop(0), _ment(0), _synnode(0),
_start_tok(start_tok_idx), _end_tok(end_tok_idx),
_head_start_tok(start_tok_idx), _head_end_tok(end_tok_idx),
_ref_cnt(0), _parents(), _exact_role_match(exact_match)
{	
	expandForChildren();
}

void PropNode::init(const Proposition* prop, const Mention* ment, const SynNode* synnode)
{
	if( !ment && !synnode && !prop && _children.empty() )
		throw UnrecoverableException("PropNode::init", "PropNode constructed without children, Mention, Proposition, or SynNode");
	if( _children.size() != _roles.size() )
		throw UnrecoverableException("PropNode::init", "Mismatch between # of children and # of associated roles");

	// Find token offsets from associated Mention/Proposition/Synnode
	if( prop && prop->getPredHead() ){
		_start_tok = (size_t) prop->getPredHead()->getStartToken();
		_end_tok   = (size_t) prop->getPredHead()->getEndToken();
		_head_start_tok = _start_tok;
		_head_end_tok   = _end_tok;
	}
	else if( ment && ment->getHead() ){
		_start_tok = (size_t) ment->getHead()->getStartToken();
		_end_tok   = (size_t) ment->getHead()->getEndToken();
		_head_start_tok = _start_tok;
		_head_end_tok   = _end_tok;
	}
	else if( synnode ){
		_start_tok = (size_t) synnode->getStartToken();
		_end_tok   = (size_t) synnode->getEndToken();
		_head_start_tok = (size_t) synnode->getHeadPreterm()->getStartToken();
		_head_end_tok   = (size_t) synnode->getHeadPreterm()->getEndToken();
	}

	// if there's no current head (commonly happens with
	//   Mention::LIST), use extents of children
	if( _head_end_tok < _head_start_tok )
	{
		if( _children.empty() )
			throw UnrecoverableException("PropNode::init", "Non-headed proposition without PropNode children");

		_head_start_tok = _children[0]->getHeadStartToken();
		_head_end_tok   = _children[0]->getHeadEndToken();

		for( size_t i = 1; i != _children.size(); i++ ){
			_head_start_tok = (std::min)( _head_start_tok, _children[i]->getHeadStartToken());
			_head_end_tok = (std::max)( _head_end_tok, _children[i]->getHeadEndToken());
		}
	}

	expandForChildren();
}

void PropNode::expandForChildren()
{
	// Expand token offsets to include children, & mark children with roles played
	for( size_t i = 0; i != _children.size(); i++ ){
		PropNode & child( *_children[i] );

		child.addParent(this);

		_start_tok = (std::min)( _start_tok, child.getStartToken() );
		_end_tok   = (std::max)( _end_tok,   child.getEndToken()   );

		if( find(child._rolesPlayed.begin(), child._rolesPlayed.end(), _roles[i]) == child._rolesPlayed.end() )
			child._rolesPlayed.push_back( _roles[i] );
	}
}

size_t PropNode::getTreeDepth() const {
	size_t cDepth = 0;
	for( PropNodes::const_iterator c_it = _children.begin(); c_it != _children.end(); c_it++ )
		cDepth = std::max( cDepth, (*c_it)->getTreeDepth() );
	return cDepth + 1;
}

size_t PropNode::getTreeSize() const {
	size_t cSize = 0;
	for (PropNodes::const_iterator c_it = _children.begin(); c_it != _children.end(); c_it++ )
		cSize += (*c_it)->getTreeSize();
	return cSize + 1;
}

void PropNode::addParent(PropNode* par) {
	if (find(_parents.begin(), _parents.end(), par) == _parents.end()) {
		_parents.push_back(par);
	}
}

// Enumerates all nodes & their children.
//  - Input is unordered 
//  - Output contains no duplicates and is sorted on PropNode::ID
void PropNode::enumAllNodes(const PropNode::PropNodes & nodes, 
									  PropNode::PropNodes & all_nodes){

	all_nodes.clear();
	all_nodes.reserve(nodes.size() * 3);
	all_nodes.insert(all_nodes.end(), nodes.begin(), nodes.end());

	for(size_t i = 0; i != all_nodes.size(); i++)
	{
		all_nodes.insert(all_nodes.end(), all_nodes[i]->getChildren().begin(), all_nodes[i]->getChildren().end());
	}

	// sort and remove duplicates
	sort(all_nodes.begin(), all_nodes.end(), PropNode::propnode_id_cmp());
	all_nodes.erase(
		unique(all_nodes.begin(), all_nodes.end()),
		all_nodes.end()
		);

	return;
}

// Enumerates the rootmost nodes of all disjoint subtrees in the set
//  - Input must be sorted on PropNode::ID
//  - Output is a subset of input, sorted on PropNode::ID
void PropNode::enumRootNodes(const PropNode::PropNodes & nodes, 
									   PropNode::PropNodes & all_roots){

	PropNodes cnodes; cnodes.reserve(nodes.size() * 3);

	// collect all children of nodes in 'nodes'
	for(PropNodes::const_iterator it = nodes.begin(); it != nodes.end(); it++)
		cnodes.insert(cnodes.end(), (*it)->getChildren().begin(), (*it)->getChildren().end());
	sort(cnodes.begin(), cnodes.end(), propnode_id_cmp());

	// merge nodes & cnodes, identifying nodes not appearing as children
	PropNodes::const_iterator n_it = nodes.begin(), c_it = cnodes.begin();

	for( ; n_it != nodes.end(); ++n_it )
	{
		// increment c_it till >= than n_it
		while( c_it != cnodes.end() && propnode_id_cmp()(*c_it, *n_it)) ++c_it;
		// if n_it < c_it, this is a root
		if(    c_it == cnodes.end() || propnode_id_cmp()(*n_it, *c_it))
		{
			all_roots.push_back(*n_it);
		}
	}

	return;
}

/**
*  Identifies those nodes in the nodes input that have a null parent.
*  Unlike PropNode::enumRootNodes(), this function does not store any
*  node in all_roots with a non-null parent. At some point, someone should
*  look more closely at PropNode::enumRootNodes() to see if behavior is
*  a bug or a feature.
*/
void PropNode::enumNullParentNodes(const PropNode::PropNodes& nodes, PropNode::PropNodes& all_roots) {
	for (PropNodes::const_iterator nodes_it = nodes.begin(); nodes_it != nodes.end(); ++nodes_it) {
		if (!(*nodes_it)->hasParent()) {
			all_roots.push_back(*nodes_it);
		}
	}
}

// Given a PropNode_ptr, gets the indices of tokens covered by the heads of this node and its children
void PropNode::getCoveredHeadTokens( PropNode::PropNode_ptr node, std::set<size_t>& headTokens ) {
	for( size_t t = node->getHeadStartToken(); t <= node->getHeadEndToken(); t++ ){
		headTokens.insert(t);
	}
	BOOST_FOREACH(PropNode::PropNode_ptr child, node->getChildren()) {
		getCoveredHeadTokens(child, headTokens);
	}
	return;
}

// Enumerates the rootmost nodes of all disjoint subtrees which are
//  also the best-covering root for some token in the covered span
//  - Input must be sorted on PropNode::ID
//  - Output is a subset of input, sorted on PropNode::ID
void PropNode::enumSpanRootNodes(const PropNode::PropNodes & nodes, 
										   PropNode::PropNodes & span_roots){

	if(nodes.empty()) return;
 
	// enum disjoint roots, and sort by length dec
	enumRootNodes(nodes, span_roots);
	sort(span_roots.begin(), span_roots.end(), propnode_len_cmp());

	//	if( span_roots[0]->getSentNumber() == 0 || span_roots[0]->getSentNumber() == 17 ){
	//		wcout << "Roots:" << endl;
	//		for( size_t i = 0; i != span_roots.size(); i++ )
	//			wcout << *span_roots[i] << endl;
	//	}


	std::vector<std::set<size_t> > headTokensCovered;

	for (size_t i = 0; i < span_roots.size(); i++) {
		// Identify head tokens covered by this span
		std::set<size_t> headTokens;
		PropNode::PropNode_ptr root = span_roots.at(i);
		getCoveredHeadTokens(root, headTokens);
		headTokensCovered.push_back(headTokens);
	}

	std::vector<size_t> nodes_to_erase;
	for (size_t i = 1; i < span_roots.size(); i++) {

		std::set<size_t> head_tokens = headTokensCovered.at(i); // this should be a copy
		for (size_t j = 0; j < i; j++) {
			BOOST_FOREACH(size_t head, headTokensCovered.at(j)) {
				head_tokens.erase(head);
			}
		}

		if (head_tokens.size() == 0)
			nodes_to_erase.push_back(i);
	}

	BOOST_FOREACH(size_t i, nodes_to_erase) {
		span_roots.erase(span_roots.begin() + i);
	}

	// sort on PropNode::ID
	sort(span_roots.begin(), span_roots.end(), propnode_id_cmp());

	//	if( span_roots[0]->getSentNumber() == 0 || span_roots[0]->getSentNumber() == 17 ){
	//		wcout << "Span Roots:" << endl;
	//		for( size_t i = 0; i != span_roots.size(); i++ )
	//			wcout << *span_roots[i] << endl;
	//	}

	return;
}

const Predicate* PropNode::getRepresentativePredicate() const {
	if (_truepreds.size()) {
		WeightedPredicates::const_iterator biggest=_truepreds.begin();

		for (WeightedPredicates::const_iterator cur=biggest; cur!=_truepreds.end(); ++cur) {
			if (cur->second > biggest->second) {
				biggest=cur;
			}
		}

		if (biggest!=_truepreds.end()) {
			return &(biggest->first);
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

void PropNode::dumpNodeContent(std::wostream &s, bool print_types, bool expand_synonyms) const
{
	const Predicate* repPred=getRepresentativePredicate();

	if (repPred) {
		s << repPred->pred();
	} else {
		s << L"???";
	}

	if (print_types) {
		s << L"-";
		if (repPred) {
			s << repPred->type();
		} else {
			s << L"???";
		}
	}

	if (expand_synonyms) {
		s<<" {";
		printSynonyms(s);
		s<<"}";
	}
}

void PropNode::compactPrint(std::wostream& s, bool print_types, bool expand_synonyms, bool replace_unknown, int indent) const {
	//for (int i=0; i<indent; ++i) s << L'\t';

	s << L"(";
	dumpNodeContent(s, print_types, expand_synonyms);
	
	for( size_t c = 0; c != getNChildren(); c++ ){
		s << std::endl;
		for (int i=0; i<indent; ++i) s << L'\t';
		std::wstring roleStr = getRoles()[c].to_string();
		if(replace_unknown && roleStr == L"<unknown>"){
			s << L" <mod>:";
		}
		else{
			s << L" " << roleStr << L":";
		}
		getChildren()[c]->compactPrint(s, print_types, expand_synonyms, replace_unknown, indent+1);
	}

	s << L")";
}

void PropNode::dumpEdges(std::wostream& s, bool print_types, bool expand_synonyms) const {
	for (size_t i=0; i<_children.size(); i++) {
		const PropNode& kid=*(_children[i]);
		const Symbol& kidRole=_roles[i];

		dumpNodeContent(s, print_types, expand_synonyms);
		s << L"\t<" << kidRole << L">\t";
		kid.dumpNodeContent(s, print_types, expand_synonyms);
		s << std::endl;

		kid.dumpEdges(s, print_types, expand_synonyms);
	}
}

void PropNode::dumpNodes(std::wostream& s, bool print_types, bool expand_synonyms) const {
	dumpNodeContent(s, print_types, expand_synonyms);
	s << std::endl;
	for (PropNodes::const_iterator it=_children.begin(); it!=_children.end(); ++it) {
		(*it)->dumpNodes(s, print_types, expand_synonyms);
	}
}

// I altered these to just output one element of the weighted predicates set
// for readability, since Wordnet, etc. can easily result in huge sets. ~ RMG
void PropNode::printSynonyms(std::wostream& s) const {
	bool first=true;


	for (WeightedPredicates::const_iterator it=getPredicates().begin(); it!=getPredicates().end(); ++it) {
		if (!first) {
			s << L", ";
		}
		first=false;
		s << it->first.pred() << L"=" << it->second;
	}
}

void PropNode::printPredicates(std::wostream& stream, const PropNodes& nodes) {
	for (unsigned int i = 0; i < nodes.size(); i++) {
		WeightedPredicates w_preds = nodes[i]->getPredicates();
		for (WeightedPredicates::const_iterator pr_it = w_preds.begin(); pr_it != w_preds.end(); pr_it++) {
			Predicate p = pr_it->first;
			stream << p.pred().to_string() << std::endl;
		}
	}
}

std::wostream & operator << (std::wostream & s, const PropNode & p) {

	static int indent = 0;

	// output predicates
	s << L"pnode( [";
/*	for( WeightedPredicates::const_iterator it = p.getPredicates().begin(); it != p.getPredicates().end(); it++ )
		s << it->first << L",";*/
	const Predicate* repPred=p.getRepresentativePredicate();
	if (repPred) {
		s << *repPred;
	} else {
		s << "???";
	}
	s << L"],";

	/* // output predicate synonyms
	s << L"[";
	for( WeightedPredicates::const_iterator it = p.getExtendedPredicates().begin(); it != p.getExtendedPredicates().end(); it++ )
	s << it->first << L",";
	s << L"],";
	*/

	indent++;
	if( ! p.getChildren().empty() ) s.put(L'\n');
	for( size_t c = 0; c != p.getNChildren(); c++ ){
		for( int in = 0; in != indent; in++ ) s.put(L'\t');
		s << L"( '" << p.getRoles()[c].to_string() << L"', " << *(p.getChildren()[c]) << L" ),\n";
	}
	indent--;

	for( int in = 0; in != indent; in++ ) s.put(L'\t');
	s << L")";
	// output children

	return s;
}

UTF8OutputStream & operator << (UTF8OutputStream & s, const PropNode & p) {

	static int indent = 0;

	// output predicates
	s << L"pnode( [";
/*	for( WeightedPredicates::const_iterator it = p.getPredicates().begin(); it != p.getPredicates().end(); it++ )
		s << it->first << L",";*/
	const Predicate* repPred=p.getRepresentativePredicate();
	if (repPred) {
		s << *repPred;
	} else {
		s << "???";
	}
	s << L"],";

	indent++;
	if( ! p.getChildren().empty() ) s.put(L'\n');
	for( size_t c = 0; c != p.getNChildren(); c++ ){
		for( int in = 0; in != indent; in++ ) s.put(L'\t');
		s << L"( '" << p.getRoles()[c].to_string() << L"', " << *(p.getChildren()[c]) << L" ),\n";
	}
	indent--;

	for( int in = 0; in != indent; in++ ) s.put(L'\t');
	s << L")";
	// output children

	return s;
}

const PropNode::ptr_t PropNode::getRoot() const 
{
	const PropNode* cur=this; 
	while (cur->hasParent())
		cur = _parents[0];
	return ptr_t(const_cast<PropNode*>(cur));
}


const Symbol& PropNode::getRoleForChild(const PropNode_ptr& child) const {
    std::vector<ChildRole> kids = getChildRoles();
	BOOST_FOREACH(ChildRole kid, kids) {
		if (kid.get<0>()==child) {
			return kid.get<1>();		
		}
	}

	throw UnrecoverableException("PropNode::getRoleForChild", 
		"Supplied node not a child of this one.");
}

const PropNode::ptr_t PropNode::getFirstChildOfRole(const Symbol& role) const {
    std::vector<ChildRole> kids = getChildRoles();
	BOOST_FOREACH(ChildRole kid, kids) {
		if (kid.get<1>()==role) {
			return kid.get<0>();
		}
	}

	return PropNode::ptr_t(0);
}

const std::vector<PropNode::ChildRole> PropNode::getChildRoles() const {
    size_t n_children = getNChildren();
    std::vector<ChildRole> ret;
    ret.reserve(n_children);
    PropNodes::const_iterator child = _children.begin();
    std::vector<Symbol>::const_iterator role = _roles.begin();
    for (; child != _children.end() && role != _roles.end(); ++child, ++role) {
        ChildRole cr = boost::make_tuple(*child, *role);
        ret.push_back(cr);
    }
    return ret;
}


