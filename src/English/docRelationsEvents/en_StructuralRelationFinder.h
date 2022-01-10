// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_STRUCTURAL_RELATION_FINDER_H
#define EN_STRUCTURAL_RELATION_FINDER_H

#include "Generic/docRelationsEvents/StructuralRelationFinder.h"
#include "Generic/common/limits.h"
#include "Generic/theories/Proposition.h"

class DebugStream;
class Symbol;
class RelMention;
class SentenceTheory;

class EnglishStructuralRelationFinder : public StructuralRelationFinder {
private:
	friend class EnglishStructuralRelationFinderFactory;

	~EnglishStructuralRelationFinder() {}

	RelMentionSet *findRelations(DocTheory *docTheory);

	bool isActive() { return _find_itea_document_relations; }

private:

	EnglishStructuralRelationFinder();

	DebugStream _debugStream;
	bool _find_itea_document_relations;

	RelMention *_relMentionList[MAX_DOCUMENT_RELATIONS];
	int _n_relations;

	void createStructureBasedRelations(const Mention *arg2, DocTheory *docTheory, int start_sentence);

    const Mention* findCandidateArg2Mention(SentenceTheory* st);
	const Mention* grabMentionFromSentence(SentenceTheory *st);

	const Mention* findOrgConsisting(SentenceTheory* st);
	const Mention* findForOrgFollows(SentenceTheory* st);
	const Mention* findOrgIsMadeOf(SentenceTheory* st);
	const Mention* findFollowingOrgPerson(SentenceTheory* st);

    bool sentenceIsTrivial(SentenceTheory* st);
	bool sentenceStartsAlphaNumericList(SentenceTheory* st);
	bool isListItemElement(SentenceTheory *st);
	bool sentenceStartsWithDash(SentenceTheory* st);
	bool containsListStopper(SentenceTheory* st);

	void createAlphaNumericListRelations(const Mention *arg2, DocTheory *docTheory, int start_sentence);
	void createDashListRelations(const Mention *arg2, DocTheory *docTheory, int start_sentence);

	Symbol getRelationType(const Mention *arg1, const Mention *arg2);

	const Proposition* findPropositionArg(Proposition *prop, 
						                  Symbol roleSym, 
										  Proposition::PredType predType, 
										  Symbol stemmedWord) const;



};

class EnglishStructuralRelationFinderFactory: public StructuralRelationFinder::Factory {
	virtual StructuralRelationFinder *build() { return _new EnglishStructuralRelationFinder(); }
};


#endif
