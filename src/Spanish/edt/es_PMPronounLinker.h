// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_PM_PRONOUN_LINKER_H
#define es_PM_PRONOUN_LINKER_H

#include "Spanish/edt/es_PronounLinker.h"
#include "Generic/edt/MentionLinker.h"
#include "Generic/theories/Parse.h"
#include "Generic/common/ProbModel.h"
#include "Generic/theories/SynNode.h"
#include "Generic/edt/LinkGuess.h"

#include <vector>

class SpanishPMPronounLinker : public PronounLinker {
public:
	SpanishPMPronounLinker();
	~SpanishPMPronounLinker();

	virtual void addPreviousParse(const Parse *parse);
	virtual int linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, EntityType linkType, LexEntitySet *results[], int max_results);
	virtual void resetPreviousParses();

	virtual int getLinkGuesses(EntitySet *currSolution, Mention *currMention, LinkGuess results[], int max_results);

	virtual void resetForNewDocument(Symbol docName) { PronounLinker::resetForNewDocument(docName); }
	void resetForNewDocument(DocTheory *docTheory) { _docTheory = docTheory; }
private:

	bool _isLinkedBadly(EntitySet *currSolution, Mention* pronMention, Entity* linkedEntity);
	char *debugTerminals(const SynNode *node);
	void _initialize(const char* model_prefix);
	void verifyEntityTypes(const char *file_name);

	DocTheory *_docTheory;

	std::vector<const Parse *> _previousParses;
	ProbModel *_priorAntModel;
	ProbModel *_hobbsDistanceModel;
	ProbModel *_pronounGivenAntModel;
	ProbModel *_parWordGivenAntModel;

	static DebugStream &_debugStream;

	bool _isSpeakerMention(Mention *ment);	
	bool _isSpeakerEntity(Entity *ent, EntitySet *ents);

	bool _use_correct_answers;
};

#endif
