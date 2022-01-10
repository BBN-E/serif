// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef PROPNODE_H
#define PROPNODE_H

#include <vector>
#include <map>
#include <set>
#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/iterator/zip_iterator.hpp>

#include "Generic/PropTree/Predicate.h"

class DocTheory;
class Mention;
class Proposition;
class SynNode;

class PropNode {

public:
	typedef boost::intrusive_ptr<PropNode> PropNode_ptr;
	typedef std::vector<PropNode_ptr> PropNodes;
	typedef boost::shared_ptr<PropNodes> PropNodes_ptr;
	typedef boost::intrusive_ptr<PropNode> ptr_t; // for shorter typing...

	typedef std::pair<Predicate, float> WeightedPredicate;
	typedef std::map< Predicate, float > WeightedPredicates;
		
	typedef boost::zip_iterator<boost::tuple<
		PropNodes::const_iterator, 
		std::vector<Symbol>::const_iterator> > const_iterator;
	typedef boost::zip_iterator<boost::tuple<
		PropNodes::iterator, 
		std::vector<Symbol>::iterator> > iterator;
	typedef boost::tuple<PropNode_ptr, Symbol> ChildRole;

	// Once created, PropNode structural relationships are immutable. Note:
	// This asserts that children exist prior to parent construction
	PropNode(	const DocTheory * dtheory, size_t sent_no,
				const WeightedPredicates & preds,
				const PropNodes & children,
				const std::vector<Symbol> & roles,
				const Proposition * prop = NULL,
				const Mention * ment = NULL,
				const SynNode * synnode = NULL,
				bool exact_match=false);
	
	PropNode(	const DocTheory * dtheory, size_t sent_no,
				const WeightedPredicates & preds,
				const PropNodes & children,
				const std::vector<Symbol> & roles,
				int start_tok_idx, int end_tok_idx,
				bool exact_match);

	const WeightedPredicates & getPredicates()         const { return _ext_preds; }
	const WeightedPredicates & getBasePredicates() const {return _truepreds; }

	const Predicate* getRepresentativePredicate() const;

	size_t getPropNodeID() const { return _propNodeID;}
	size_t getSentNumber() const { return _sent_no;   }
	
	size_t getStartToken() const { return _start_tok; }
	size_t getEndToken()   const { return _end_tok;   }
	size_t getHeadStartToken() const { return _head_start_tok; }
	size_t getHeadEndToken()   const { return _head_end_tok;   }
	
	size_t getNChildren()  const { return _children.size(); }
	
	size_t getTreeDepth()  const;
	size_t getTreeSize()   const;
	
	bool matchRoleExactly() const {return _exact_role_match; }

	const PropNodes           & getChildren()    const { return _children; }
	const std::vector<Symbol> & getRoles()       const { return _roles;    }
	const std::vector<Symbol> & getRolesPlayed() const { return _rolesPlayed; }
	const Symbol& getRoleForChild(const PropNode_ptr& child) const; 
	const ptr_t getFirstChildOfRole(const Symbol& role) const;

	const std::vector<ChildRole> getChildRoles() const;

	bool hasParent() const { return _parents.size() > 0; }
	size_t getNParents() const { return _parents.size(); }
	PropNode_ptr getParent(size_t idx) const { return ptr_t(_parents[idx]); }
	void addParent(PropNode* par);
	const PropNode_ptr getRoot() const;

    // If Boost libraries newer than 1.42.0 enable us to solve the problem described
    // at https://svn.boost.org/trac/boost/ticket/4061, then we should remove the
    // #ifndef AJF_BOOST_PRE_1_42_HACK and #endif lines.
#ifndef AJF_BOOST_PRE_1_42_HACK
	const_iterator begin() const {
		return boost::make_zip_iterator(
			boost::make_tuple(getChildren().begin(), getRoles().begin()));}
	const_iterator end() const {
		return boost::make_zip_iterator(
			boost::make_tuple(getChildren().end(), getRoles().end()));}
#endif

	const DocTheory   * getDocTheory()   const { return _dtheory; }
	const Proposition * getProposition() const { return _prop; }
	const Mention     * getMention()     const { return _ment; }
	const SynNode     * getSynNode()     const { return _synnode; }

	void addPredicate(Predicate pred, float weight) {
		float& v=_ext_preds[pred]; v=std::max(v, weight);
	}

