// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TOKEN_SUBSET_TREES_H
#define TOKEN_SUBSET_TREES_H

#include "Generic/common/Symbol.h"
#include "Generic/xdoc/EditDistance.h"

#include <vector>
#include <set>
#include <string>
#include <boost/unordered_map.hpp>


class TokenSubsetTrees {

public:
	TokenSubsetTrees();
	~TokenSubsetTrees();

	void initializeTrees(std::vector<std::wstring> &names);
	std::vector<std::wstring> getTSTAliases(std::wstring name);
	std::vector<std::wstring> getTSTOneCharChildren(std::wstring name);
	std::vector<std::wstring> getEditDistTSTChildren(std::wstring name);
	
private:
	struct TSTNode {
		std::set<int> toks;
		std::wstring value;
		std::vector<TSTNode *> children;
		TSTNode *unambiguousLeaf;
		TSTNode *parent;

		TSTNode(std::set<int> t, std::wstring v) {
			toks = t;
			value = v;
			unambiguousLeaf = 0;
			parent = 0;
		}

		void insertNode(TSTNode *node);
		bool isSubset(TSTNode *node);
	};

	boost::unordered_map<Symbol, int> _tokens; // map from token to token_id
	std::vector<TSTNode *> _allNodes; // ordered list
	boost::unordered_map<int, TSTNode *> _allRoots; // tree roots, these represent unique tokens, not names
	boost::unordered_map<std::wstring, TSTNode*> _nodeMap; // map from name to TSTNode
	boost::unordered_map<TSTNode*, std::vector<TSTNode*> > _unambiguousLeaf2NodeMap;

	std::set<int> getTokIds(std::wstring &name);

	void deleteNodes();
	void markUnambiguousLeafs();

	EditDistance _editDistance;

};

#endif

