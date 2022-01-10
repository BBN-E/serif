#include "common/leak_detection.h"

#include "MentionToPropNodeMap.h"

#include <map>
#include <vector>
#include <algorithm>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include "common/foreach_pair.hpp"
#include "theories/DocTheory.h"
#include "theories/Mention.h"
#include "theories/SentenceTheory.h"
#include "theories/MentionSet.h"
#include "PropNode.h"
#include "DocPropForest.h"

using std::make_pair;
using boost::make_shared;

MentionToPropNodeMap_ptr MentionToPropNode::buildMap(const DocPropForest& forest) {
	return buildMap(forest, 0, (std::max)((size_t)0,forest.size()-1));
}

MentionToPropNodeMap_ptr MentionToPropNode::buildMap(const DocPropForest& forest,
		size_t start_sentence, size_t end_sentence)
{
	MentionToPropNodeMap_ptr ret = make_shared<MentionToPropNodeMap>();

	for (size_t i=start_sentence; i<forest.size() && i<=end_sentence; ++i) {
		const PropNodes_ptr& nodes = forest[i];
		BOOST_FOREACH(const PropNode_ptr& node, *nodes) {
			buildMap(node, *ret);
		}
	}

	return ret;
}

void MentionToPropNode::buildMap(const PropNode_ptr& node, MentionToPropNodeMap& mToP) {
	const Mention* m = node->getMention();
	if (m) {
		mToP.insert(make_pair(m, node));
	}
	
	BOOST_FOREACH(const PropNode_ptr& kid, node->getChildren()) {
		buildMap(kid, mToP);
	}
}

PropNodes_ptr MentionToPropNode::propNodesForEntity(const Entity* e,
		const DocTheory* dt, const MentionToPropNodeMap& mToP) 
{
	PropNodes_ptr ret = make_shared<PropNodes>();
	
	for (int i=0; i<e->getNMentions(); ++i) {
		MentionUID m_id = e->getMention(i);
		int sn = Mention::getSentenceNumberFromUID(m_id);
		const Mention* m = dt->getSentenceTheory(sn)->getMentionSet()->getMention(m_id);

		MentionToPropNodeMap::const_iterator probe = mToP.find(m);

		if (probe != mToP.end()) {
			ret->push_back(probe->second);
		}
	}
			
	return ret;
}

