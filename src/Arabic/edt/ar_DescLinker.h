// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_DESC_LINKER_H
#define ar_DESC_LINKER_H

#include "edt/DescLinker.h"
#include "theories/EntitySet.h"
#include "theories/Mention.h"
#include "edt/EntityGuess.h"

// Warning: This is an empty version of descLinker!
class ArabicDescLinker : public DescLinker {
private:
	friend class ArabicDescLinkerFactory;

public:
	virtual ~ArabicDescLinker(){};
	int linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, 
		EntityType linkType, LexEntitySet *results[], int max_results);
private:
	ArabicDescLinker(){};
	EntityGuess* _guessNewEntity(Mention *ment, EntityType linkType);

};

class ArabicDescLinkerFactory: public DescLinker::Factory {
	virtual DescLinker *build() { return _new ArabicDescLinker(); }
};

#endif

