// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DT_REL_SENTENCE_INFO_H
#define DT_REL_SENTENCE_INFO_H

#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/NPChunkTheory.h"

#include "Generic/relations/discmodel/DTRelationSet.h"

class PropTreeLinks;

class DTRelSentenceInfo {
public:
	DTRelSentenceInfo(int n) {		
		tokenSequences = _new const TokenSequence *[n];
		parses = _new const Parse *[n];
		npChunks = _new const NPChunkTheory*[n];
		secondaryParses = _new const Parse *[n];
		mentionSets = _new const MentionSet *[n];
		valueMentionSets = _new const ValueMentionSet *[n];
		propSets = _new PropositionSet *[n];
		relSets = _new DTRelationSet *[n];
		entitySets = _new const EntitySet *[n];
		propTreeLinks = _new const PropTreeLinks *[n];
		documentTopic = Symbol();
		nTheories = n;	
	}

	~DTRelSentenceInfo() {
		delete[] tokenSequences;
		delete[] parses;
		delete[] npChunks;
		delete[] secondaryParses;
		delete[] mentionSets;
		delete[] valueMentionSets;
		delete[] propSets;
		delete[] relSets;
		delete[] entitySets;
		if ( propTreeLinks ) delete[] propTreeLinks;
	}

	void populate(const EntitySet *eset, const Parse* parse, const Parse* secondaryParse, 
				const MentionSet* mentionSet, const ValueMentionSet *valueMentionSet, 
				PropositionSet* propSet, const NPChunkTheory* npChunkParse, Symbol docTopic,
				const TokenSequence *tokens = 0, const PropTreeLinks* ptLinks = 0) {
		tokenSequences[0] = tokens;
		parses[0] = parse;
		secondaryParses[0] = secondaryParse;
		mentionSets[0] = mentionSet;
		valueMentionSets[0] = valueMentionSet;
		propSets[0] = propSet;
		relSets[0] = 0;
		entitySets[0] = eset;
		propTreeLinks[0] = ptLinks;
		documentTopic = docTopic;
	}

	const TokenSequence** tokenSequences;
	const Parse** parses;
	const Parse** secondaryParses;
	const MentionSet** mentionSets;
	const ValueMentionSet** valueMentionSets;
	const NPChunkTheory** npChunks;
	PropositionSet** propSets;
	DTRelationSet** relSets;
	const EntitySet **entitySets;
	const PropTreeLinks** propTreeLinks;
	Symbol documentTopic;
	int nTheories;
};

#endif
