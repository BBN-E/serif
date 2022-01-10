// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_COMPOUND_MENTION_FINDER_H
#define es_COMPOUND_MENTION_FINDER_H

#include "Generic/common/Symbol.h"
#include "Generic/descriptors/CompoundMentionFinder.h"

class Mention;
class MentionSet;
class SynNode;
class CorrectMention;
class CorrectEntity;
class CorrectAnswers;
class CorrectDocument;


#define MAX_RETURNED_MENTIONS 100
/*
* NOTE: This does not do anything yet! It always returns 0
*
*/
class SpanishCompoundMentionFinder : public CompoundMentionFinder {
private:
	// This class should only be instantiated via CompoundMentionFinder::getInstance()
	SpanishCompoundMentionFinder();
	friend struct CompoundMentionFinder::FactoryFor<SpanishCompoundMentionFinder>;
	// This class should only be deleted via CompoundMentionFinder::deleteInstance()
	~SpanishCompoundMentionFinder();
public:
	// These are heuristics for detecting compound mentions.
	// They return the submention(s) you ask for, or 0
	// if they find that the mention is not of the right type.

	// * CAUTION! THERE IS TRICKERY HERE: *
	// The order of these is important. The four types of mentions
	// tested for (nested, partitive, appos, list) are mutually
	// exclusive, so each of these tests is allowed to assume that
	// each previous test failed (returned 0) (or would have if they
	// haven't actually been called). For example, don't call
	// findNestedMention() on an appositive.

	virtual Mention *findPartitiveWholeMention(MentionSet *mentionSet,
											   Mention *baseMention);

	// these two return a pointer to a 0-terminated *static* array 
	// of pointers to mentions.
	virtual Mention **findAppositiveMemberMentions(MentionSet *mentionSet,
												   Mention *baseMention);

	virtual Mention **findListMemberMentions(MentionSet *mentionSet,
											 Mention *baseMention);

	/**
	 * Look for a mention in the head path of the baseMention
	 * @param mentionSet the set of all available mentions
	 * @param baseMention the mention beneath which we search
	 * @return a pointer to a nested mention, or 0 if none exists
	 */
	virtual Mention *findNestedMention(MentionSet *mentionSet,
									   Mention *baseMention);
	/**
	 * If necessary, change entity types of these appositive members
	 * @param mentions the members of an appositive
	 */
	virtual void coerceAppositiveMemberMentions(Mention** mentions);

private:
	static CompoundMentionFinder* _instance;
	Mention *_mentionBuf[MAX_RETURNED_MENTIONS+1];

	//try to find partitives

	static int _n_partitive_headwords;
	static Symbol _partitiveHeadwords[];
	static bool isPartitiveHeadWord(Symbol sym);
	static bool isNumber(const SynNode *node);
	static void initializeSymbols();
	
public:
	bool _use_correct_answers;
	// These are all used by CorrectAnswerSerif:
	// (Do the variables need to be public?)
	CorrectMention *_correctMention;
	CorrectEntity *_correctEntity;
	CorrectAnswers *_correctAnswers;
	CorrectDocument *_correctDocument;
	int _sentence_number;
	void setCorrectAnswers(CorrectAnswers *correctAnswers) { _correctAnswers = correctAnswers; }
	void setCorrectDocument(CorrectDocument *correctDocument) { _correctDocument = correctDocument; }
	void setSentenceNumber(int sentno) { _sentence_number = sentno; }


};





#endif
