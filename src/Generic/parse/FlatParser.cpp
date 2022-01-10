// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/ParseNode.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/NestedNameTheory.h"
#include "Generic/parse/Constraint.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntityType.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/parse/FlatParser.h"


FlatParser::FlatParser(): _debug(Symbol(L"parse_debug")) {
}

FlatParser::~FlatParser() {
}

void FlatParser::resetForNewDocument(DocTheory *docTheory) {
}

int FlatParser::getParses(Parse **results, int max_num_parses,
					  const TokenSequence *tokenSequence,
					  const PartOfSpeechSequence *partOfSpeech,
					  const NameTheory *nameTheory,
					  const NestedNameTheory *nestedNameTheory,
					  const ValueMentionSet *valueMentionSet)
{
	return getParses(results, max_num_parses, tokenSequence, partOfSpeech,
		nameTheory, nestedNameTheory, valueMentionSet, 0, 0);
}

int FlatParser::getParses(Parse **results, int max_num_parses,
					  const TokenSequence *tokenSequence,
					  const PartOfSpeechSequence *partOfSpeech,
					  const NameTheory *nameTheory,
					  const NestedNameTheory *nestedNameTheory,
					  const ValueMentionSet *valueMentionSet,
					  Constraint *constraints,
					  int n_constraints)
{
	if (tokenSequence->getNTokens() == 0) {
		results[0] = _new Parse(tokenSequence);
		return 1;
	}

	// set sentence tokens
	ParseNode* parseNode = getDefaultFlatParse(tokenSequence, partOfSpeech);
	if (parseNode == 0) {
		// looks like we had to bail out
		return 0;
	}
	SynNode *synNode = convertParseToSynNode(parseNode, 0, nameTheory, nestedNameTheory, tokenSequence, 0);									
	results[0] = _new Parse(tokenSequence, synNode, 1);
	delete parseNode;

	return 1;
}

ParseNode *FlatParser::getDefaultFlatParse(const TokenSequence *tokenSequence,
										   const PartOfSpeechSequence *posSequence) 
{
	ParseNode *tree = _new ParseNode(ParserTags::FRAGMENTS);

	if (tokenSequence->getNTokens() < 1) {
		throw InternalInconsistencyException("FlatParser::getDefaultFlatParse()", 
											 "No tokens found in TokenSequence.");
	}
	if (posSequence->getNTokens() < 1 || posSequence->getPOS(0)->getNPOS() < 1) {
		throw InternalInconsistencyException("FlatParser::getDefaultFlatParse()", 
											 "No parts of speech found in PartOfSpeechSequence.");
	}
	tree->headNode = _new ParseNode(posSequence->getPOS(0)->getLabel(0));
	tree->headNode->headNode = _new ParseNode(LanguageSpecificFunctions::getSymbolForParseNodeLeaf(tokenSequence->getToken(0)->getSymbol()));
	if (tokenSequence->getNTokens() > 1) {
		tree->postmods = _new ParseNode(posSequence->getPOS(1)->getLabel(0));
		tree->postmods->headNode = _new ParseNode(LanguageSpecificFunctions::getSymbolForParseNodeLeaf(tokenSequence->getToken(1)->getSymbol()));
		ParseNode *placeholder = tree->postmods;
		for (int i = 2; i < tokenSequence->getNTokens(); i++) {
			placeholder->next = _new ParseNode(posSequence->getPOS(i)->getLabel(0));
			placeholder->next->headNode = _new ParseNode(LanguageSpecificFunctions::getSymbolForParseNodeLeaf(tokenSequence->getToken(i)->getSymbol()));
			placeholder = placeholder->next;
		}
	}
	return tree;
}
