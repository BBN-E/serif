// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_POTENTIAL_RELATION_INSTANCE_H
#define AR_POTENTIAL_RELATION_INSTANCE_H

#include "Generic/common/Symbol.h"
#include "Generic/theories/Mention.h"
#include "Generic/relations/PotentialRelationInstance.h"
class Argument;
class Proposition;
class MentionSet;
class TokenSequence;
class RelationObservation;
#include <string>

#define AR_POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE + 3


class ArabicPotentialRelationInstance : public PotentialRelationInstance {
public:
	ArabicPotentialRelationInstance();
	ArabicPotentialRelationInstance(const ArabicPotentialRelationInstance &other);

	void setStandardInstance(Mention *first, Mention *second, const TokenSequence *tokenSequence);
	void setStandardInstance(RelationObservation *obs);	
	void setFromTrainingNgram(Symbol *ngram);
	Symbol* getTrainingNgram();

	Symbol getLeftMentionType();
	Symbol getRightMentionType();
	Symbol getWordsBetween();

	std::wstring toString();
	std::string toDebugString();

	void setLeftMentionType(Symbol sym);
	void setRightMentionType(Symbol sym);
	void setWordsBetween(Symbol sym);
	void setLeftMentionType(Mention::Type type);
	void setRightMentionType(Mention::Type type);

	static Symbol TOO_LONG;
	static Symbol ADJACENT;
	static Symbol CONFUSED;

private:
	Symbol _ngram[AR_POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE];

	Symbol convertMentionType(Mention::Type type);
	const SynNode* findNPChunk(const SynNode *node);

	Symbol findWordsBetween(Mention *m1, Mention *m2, const TokenSequence *tokenSequence);
	Symbol makeWBSymbol(int start, int end, const TokenSequence *tokenSequence);
	Symbol findStem(const SynNode *node, const TokenSequence *tokenSequence);
};


#endif
