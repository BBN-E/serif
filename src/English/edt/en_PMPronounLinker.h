// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_PMPRONOUNLINKER_H
#define en_PMPRONOUNLINKER_H
#include "English/edt/en_PronounLinker.h"
#include "Generic/edt/MentionLinker.h"
#include "Generic/theories/Parse.h"
#include "Generic/common/ProbModel.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/DebugStream.h"
#include "Generic/edt/LinkGuess.h"

#include <vector>

class EnglishPMPronounLinker : public PronounLinker {
public:
	EnglishPMPronounLinker();
	~EnglishPMPronounLinker();

	virtual void addPreviousParse(const Parse *parse);
	virtual int linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, EntityType linkType, LexEntitySet *results[], int max_results);
	virtual void resetPreviousParses();

	int getLinkGuesses(LexEntitySet *currSolution, Mention *currMention, LinkGuess results[], int max_results);
	int getWHQLinkGuesses(LexEntitySet *currSolution, Mention *currMention, LinkGuess results[], int max_results);

	static DebugStream &getDebugStream() {return _debugOut; }
	virtual void resetForNewDocument(Symbol docName) { PronounLinker::resetForNewDocument(docName); }
	void resetForNewDocument(DocTheory *docTheory) { _docTheory = docTheory; }

private:

	bool _isLinkedBadly(LexEntitySet *currSolution, Mention* pronMention, Entity* linkedEntity);
	char *debugTerminals(const SynNode *node);
	void _initialize(const char* model_prefix);
	void verifyEntityTypes(char *file_name);

	DocTheory *_docTheory;

	std::vector<const Parse *> _previousParses;
	ProbModel *_priorAntModel;
	ProbModel *_hobbsDistanceModel;
	ProbModel *_pronounGivenAntModel;
	ProbModel *_parWordGivenAntModel;

	static DebugStream _debugOut;

	bool _isSpeakerMention(Mention *ment);	
	bool _isSpeakerEntity(Entity *ent, LexEntitySet *ents);

	bool _use_correct_answers;
};

#endif
