// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_PRONOUNLINKER_H
#define ar_PRONOUNLINKER_H

#include "Generic/edt/MentionLinker.h"
#include "Generic/edt/PronounLinker.h"

#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/edt/EntityGuess.h"
#include "Generic/edt/LinkGuess.h"
#include "Generic/edt/HobbsDistance.h"
#include "Generic/common/DebugStream.h"
#include "Generic/theories/PartOfSpeechSequence.h"

class DTPronounLinker;
class SymbolHash;


class ArabicPronounLinker : public PronounLinker {
private:
	friend class ArabicPronounLinkerFactory;

public:
	/**
	 * WARNING: This is an unimplemented Pronoun Linker, no warning will
	 * be printed, but it does not do any work!
	 */
	~ArabicPronounLinker();
	virtual void resetForNewDocument(Symbol docName) { PronounLinker::resetForNewDocument(docName); }
	virtual void resetForNewDocument(DocTheory *docTheory);
	virtual int linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, 
		EntityType linkType, LexEntitySet *results[], int max_results);
	void addPreviousParse(const Parse *parse);
	void resetPreviousParses() ;
	void addPartOfSpeechSequence(const PartOfSpeechSequence* posSeq);
	void resetPartOfSpeechSequence() ;

	
private:
	ArabicPronounLinker();
	EntityGuess* _guessNewEntity(Mention *ment, EntityType linkType);
	int _getClosestHeadMatch(Mention *currMention, LinkGuess* guesses, int max_guesses, const MentionSet* currMentionSet);
	int _getCandidates(const SynNode* pronNode,  GrowableArray <const Parse *> &prevSentences, 
						HobbsDistance::SearchResult results[], int max_results);
	GrowableArray <const Parse *> _previousParses;
	GrowableArray <const PartOfSpeechSequence *> _previousPOSSeq;
	bool _doLink;
	//for pronoun linking choices
	SymbolHash* _mascSing;
	//SymbolHash* _mascPl;
	SymbolHash* _femSing;
	//SymbolHash* _femPl;
	void loadSymbolHash(SymbolHash *hash, const char* file);

	// when true we ignore all pronoun mentions and not add them to the entity set
	bool _discard_pron;


	int MODEL_TYPE;
	enum {PM, DT};

//	PMPronounLinker *_pmPronounLinker;
	DTPronounLinker *_dtPronounLinker;

public:
	bool _PronounAndNounAgree(const SynNode* pron, const SynNode* np);
	bool _isPossesivePronoun(const SynNode* pron);
	bool _isRelativePronoun(const SynNode* pron);
	bool _isRegularPronoun(const SynNode* pron);
	int _getNodeAncestors(const SynNode* node, const SynNode* ancestors[], int childNum[], int max);
	bool _genderNumberClash(const Mention* ment, const SynNode* pronNode, int pron_sent);

	
};

class ArabicPronounLinkerFactory: public PronounLinker::Factory {
	virtual PronounLinker *build() { return _new ArabicPronounLinker(); }
};

#endif
