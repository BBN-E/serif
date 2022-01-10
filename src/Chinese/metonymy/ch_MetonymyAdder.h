// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_METONYMY_ADDER_H
#define ch_METONYMY_ADDER_H

#include "Generic/common/SymbolHash.h"
#include "Generic/common/DebugStream.h"
#include "Generic/theories/EntityType.h"

class MentionSet;
class PropositionSet;
class Proposition;
class Argument;
class Mention;
class SynNode;

#include "Generic/metonymy/MetonymyAdder.h"


class ChineseMetonymyAdder : public MetonymyAdder {
private:
	friend class ChineseMetonymyAdderFactory;

public:

	void resetForNewSentence() {}
	void resetForNewDocument(DocTheory *docTheory) {}

	virtual void addMetonymyTheory(const MentionSet *mentionSet,
				                   const PropositionSet *propSet);

private:
	ChineseMetonymyAdder();
	bool _use_metonymy;
	bool _use_gpe_roles;

	bool _is_sports_story;

	void processNounOrNamePredicate(Proposition *prop, const MentionSet *mentionSet);
	void processVerbPredicate(Proposition *prop, const MentionSet *mentionSet);
	void processSetPredicate(Proposition *prop, const MentionSet *mentionSet);
	void processLocPredicate(Proposition *prop, const MentionSet *mentionSet);
	void processModifierPredicate(Proposition *prop, const MentionSet *mentionSet);
	void processPossessivePredicate(Proposition *prop, const MentionSet *mentionSet);

	void processGPENounArgument(Argument *arg, const Mention *ref, const MentionSet *mentionSet);
	void processGPEVerbArgument(Argument *arg, const SynNode *verbNode, const Mention *subj, const MentionSet *mentionSet);
	void processORGNounArgument(Argument *arg, const Mention *ref, const MentionSet *mentionSet);
	void processORGVerbArgument(Argument *arg, const SynNode *verbNode, const MentionSet *mentionSet);

	bool isStoryHeader(Mention *ment, const MentionSet *mentionSet);

	void addMetonymyToMention(Mention *mention, EntityType type);

	DebugStream _debug;
};

class ChineseMetonymyAdderFactory: public MetonymyAdder::Factory {
	virtual MetonymyAdder *build() { return _new ChineseMetonymyAdder(); }
};


#endif