	void clearExpansions() { _ext_preds.clear(); _ext_preds.insert(_truepreds.begin(), _truepreds.end()); }
	
	// Enumerates all nodes & their children
	static void enumAllNodes( const PropNodes & nodes, PropNodes & all_nodes );
	
	// Identifies the 'rootmost' nodes of all disjoint subtrees in the set
	static void enumRootNodes( const PropNodes & nodes, PropNodes & roots );
	// Identifies those nodes in the nodes input that have null parents.
	static void enumNullParentNodes( const PropNodes & nodes, PropNodes & roots );
	
	// Enumerates the rootmost nodes of all disjoint subtrees which are
	//  also the best-covering root for some token in the covered span
	//  - Input must be sorted on PropNode::ID
	//  - Output contains no duplicates and is sorted on PropNode::ID
	static void enumSpanRootNodes( const PropNodes & nodes, PropNodes & span_roots );

	// Get the list of head tokens covered by this node and its children
	static void getCoveredHeadTokens( PropNode_ptr node, std::set<size_t>& headTokens );

	void printSynonyms(std::wostream& s) const;
	void compactPrint(std::wostream& s, bool printTypes=false, bool expandSynonyms=false, bool replace_unknown=false, int indent=0) const;
	void dumpEdges(std::wostream& s, bool print_types, bool expand_synonyms) const;
	void dumpNodes(std::wostream& s, bool print_types, bool expand_synonyms) const;
	void dumpNodeContent(std::wostream &s, bool print_types, bool expand_synonyms) const;

	static void printPredicates(std::wostream& s, const PropNodes& nodes);
private:
	const size_t _propNodeID;
	const DocTheory * _dtheory;
	size_t _sent_no;

	WeightedPredicates _truepreds;
	WeightedPredicates _ext_preds;
	
	const PropNodes _children;
	std::vector<Symbol> _roles;
	std::vector<Symbol> _rolesPlayed;
	std::vector<PropNode*> _parents;

	const Proposition * _prop;
	const Mention * _ment;
	const SynNode * _synnode;
	
	size_t _start_tok;
	size_t _end_tok;
	size_t _head_start_tok;
	size_t _head_end_tok;

	bool _exact_role_match;

	static size_t __unused_PropNodeID;
	
	mutable size_t _ref_cnt;
	
	void init(const Proposition* prop, const Mention* ment, const SynNode* synnode);
	void expandForChildren();

	friend void intrusive_ptr_add_ref( const PropNode * p );
	friend void intrusive_ptr_release( const PropNode * p );

public:
	// orders by ID, which guarentees children come before parents
	struct propnode_id_cmp {
		inline bool operator() (const PropNode_ptr & lhs, const PropNode_ptr & rhs) const {
			return lhs->getPropNodeID() < rhs->getPropNodeID();
		}
		inline bool operator() (const PropNode & lhs, const PropNode_ptr & rhs) const {
			return lhs.getPropNodeID() < rhs->getPropNodeID();
		}
		inline bool operator() (const PropNode_ptr & lhs, const PropNode & rhs) const {
			return lhs->getPropNodeID() < rhs.getPropNodeID();
		}
		inline bool operator() (const PropNode & lhs, const PropNode & rhs) const {
			return lhs.getPropNodeID() < rhs.getPropNodeID();
		}

	};

	struct propnode_len_cmp{
		inline bool operator() (const PropNode_ptr & lhs, const PropNode_ptr & rhs) const {
			return (lhs->getEndToken() - lhs->getStartToken()) > (rhs->getEndToken() - rhs->getStartToken());
		}
	};
};

typedef PropNode::PropNodes PropNodes;
typedef PropNode::PropNode_ptr PropNode_ptr;
typedef PropNode::PropNodes_ptr PropNodes_ptr;

// intrusive_ptr provides an interesting tradeoff: it provides the "lightest possible" reference counting pointer, if 
// the object implements the reference count itself. This isn't so bad after all, when designing your own classes to 
// work with smart pointers; it is easy to embed the reference count in the class itself, to get less memory footprint 
// and better performance. 
inline void intrusive_ptr_add_ref( const PropNode * p ){
	p->_ref_cnt += 1;
}
inline void intrusive_ptr_release( const PropNode * p ){
	if( ! --(p->_ref_cnt) ) delete p;
}

#endif
