// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DISCOURSE_RELATION_OBSERVATION_H
#define DISCOURSE_RELATION_OBSERVATION_H

//#include <iostream>
//#include <stdio.h>

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/discTagger/DTObservation.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"

#include "Generic/common/SymbolUtilities.h"

#include "Generic/discourseRel/StopWordFilter.h"
#include <vector>

#define ETO_MAX_WN_OFFSETS 20
#define ETO_MAX_OTHER_ARGS 3
#define MAXENTITYINSENT2 10

class DiscourseRelObservation : public DTObservation {
public:
	
	DiscourseRelObservation() : DTObservation(_className),
		_wordCluster(WordClusterClass::nullCluster()) {}
	
	//DiscourseRelObservation() : DTObservation(_className) {}

	~DiscourseRelObservation() {_root = 0; _commonParentforLeftNeighbor=0;
	_commonParentforRightNeighbor=0;}

	virtual DTObservation *makeCopy();
	void populate();
	void populate(DiscourseRelObservation *other);
	void populate(int token_index, const TokenSequence *tokens, 
									   const Parse *parse, bool use_wordnet);
	void populateWithWords(int sentIndex, TokenSequence** _tokens, const Parse** _parses);
	void populateWithNonStopWords(int sentIndex, TokenSequence** _tokens, const Parse** _parses);
	void populateWithWordNet(int sentIndex, TokenSequence** _tokens, const Parse** _parses);
	void populateWithWordNetUsingStopWordLs(int sentIndex, TokenSequence** _tokens, const Parse** _parses);
	//void populateWithProposition(int sentIndex, TokenSequence** _tokens, const Parse** _parses, 
	//	MentionSet** _mentions, const PropositionSet** _propSet, bool use_wordnet);
	void populateWithRichFeatures(int sentIndex, TokenSequence** _tokens, EntitySet** _entitySets, MentionSet** _mentionSets, const Parse** _parses, const PropositionSet** _propSets, UTF8OutputStream& out);
	
	const SynNode* getRootOfContextTree () {return _root;}

	Symbol getPOS();
	Symbol getStemmedWord();
	Symbol getWord();
	Symbol getLCWord();
	bool isLastWord();
	bool isFirstWord();
	Symbol getNextWord();
	Symbol getSecondNextWord();
	Symbol getPrevWord();
	Symbol getSecondPrevWord();
	
	Symbol getParentPOS();
	Symbol getLeftSiblingHead();
	Symbol getLeftSiblingPOS();
	Symbol getRightSiblingHead();
	Symbol getRightSiblingPOS();
	Symbol getLeftNeighborHead();
	Symbol getLeftNeighborPOS();
	Symbol getRightNeighborHead();
	Symbol getRightNeighborPOS();
	Symbol getCommonParentPOSforLeftNeighbor();
	Symbol getCommonParentPOSforRightNeighbor();
	const SynNode *getCommonParentforLeftNeighbor();
	const SynNode *getCommonParentforRightNeighbor();
	wstring getSentPair();
	wstring getCurrentSent();
	wstring getNextSent();
	vector<Symbol> getWordPairs();
	vector<int> getSynsetIdsInCurrentSent();
	vector<int> getSynsetIdsInNextSent();
	int getNOffsets();
	int getNthOffset(int n);
	//WordClusterClass getWordCluster();

	bool sent2IsLedbyConn();
	Symbol getConnLeadingSent2();
	bool shareMentions();
	bool shareNPargMentions();
	bool share2NPargMentions();
	int getNSharedargMentions();

private:
	//UTF8OutputStream _featDebugStream;

	static const Symbol _className;
	const SynNode *findCommonParent (const SynNode *node1, const SynNode *node2);
	Symbol _word;
	Symbol _nextWord;
	Symbol _secondNextWord;
	Symbol _prevWord;
	Symbol _secondPrevWord;
	Symbol _lcWord;
	Symbol _stemmedWord;
	Symbol _pos;
	
	Symbol _pPos;
	Symbol _leftSiblingHead;
	Symbol _leftSiblingPOS;
	Symbol _rightSiblingHead;
	Symbol _rightSiblingPOS;
	
	Symbol _leftNeighborHead;
	Symbol _leftNeighborPOS;
	Symbol _rightNeighborHead;
	Symbol _rightNeighborPOS;
	Symbol _commonParentPOSforLeftNeighbor;
	Symbol _commonParentPOSforRightNeighbor;

	const SynNode *_commonParentforLeftNeighbor;
	const SynNode *_commonParentforRightNeighbor;
	// context information
	const SynNode *_root;

	// features for cross-sent discourse relation 
	WordClusterClass _wordCluster;

	vector<Symbol> _bagsOfWordPairs;
	vector<int> _synsetIdsInCurrentSent;
	vector<int> _synsetIdsInNextSent;
	wstring _currentSent;
	wstring _nextSent;

	bool _sent2IsLedbyConn;
	Symbol _connLeadingSent2;
	bool _shareMentions;
	int _shareNPargMentions;
		
};

#endif
