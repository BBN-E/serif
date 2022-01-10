// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/ParserBase.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/parse/ParseNode.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/NestedNameTheory.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/TokenSequence.h"


void ParserBase::resetForNewSentence() {
	_nodeIDGenerator = IDGenerator(0);
}

SynNode *ParserBase::convertParseToSynNode(ParseNode* parse, 
									   SynNode* parent, 
									   const NameTheory *nameTheory,
									   const NestedNameTheory *nestedNameTheory,
									   const TokenSequence *tokenSequence,
									   int start_token)
{
	if (parse->headNode == 0) {
		SynNode *snode = _new SynNode(_nodeIDGenerator.getID(), parent, 
			parse->label, 0);
		snode->setTokenSpan(start_token, start_token);
		return snode;
	} 
	if (parse->label == ParserTags::LIST) {
		return convertListParseToSynNode(parse, parent, nameTheory, nestedNameTheory, tokenSequence);
	}

	// count children, store last premod node
	int n_children = 0;
	ParseNode* iterator = parse->premods;
	ParseNode* last_premod = parse->premods;
	while (iterator != 0) {
		n_children++;
		last_premod = iterator;
		iterator = iterator->next;
	}
	int head_index = n_children;
	n_children++;
	iterator = parse->postmods;
	while (iterator != 0) {
		n_children++;
		iterator = iterator->next;
	}

	SynNode *snode = _new SynNode(_nodeIDGenerator.getID(), parent, 
		parse->label, n_children);
	snode->setHeadIndex(head_index);

	int token_index = start_token;

	// make life easier
	if (parse->premods != 0) {
		parse->premods_reverse(parse->premods, parse->premods->next);
		parse->premods->next = 0;
		// must assign premods as well, since otherwise this branch won't be deleted
		parse->premods = last_premod;
	}

	// create children
	int child_index = 0;
	iterator = last_premod;
	while (iterator != 0) {
		snode->setChild(child_index, 
			convertParseToSynNode(iterator, snode, nameTheory, nestedNameTheory, tokenSequence, token_index));
		iterator = iterator->next;
		token_index = snode->getChild(child_index)->getEndToken() + 1;			
		child_index++;
	}
	snode->setChild(child_index, 
			convertParseToSynNode(parse->headNode, snode, nameTheory, nestedNameTheory, tokenSequence, token_index));
	token_index = snode->getChild(child_index)->getEndToken() + 1;	
	child_index++;
	iterator = parse->postmods;
	while (iterator != 0) {
		snode->setChild(child_index, 
			convertParseToSynNode(iterator, snode, nameTheory, nestedNameTheory, tokenSequence, token_index));
		iterator = iterator->next;
		token_index = snode->getChild(child_index)->getEndToken() + 1;			
		child_index++;
	}
	
	snode->setTokenSpan(start_token, token_index - 1);
	return snode;

}

SynNode* ParserBase::convertListParseToSynNode(ParseNode* parse, 
											   SynNode* parent, 
											   const NameTheory *nameTheory,
											   const NestedNameTheory *nestedNameTheory,
											   const TokenSequence *tokenSequence)
{
	int list_start = parse->chart_start_index;
	int list_end = parse->chart_end_index;

	Symbol tag = LanguageSpecificFunctions::getNPlabel();
	Symbol headTag = parse->headNode->label;

	return convertFlatParseWithNamesToSynNode(parent, nameTheory, nestedNameTheory, 
												tokenSequence, list_start, list_end,
												tag, headTag);
	
}

