#ifndef EN_PRE_LINKER_H
#define EN_PRE_LINKER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/edt/xx_PreLinker.h"
#include "Generic/common/Symbol.h"
class Proposition;
class SynNode;

class EnglishPreLinker : public GenericPreLinker {
public:
	// These methods override defintions given in PreLinker:
	static void preLinkSpecialCases(MentionMap &preLinks,
									const MentionSet *mentionSet,
									const PropositionSet *propSet);
	static void preLinkTitleAppositives(MentionMap &preLinks,
									    const MentionSet *mentionSet);

public:
	// These methods are english-only, and must be accessed via
	// the EnglishPreLinker class (not via PreLinker):
	static bool getTitle(const MentionSet *mentionSet, 
		const Mention *mention, MentionMap *preLinks=0);

	static void preLinkBody(MentionMap &preLinks,
							const Proposition *prop,
							const MentionSet *mentionSet);
	static void preLinkGPELocDescsToNames(MentionMap &preLinks,
								   const Proposition *prop,
								   const MentionSet *mentionSet);
	static void preLinkGovernment(MentionMap &preLinks,
								  const Proposition *prop,
								  const MentionSet *mentionSet);
	static void preLinkOrgNameDescs(MentionMap &preLinks,
									const Proposition *prop,
									const MentionSet *mentionSet);


	static void preLinkWHQCopulas(MentionMap &preLinks, 
		                          const Proposition *prop, 
								  const MentionSet *mentionSet);
	
	static void preLinkNationalityPeople(MentionMap &preLinks,
		const Proposition *prop,
		const MentionSet *mentionSet);

	static void preLinkContextLinks(MentionMap &preLinks,
									const MentionSet *mentionSet);
	static bool preLinkContextLinksGivenNode(MentionMap &preLinks,
											 const MentionSet *mentionSet,
											 Mention *mention,
											 const SynNode *node);

	/*
	// +++ JJO 29 July 2011 +++
	// Relative pronouns
	static void preLinkRelativePersonPronouns(MentionMap &preLinks,
										const Proposition *prop,
										const MentionSet *mentionSet);
	static bool isRelativePersonPronoun(const Symbol sym);
	// --- JJO 29 July 2011 ---
	*/

	static void preLinkRelativePronouns(MentionMap &preLinks,
										const Proposition *prop,
										const MentionSet *mentionSet);
private:
	static Mention *getMentionFromTerminal(const MentionSet *mentionSet,
										   const SynNode *terminal);
public:
	static const Mention *getWHQLink(const Mention *currMention, 
							                    const MentionSet *mentionSet); 
private:
	static bool theIsOnlyPremod(const SynNode *node);
	
	static bool isNonTitleWord(Symbol word);

	static bool isGeocoord(const Mention *mention);

	static Mention *getAppropriateGeocoordLink(const MentionSet *mentionSet, const SynNode *terminalNode, Mention *mention);

};



#endif
