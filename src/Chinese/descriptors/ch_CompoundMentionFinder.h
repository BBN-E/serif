// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_COMPOUND_MENTION_FINDER_H
#define CH_COMPOUND_MENTION_FINDER_H

#include "Generic/common/Symbol.h"
#include "Generic/descriptors/CompoundMentionFinder.h"
class Mention;
class MentionSet;
class SynNode;
class SymbolListMap;
class CorrectMention;
class CorrectEntity;
class CorrectAnswers;
class CorrectDocument;

#define MAX_RETURNED_MENTIONS 100

/**
  * This singleton class contains heuristics for detecting 
  * Chinese compound mentions (e.g. appositives, lists, etc.)
  */
class ChineseCompoundMentionFinder : public CompoundMentionFinder {
private:
	// This class should only be instantiated via CompoundMentionFinder::getInstance()
	ChineseCompoundMentionFinder();
	friend struct CompoundMentionFinder::FactoryFor<ChineseCompoundMentionFinder>;
	// This class should only be deleted via CompoundMentionFinder::deleteInstance()
	~ChineseCompoundMentionFinder();
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

	/**
	  * Looks for an appositive construction within baseMention
	  * @param mentionSet the set of all available mentions
	  * @param baseMention the mention beneath which we search
	  * @return a pointer to a 0-terminated static array of pointers
	  *		to appositive sub-mentions, or 0 if no appositive exists
	  */	  
	virtual Mention **findAppositiveMemberMentions(MentionSet *mentionSet,
												   Mention *baseMention);

	/**
	  * Looks for a list construction within baseMention
	  * @param mentionSet the set of all available mentions
	  * @param baseMention the mention beneath which we search
	  * @return a pointer to a 0-terminated static array of pointers
	  *		to list member mentions, or 0 if no list exists
	  */	  
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

	SymbolListMap *_descWordMap;

	// using a parameter, fill the map with head word info
	void _loadDescWordMap();
	
	// recursively determine how many name mentions are in 
	// a tree of hierarchical mentions
	static int _countNamesInMention(Mention* ment);


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
	void setCorrectMentionForNode(const SynNode *node);

};


#endif