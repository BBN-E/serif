// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CHART_ENTRY_H
#define CHART_ENTRY_H

#include <cstddef>
#include "Generic/common/Symbol.h"
#include "Generic/parse/BridgeType.h"
#include "Generic/parse/BridgeKernel.h"
#include "Generic/parse/BridgeExtension.h"
#include "Generic/parse/SequentialBigrams.h"
#include "Generic/parse/SignificantConstitNode.h"
#include "Generic/parse/ParseNode.h"

#define CHART_ENTRY_MAX_CHAIN 10
#define CHART_ENTRY_BLOCK_SIZE 10000

class ChartEntry {
private:
	static const int maxChain;
	static const size_t blockSize;
	static SequentialBigrams * sequentialBigrams;
	
public:
	Symbol constituentCategory;
	Symbol headConstituent;
	Symbol headWord;
	Symbol headTag;
	Symbol leftEdge;
	Symbol rightEdge;
	
	// added to be able to do lexical bigrams
	Symbol leftWord;
	Symbol rightWord;
	Symbol leftTag;
	Symbol rightTag;
	
	// added for diversity parsing
	SignificantConstitNode *significantConstitNode;
	int leftToken;
	int rightToken;
	Symbol nameType;
	bool isPPofSignificantConstit;
	bool headIsSignificant;
	
	bool isPreterminal;
	int bridgeType;
	ChartEntry* leftChild;
	ChartEntry* rightChild;
	union {
		BridgeKernel* kernelOp;
		BridgeExtension* extensionOp;
	};
	float insideScore;
	float rankingScore;
	float leftCapScore;
	float rightCapScore;

	bool operator==(const ChartEntry& entry2) const {
		if ((headConstituent != entry2.headConstituent) ||
			(rightEdge != entry2.rightEdge) ||
			(constituentCategory != entry2.constituentCategory) ||
			(headWord != entry2.headWord) ||
			(headTag != entry2.headTag) ||
			(leftEdge != entry2.leftEdge))
			return false;
		else if (sequentialBigrams->use_left_sequential_bigrams(constituentCategory)) {
			if (leftWord != entry2.leftWord ||
				leftTag != entry2.leftTag)
				return false;
		} else if (sequentialBigrams->use_right_sequential_bigrams(constituentCategory)) {
			if (rightWord != entry2.rightWord ||
				rightTag != entry2.rightTag)
				return false;
		} 
		
		// added for proposition parsing
		if (!(*significantConstitNode == *(entry2.significantConstitNode)))
			return false;
		
		return true;
	}
	static void set_sequentialBigrams(SequentialBigrams *sb) {
		sequentialBigrams = sb;
	}
	ParseNode* toParseNode();
	ParseNode* addChain(ParseNode* base, const Symbol &chainSym);
	
	ChartEntry() {}
	ChartEntry(const Symbol &constituentCategory, const Symbol &headConstituent, 
		const Symbol &headWord, bool headIsSignificant, const Symbol &headTag, 
		const Symbol &leftEdge, const Symbol &leftTag, const Symbol &leftWord, 
		const Symbol &rightEdge, const Symbol &rightTag, const Symbol &rightWord, 
		ChartEntry* leftChild, ChartEntry* rightChild, int leftToken, int rightToken, 
		const Symbol &nameType, SignificantConstitNode *significantConstitNode, 
		bool isPPofSignificantConstit, BridgeKernel* kernelOp, bool isPreterminal):
		constituentCategory(constituentCategory), headConstituent(headConstituent), 
		headWord(headWord), headIsSignificant(headIsSignificant), headTag(headTag),
		leftEdge(leftEdge), leftTag(leftTag), leftWord(leftWord), 
		rightEdge(rightEdge), rightTag(rightTag), rightWord(rightWord), 
		leftChild(leftChild), rightChild(rightChild), leftToken(leftToken), rightToken(rightToken), 
		nameType(nameType), significantConstitNode(significantConstitNode),
		isPPofSignificantConstit(isPPofSignificantConstit),
		bridgeType(BRIDGE_TYPE_KERNEL), kernelOp(kernelOp), isPreterminal(isPreterminal),
		insideScore(0), rankingScore(0), leftCapScore(0), rightCapScore(0) {
		}


	ChartEntry(const Symbol &constituentCategory, const Symbol &headConstituent, 
		const Symbol &headWord, bool headIsSignificant, const Symbol &headTag, 
		const Symbol &leftEdge, const Symbol &leftTag, const Symbol &leftWord, 
		const Symbol &rightEdge, const Symbol &rightTag, const Symbol &rightWord, 
		ChartEntry* leftChild, ChartEntry* rightChild, int leftToken, int rightToken, 
		const Symbol &nameType, SignificantConstitNode *significantConstitNode, 
		bool isPPofSignificantConstit, BridgeExtension* extensionOp, bool isPreterminal):
		constituentCategory(constituentCategory), headConstituent(headConstituent), 
		headWord(headWord), headIsSignificant(headIsSignificant), headTag(headTag),
		leftEdge(leftEdge), leftTag(leftTag), leftWord(leftWord), 
		rightEdge(rightEdge), rightTag(rightTag), rightWord(rightWord), 
		leftChild(leftChild), rightChild(rightChild), leftToken(leftToken), rightToken(rightToken), 
		nameType(nameType), significantConstitNode(significantConstitNode),
		isPPofSignificantConstit(isPPofSignificantConstit),
		bridgeType(BRIDGE_TYPE_EXTENSION), extensionOp(extensionOp), isPreterminal(isPreterminal),
		insideScore(0), rankingScore(0), leftCapScore(0), rightCapScore(0) {}

	~ChartEntry()
	{
		delete significantConstitNode;
	}
	
	static void* operator new(size_t n, int, char *, int) { return operator new(n); }
	static void* operator new(size_t);
	static void operator delete(void* object);
private:
	//static ParseNode* name_premods;
	static ChartEntry* freeList;
	ChartEntry* next;
};

#endif

