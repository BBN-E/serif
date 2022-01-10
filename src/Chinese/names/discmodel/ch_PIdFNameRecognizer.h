// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_PIDF_NAME_RECOGNIZER_H
#define ch_PIDF_NAME_RECOGNIZER_H

#include "Chinese/names/ch_NameRecognizer.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/names/IdFSentenceTokens.h"
#include "Generic/names/discmodel/PIdFModel.h"
#include "Chinese/names/discmodel/ch_PIdFCharModel.h"
#include <wchar.h>

class PIdFNameRecognizer : public NameRecognizer {
public:
	PIdFNameRecognizer();

	void resetForNewSentence(const Sentence *sentence);

	void cleanUpAfterDocument(){}
	void resetForNewDocument(class DocTheory *docTheory){}

	/** 
	  * This does the work. It populates an array of pointers to NameTheorys
	  * specified by <code>results</code> with up to <code>max_theories</code> 
	  * NameTheory pointers. The client is responsible for deleting the NameTheorys.
	  *
	  * @param results the array of pointers to populate
	  * @param max_theories the maximum number of theories to return
	  * @param tokenSequence the TokenSequence representing source text 
	  * @return the number of theories produced or 0 for failure
	  */
	int getNameTheories(NameTheory **results, int max_theories,
						TokenSequence *tokenSequence);

private:

	PIdFModel *_pidfDecoder;
	PIdFCharModel *_pidfCharDecoder;

	bool _run_on_tokens;
	
};

#endif
