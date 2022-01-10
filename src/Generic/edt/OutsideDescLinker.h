// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef OUTSIDEDESCLINKER_H
#define OUTSIDEDESCLINKER_H

#include "Generic/edt/MentionLinker.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/edt/EntityGuess.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntityType.h"

#include "Generic/CASerif/correctanswers/CorrectDocument.h"

class OutsideDescLinker: public MentionLinker {
public:
	OutsideDescLinker ();
	~OutsideDescLinker();

	virtual void resetForNewSentence();
	virtual void resetForNewDocument(Symbol docName);
	virtual void resetForNewDocument(DocTheory *docTheory) { MentionLinker::resetForNewDocument(docTheory); }
	virtual int linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, 
							 EntityType linkType, LexEntitySet *results[], int max_results);

	static const int MAX_PREDICATES;

	CorrectDocument *currentCorrectDocument;
	int guessCorrectAnswerEntity(EntitySet *currSolution, Mention *currMention, 
								 EntityType linkType, EntityGuess *results[], int max_results); 
	//void correctAnswersLinkMention(EntitySet *currSolution, MentionUID currMentionUID, EntityType linkType); 
	//void printCorrectAnswer(int id);


private:
	EntityGuess* _findOutsideMatch(Mention* ment, LexEntitySet *ents, EntityType linkType);
	EntityGuess* _guessNewEntity(Mention *ment, EntityType linkType);

	std::string _outside_coref_directory;
	class Sexp *_outsideSexp;
	static DebugStream _debug;
	


};

#endif
