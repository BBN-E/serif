// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/xdoc/TokenSubsetTrees.h"
#include "Generic/xdoc/EditDistance.h"
#include <iostream>
#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

/* For a description of TokenSubsetTrees, see:
   https://wiki.d4m.bbn.com/wiki/XDoc-lite#Step_4:_Token_Subset_Trees_.28TSTs.29
*/

using namespace std;

namespace {
	bool lengthSort (const wstring &a, const wstring &b) { return a.length() < b.length(); }
}

TokenSubsetTrees::TokenSubsetTrees() { }

TokenSubsetTrees::~TokenSubsetTrees() { 
	deleteNodes();
}

void TokenSubsetTrees::deleteNodes() {
	BOOST_FOREACH(TSTNode *node, _allNodes) {
		delete node;
	}
	_allNodes.clear();

	boost::unordered_map<int, TSTNode *>::iterator it;
	for (it = _allRoots.begin(); it != _allRoots.end(); ++it) {
		delete it->second;
	}
	_allRoots.clear();

	_tokens.clear();
	_nodeMap.clear();
	_unambiguousLeaf2NodeMap.clear();
}

void TokenSubsetTrees::initializeTrees(vector<wstring> &names) {
	_tokens.clear();
	deleteNodes();

	// intern lower-cased tokens of the name to integers.
	BOOST_FOREACH(wstring &name, names) {
		transform(name.begin(), name.end(), name.begin(), towlower);
		vector<wstring> words;
		boost::split(words, name, boost::is_any_of(L" "));
		BOOST_FOREACH(wstring &word, words) {
			if (_tokens.find(word) == _tokens.end())
				_tokens[word] = static_cast<int>(_tokens.size());
		}
	}

	sort(names.begin(), names.end(), lengthSort);

	//std::cout << "Working with " << names.size() << " names.\n"; std::cout.flush();
	int count = 0;
	BOOST_FOREACH(wstring &name, names) {
		//count++;
		//if (count % 5000 == 0) {
		//	std::cout << count << "\n"; std::cout.flush();
		//	std::cout << "Num roots: " << _allRoots.size() << "\n"; std::cout.flush();
		//}
		if (_nodeMap.find(name) != _nodeMap.end())
			continue;

		set<int> tokIds = getTokIds(name);
		TSTNode *node = new TSTNode(tokIds, name);
		_nodeMap[name] = node;

		BOOST_FOREACH(int tok_id, tokIds) {
			TSTNode *curRoot = 0;
			if (_allRoots.find(tok_id) != _allRoots.end()) {
				curRoot = _allRoots[tok_id];
			} else {
				set<int> tokSet;
				tokSet.insert(tok_id);
				curRoot = new TSTNode(tokSet, L"--ROOT--");
				_allRoots[tok_id] = curRoot;
			}
			curRoot->insertNode(node);
		}
		_allNodes.push_back(node);
	}

	reverse(_allNodes.begin(), _allNodes.end());
	markUnambiguousLeafs();
}

void TokenSubsetTrees::markUnambiguousLeafs() {
	// _allNodes must be ordered from children to parent. 
	// For each node, determines and sets the unambiguous leaf-most
    // child the node is connected to, if there is one.

	BOOST_FOREACH(TSTNode *node, _allNodes) {
		if (node->children.size() == 0)
			node->unambiguousLeaf = node;
		else {
			set<TSTNode *> childLeafs;
			BOOST_FOREACH(TSTNode *child, node->children) 
				childLeafs.insert(child->unambiguousLeaf);
			if (childLeafs.find(0) != childLeafs.end() || childLeafs.size() != 1) 
				continue;
			TSTNode *onlyChildLeaf = *(childLeafs.begin());
			node->unambiguousLeaf = onlyChildLeaf;

			if (_unambiguousLeaf2NodeMap.find(onlyChildLeaf) == _unambiguousLeaf2NodeMap.end())
				_unambiguousLeaf2NodeMap[onlyChildLeaf] = vector<TSTNode*>();
			_unambiguousLeaf2NodeMap[onlyChildLeaf].push_back(node);

			//std::cout << "Unambiguous leaf of " << UnicodeUtil::toUTF8StdString(node->value) << " is " << UnicodeUtil::toUTF8StdString(node->unambiguousLeaf->value) << "\n";
		}
	}
}

