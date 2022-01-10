// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RAW_RELATION_FINDER
#define RAW_RELATION_FINDER


class RelMention;
class Proposition;
class MentionSet;
class Mention;

/** This just simple-mindedly turns some propositions into relations. It
  * can be used in addition to the real relation finder to add more relations
  * and make demos more impressive. */

class RawRelationFinder {
public:
	static RelMention *getRawRelMention(Proposition *prop,
										const MentionSet *mentionSet,
										int sent_no, int rel_no);

	static bool isWorthwhileMention(const Mention *mention);
};


#endif

