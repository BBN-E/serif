// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_PRELINKER_H
#define xx_PRELINKER_H

#include "Generic/edt/PreLinker.h"

class GenericPreLinker {
	// Note: this class is intentionally not a subclass of PreLinker.
	// See PreLinker.h for an explanation.
public:
	typedef std::map<int, const Mention*> MentionMap;

	// These methods are given default definitions:
	static void preLinkAppositives(MentionMap &preLinks,
								   const MentionSet *mentionSet);
	static void preLinkCopulas(MentionMap &preLinks,
							   const MentionSet *mentionSet,
							   const PropositionSet *propSet);
	static void setSpecialCaseLinkingSwitch(bool linking_switch);
	static void setEntitySubtypeFilteringSwitch(bool filtering_switch);

	// These methods are given trivial default definitions (i.e., do
	// nothing):
	static const int postLinkSpecialCaseNames(const EntitySet* entitySet, 
											  int* entity_link_pairs, int maxpairs);
	static void preLinkSpecialCases(MentionMap &preLinks,
									const MentionSet *mentionSet,
									const PropositionSet *propSet);
	static void preLinkTitleAppositives(MentionMap &preLinks,
										const MentionSet *mentionSet);

protected:
	/** create a pre-link from m1 to m2 or from m2 to m1, depending on
	  * the mention types */
	static void createPreLink(MentionMap &preLinks,
							  const Mention *m1, const Mention *m2);

	static bool subtypeMatch(const Mention *ment1, const Mention *ment2);

	static bool LINK_SPECIAL_CASES;
	static bool FILTER_BY_ENTITY_SUBTYPE;
};
#endif
