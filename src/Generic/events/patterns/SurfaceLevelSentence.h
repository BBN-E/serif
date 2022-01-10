// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SURFACE_LEVEL_SENTENCE_H
#define SURFACE_LEVEL_SENTENCE_H

#include "Generic/common/Symbol.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/Token.h"
class Mention;
class MentionSet;
class SynNode;

class SurfaceLevelSentence {
public:
	SurfaceLevelSentence(const SynNode *node, MentionSet *mentionSet);
	std::string toDebugString() const;
	std::wstring toString() const;

	int getTokenIndex(int index) { return _surfaceSentence[index].token_index; }
	const SynNode *getNode(int index) { return _surfaceSentence[index].node; }
	const Mention* getMention(int index) { return _surfaceSentence[index].mention; }
	Symbol getWord(int index) { return _surfaceSentence[index].token; }
	int getLength() { return _length; }

private:
	struct SurfaceEntry {
		const Mention *mention;
		const SynNode *node;
		Symbol token;
		int token_index;
	};

	SurfaceEntry _surfaceSentence[MAX_SENTENCE_TOKENS];


	int _length;
	void addToSentence(const SynNode *node);

	MentionSet *_mentionSet;


};

#endif
