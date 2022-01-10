// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DOC_ENTITY_LINKER_H
#define DOC_ENTITY_LINKER_H

class DocTheory;

#include "Generic/CASerif/correctanswers/CorrectAnswers.h"

/**
 * Do document-wide entity linking
 */
class DocEntityLinker {
public:
	DocEntityLinker(bool use_correct_coref = false);
	~DocEntityLinker();
	void cleanup();

	void setCorrectAnswers(CorrectAnswers *correctAnswers) 
	{
		_correctAnswers = correctAnswers;
	}

	void linkEntities(DocTheory* docTheory);
private:
	class StrategicEntityLinker *_stratLinker;
	class ReferenceResolver *_referenceResolver;
	class EntitySetBuilder *_entitySetBuilder;

	enum { SENTENCE, OUTSIDE, MENTION_TYPE, CORRECT_ANSWER, MENTION_GROUP };
	int _mode;

	void importOutsideCoref(DocTheory* docTheory);
	std::string _outside_coref_directory;
	int _second_pass_desc_overgen;
	double _second_pass_desc_me_threshold;
	void doMentionGroupCoref(DocTheory* docTheory);
	void doSentenceBySentenceCoref(DocTheory* docTheory);
	void doMentionTypeSentenceBySentenceCoref(DocTheory* docTheory);

	bool _use_correct_answers;
	CorrectAnswers *_correctAnswers;
};
#endif
