// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_DOCUMENT_RELATION_FINDER_H
#define EN_DOCUMENT_RELATION_FINDER_H

#include "docRelationsEvents/DocumentRelationFinder.h"
#include "common/limits.h"
#include "theories/Proposition.h"

class DebugStream;
class Symbol;
class RelMention;
class SentenceTheory;

class EnglishDocumentRelationFinder : public DocumentRelationFinder 
{

public:

	EnglishDocumentRelationFinder();
	~EnglishDocumentRelationFinder() {}

	void findRelations(DocTheory* docTheory);

private:

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

#endif
