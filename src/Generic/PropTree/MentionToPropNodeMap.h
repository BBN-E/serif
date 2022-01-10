// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTIONTOPROPNODEMAP_H
#define MENTIONTOPROPNODEMAP_H

#include <map>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "PropNode.h"

class DocTheory;
class Mention;
class Entity;
class DocPropForest;

namespace MentionToPropNode {
	typedef std::map<const Mention*, PropNode_ptr > MentionToPropNodeMap;
	typedef boost::shared_ptr<MentionToPropNodeMap> MentionToPropNodeMap_ptr;

	MentionToPropNodeMap_ptr buildMap(const DocPropForest& forest);
	MentionToPropNodeMap_ptr buildMap(const DocPropForest& forest, 
			size_t start_sentence, size_t end_sentence);
	void buildMap(const PropNode_ptr& node, MentionToPropNodeMap& mToP);
	PropNodes_ptr propNodesForEntity(const Entity* e, const DocTheory* dt,
		const MentionToPropNodeMap& mToP);

};

// promote to global namespace
using MentionToPropNode::MentionToPropNodeMap;
using MentionToPropNode::MentionToPropNodeMap_ptr;

#endif