set<int> TokenSubsetTrees::getTokIds(wstring &name) {
	set<int> result;
	vector<wstring> words;
	boost::split(words, name, boost::is_any_of(L" "));
	BOOST_FOREACH(wstring &word, words) {
		result.insert(_tokens[word]);
	}
	return result;
}

/* TSTNode functions */
void TokenSubsetTrees::TSTNode::insertNode(TSTNode *node) {
	//Recursively identifies the appropriate place in the TST for node.
	
	// If we've already reached & inserted ourselves in
    // this portion of the graph, backtrack back up
	if (find(children.begin(), children.end(), node) != children.end()) 
		return;

	bool ins = false;
	BOOST_FOREACH(TSTNode *c, children) {
		// Recursive case: if child is a subset
        // of node, then node belongs under it.
		if (c->isSubset(node)) {
			c->insertNode(node);
			ins = true;
		}
	}

	// Base case: No children appropriate to add node to. It must belong here.
	// There can be no superset siblings, because we construct in strictly
	// increasing set order.
	if (!ins) {
		//if (value != L"--ROOT--")
		//	std::cout << UnicodeUtil::toUTF8StdString(node->value) << " is child of " <<  UnicodeUtil::toUTF8StdString(value) << "\n";
		children.push_back(node);
		node->parent = this;
		return;
	}
}

bool TokenSubsetTrees::TSTNode::isSubset(TSTNode *node) {
	BOOST_FOREACH(int tok_id, toks) {
		if (find(node->toks.begin(), node->toks.end(), tok_id) == node->toks.end()) 
			return false;
	}
	return true;
}

vector<wstring> TokenSubsetTrees::getTSTAliases(wstring name) {
	vector<wstring> results;

	transform(name.begin(), name.end(), name.begin(), towlower);
	if (_nodeMap.find(name) == _nodeMap.end())
		throw InternalInconsistencyException("TokenSubsetTrees::getTSTAliases", "name has not been added to TokenSubsetTrees");
	TSTNode *node = _nodeMap[name];

	if (node->unambiguousLeaf && node->unambiguousLeaf != node)
		results.push_back(node->unambiguousLeaf->value);
	
	if (_unambiguousLeaf2NodeMap.find(node) != _unambiguousLeaf2NodeMap.end()) {
		BOOST_FOREACH(TSTNode *n, _unambiguousLeaf2NodeMap[node]) 
			if (n != node)
				results.push_back(n->value);
	}

	return results;
}


vector<wstring> TokenSubsetTrees::getTSTOneCharChildren(wstring name) {
	vector<wstring> results;

	transform(name.begin(), name.end(), name.begin(), towlower);
	if (_nodeMap.find(name) == _nodeMap.end())
		throw InternalInconsistencyException("TokenSubsetTrees::getTSTOneCharChildren", "name has not been added to TokenSubsetTrees");
	TSTNode *node = _nodeMap[name];

	BOOST_FOREACH(TSTNode *child, node->children) {
		if (child->value.length() - 2 <= node->value.length())
			results.push_back(child->value);
	}

	// only include TST-parents if parent > 1 token
	TSTNode *parent = node->parent;
	if (node->value.length() - 2 <= parent->value.length() && 
		parent->value.find(L' ') != wstring::npos &&
		parent->parent)
	{
		results.push_back(parent->value);
	}

	return results;
}

vector<wstring> TokenSubsetTrees::getEditDistTSTChildren(wstring name) {
	vector<wstring> results;

	transform(name.begin(), name.end(), name.begin(), towlower);
	if (_nodeMap.find(name) == _nodeMap.end())
		throw InternalInconsistencyException("TokenSubsetTrees::getEditDistTSTChildren", "name has not been added to TokenSubsetTrees");
	TSTNode *node = _nodeMap[name];

	BOOST_FOREACH(TSTNode *child, node->children) {
		if (_editDistance.similarity(node->value, child->value) >= 0.8)
			results.push_back(child->value);
	}

	return results;
}
