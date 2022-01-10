// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_PRONOUN_LINKER_H
#define ch_PRONOUN_LINKER_H

#include "Generic/edt/PronounLinker.h"
#include "Generic/edt/LinkGuess.h"
#include "Generic/theories/EntityType.h"
#include "Generic/common/DebugStream.h"


class Parse;
class LexEntitySet;
class EntitySet;
class Mention;
class DTPronounLinker;
class ChinesePMPronounLinker;

class ChinesePronounLinker : public PronounLinker {
private:
	friend class ChinesePronounLinkerFactory;

public:
	~ChinesePronounLinker();

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
	ChinesePronounLinker();
	int MODEL_TYPE;
	enum {PM, DT};

	ChinesePMPronounLinker *_pmPronounLinker;
	DTPronounLinker *_dtPronounLinker;
};

class ChinesePronounLinkerFactory: public PronounLinker::Factory {
	virtual PronounLinker *build() { return _new ChinesePronounLinker(); }
};


#endif
