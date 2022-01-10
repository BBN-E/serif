// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTION_EXPANDER_H
#define MENTION_EXPANDER_H

#include "PropTreeExpander.h"

class Mention;

class MentionExpander : public PropTreeExpander {
public: 
	// if lowercase is true, when a mention is found through
	// mention expansion, a lowercase copy is added as well,
	// unless it is a name.
	MentionExpander(bool lowercase) : PropTreeExpander(), 
		_lowercase(lowercase) {}
	void expand(const PropNodes& pnodes) const = 0;
protected:
	bool doesLowercasing() const {return _lowercase; }

	static void getAllMents(const Mention* ment, const DocTheory* dt,
							std::vector<std::pair<const Mention*, float> >&  allMents);
	static float mentionExpansionCost(const Mention * src, const Mention * trg) {return 1.0f;};
	static void addMentionPredicate(const PropNode_ptr& node, const Mention * ment, float weight, bool lc);
private:
	bool _lowercase;
};


#endif

