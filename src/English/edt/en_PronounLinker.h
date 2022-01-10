// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_PRONOUN_LINKER_H
#define en_PRONOUN_LINKER_H

#include "Generic/edt/PronounLinker.h"
#include "Generic/edt/LinkGuess.h"
#include "Generic/theories/EntityType.h"
#include "Generic/common/DebugStream.h"


class Parse;
class LexEntitySet;
class EntitySet;
class Mention;
class DTPronounLinker;
class EnglishPMPronounLinker;

class EnglishPronounLinker : public PronounLinker {
private:
	friend class EnglishPronounLinkerFactory;

public:
	~EnglishPronounLinker();

	virtual void addPreviousParse(const Parse *parse);
	virtual int linkMention (LexEntitySet * currSolution, 
							 MentionUID currMentionUID, 
							 EntityType linkType, 
							 LexEntitySet *results[], 
							 int max_results);

	virtual void resetPreviousParses();
	virtual void resetForNewDocument(Symbol docName) { PronounLinker::resetForNewDocument(docName); }
	virtual void resetForNewDocument(DocTheory *docTheory);

private:
	EnglishPronounLinker();
	int MODEL_TYPE;
	enum {PM, DT};

	EnglishPMPronounLinker *_pmPronounLinker;
	DTPronounLinker *_dtPronounLinker;
};

class EnglishPronounLinkerFactory: public PronounLinker::Factory {
	virtual PronounLinker *build() { return _new EnglishPronounLinker(); }
};


#endif