SynNode* ParserBase::convertFlatParseWithNamesToSynNode(SynNode* parent, 
														const NameTheory *nameTheory,
														const NestedNameTheory *nestedNameTheory,
														const TokenSequence *tokenSequence,
														int start_token, int end_token,
														Symbol tag, Symbol headTag) 
{
	int nkids = 0;
	for (int j = 0; j < nameTheory->getNNameSpans(); j++) {
		NameSpan *span = nameTheory->getNameSpan(j);
		if (span->start >= start_token &&
			span->end <= end_token)
		{
			nkids++;	
		}
	}
	for (int k = start_token; k <= end_token; k++) {
		bool found_in_name = false;
		for (int j = 0; j < nameTheory->getNNameSpans(); j++) {
			NameSpan *span = nameTheory->getNameSpan(j);
			if (k >= span->start && k <= span->end)
			{
				found_in_name = true;	
			}
		}
		if (!found_in_name)
			nkids++;
	}
	SynNode *snode = _new SynNode(_nodeIDGenerator.getID(), parent, 
		tag, nkids);
	snode->setHeadIndex(nkids - 1);
	snode->setTokenSpan(start_token, end_token);
	nkids = 0;
	int token_iter = start_token;
	for (int m = 0; m < nameTheory->getNNameSpans(); m++) {
		NameSpan *span = nameTheory->getNameSpan(m);
		if (span->start < start_token || span->end > end_token)
			continue;

		while (token_iter < span->start) {
			Symbol word = LanguageSpecificFunctions::getSymbolForParseNodeLeaf(tokenSequence->getToken(token_iter)->getSymbol());
			SynNode *tagNode = _new SynNode(_nodeIDGenerator.getID(), snode, 
				LanguageSpecificFunctions::getParseTagForWord(word), 1);
			tagNode->setHeadIndex(0);
			tagNode->setTokenSpan(token_iter, token_iter);
			snode->setChild(nkids++, tagNode);			
			SynNode *wordNode = _new SynNode(_nodeIDGenerator.getID(), tagNode, 
				word, 0);
			wordNode->setTokenSpan(token_iter, token_iter);
			tagNode->setChild(0, wordNode);
			token_iter++;
		}			

		SynNode * childNode;
		if (nestedNameTheory != 0 && nestedNameTheory->getNNameSpans() > 0) {
			childNode = convertFlatParseWithNamesToSynNode(snode, (NameTheory*)nestedNameTheory, 0, 
															tokenSequence, span->start, span->end,
															LanguageSpecificFunctions::getNameLabel(), headTag);
		} else {
			childNode = _new SynNode(_nodeIDGenerator.getID(), snode, 
									LanguageSpecificFunctions::getNameLabel(), span->end - span->start + 1);
			childNode->setHeadIndex(span->end - span->start);
			childNode->setTokenSpan(span->start, span->end);		

			for (int word = span->start; word <= span->end; word++) {
				SynNode *tagNode = _new SynNode(_nodeIDGenerator.getID(), childNode, 
					headTag, 1);
				tagNode->setHeadIndex(0);
				tagNode->setTokenSpan(word, word);
				childNode->setChild(word - span->start, tagNode);
				SynNode *wordNode = _new SynNode(_nodeIDGenerator.getID(), tagNode, 
					LanguageSpecificFunctions::getSymbolForParseNodeLeaf(tokenSequence->getToken(word)->getSymbol()),
					0);
				wordNode->setTokenSpan(word, word);
				tagNode->setChild(0, wordNode);
			}
		}	
		snode->setChild(nkids++, childNode);
	
		token_iter = span->end + 1;
	}

	while (token_iter <= end_token) {
		Symbol word = LanguageSpecificFunctions::getSymbolForParseNodeLeaf(tokenSequence->getToken(token_iter)->getSymbol());
		SynNode *tagNode = _new SynNode(_nodeIDGenerator.getID(), snode, 
				LanguageSpecificFunctions::getParseTagForWord(word), 1);
		tagNode->setHeadIndex(0);
		tagNode->setTokenSpan(token_iter, token_iter);
		snode->setChild(nkids++, tagNode);			
		SynNode *wordNode = _new SynNode(_nodeIDGenerator.getID(), tagNode, 
				word, 0);
		wordNode->setTokenSpan(token_iter, token_iter);
		tagNode->setChild(0, wordNode);
		token_iter++;
	}		
	return snode;
}
