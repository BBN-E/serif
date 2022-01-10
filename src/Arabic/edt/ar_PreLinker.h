// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_PRELINKER_H
#define ar_PRELINKER_H

#include "Generic/edt/xx_PreLinker.h"
#include "Generic/common/Symbol.h"

class EntitySet;

//WARNING: This is an empty prelinker, it will not do anything!
class ArabicPreLinker : public GenericPreLinker {
public:
	static void preLinkSpecialCases(MentionMap &preLinks,
									const MentionSet *mentionSet,
									const PropositionSet *propSet);
	
	static void preLinkTitleAppositives(MentionMap &preLinks,
									    const MentionSet *mentionSet);

	//for now this handles metonymy like links of Washington 
	//unlike other prelinker this links entities.  U
	static const int postLinkSpecialCaseNames(const EntitySet* entitySet, int* entity_link_pairs, int maxpairs);

private:
	static const Mention* preLinkPerTitles(const MentionSet *mentionSet, const Mention* ment);
	static const Mention* preLinkHeadOf(const MentionSet *mentionSet, const Mention* ment);
	static const int preLinkPerNationalities(MentionMap &link_pairs, const MentionSet *mentionSet);

private:
	static const bool _matchAmerica(Symbol word);
};
#endif
