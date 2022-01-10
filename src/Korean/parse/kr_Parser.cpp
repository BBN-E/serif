// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "parse/KoreanParser.h"

#include "parse/ParseNode.h"
#include "theories/Parse.h"
#include "theories/SynNode.h"
#include "theories/PartOfSpeechSequence.h"
#include "parse/ParserTags.h"
#include "theories/TokenSequence.h"
#include "theories/Token.h"
#include "theories/NameTheory.h"
#include "parse/Constraint.h"
#include "parse/ChartDecoder.h"
#include "parse/LanguageSpecificFunctions.h"
#include "parse/DefaultParser.h"
#include "common/ParamReader.h"
#include "common/UnexpectedInputException.h"
#include "Korean/morphology/kr_Retokenizer.h"


KoreanParser::KoreanParser() : _standardParser(0), _debug(Symbol(L"parse_debug")) {
	_max_sentence_length = 100;
	_max_constraints = 50;
	_sentence = _new Symbol[_max_sentence_length];
	_constraints = _new Constraint[_max_constraints];

	_ps = _new KoreanParseSeeder();

	if (ParamReader::isParamTrue("use_standard_parser")) 
		_standardParser = _new DefaultParser();
}

KoreanParser::~KoreanParser() {
	delete _standardParser;
	delete _ps;
	delete [] _constraints;
	delete [] _sentence;
}

void KoreanParser::cleanup() {
	_standardParser->cleanup();
}

void KoreanParser::resetForNewSentence() {
	_nodeIDGenerator = IDGenerator(0);

	if (_standardParser != 0)
		_standardParser->resetForNewSentence();
}

void KoreanParser::resetForNewDocument(DocTheory *docTheory) {
	if (_standardParser != 0)
		_standardParser->resetForNewDocument(docTheory);
}

int KoreanParser::getParses(Parse **results, int max_num_parses,
					  TokenSequence *tokenSequence,
					  PartOfSpeechSequence *partOfSpeech,
					  NameTheory *nameTheory,
					  ValueMentionSet *valueMentionSet)
{
	if (_standardParser != 0) {
		return _standardParser->getParses(results, max_num_parses, tokenSequence,
			partOfSpeech, nameTheory, valueMentionSet);
	}
	
	/*int numConstraints = nameTheory->n_name_spans;
	int sentenceLength = tokenSequence->getNTokens();

	if (numConstraints > _max_constraints) {
		_max_constraints = numConstraints + 10;
		_constraints = _new Constraint[_max_constraints];
	}

	int num_seg = _putTokenSequenceInLookupChart(tokenSequence, nameTheory);
	*/

	ParseNode* parseNode = getDefaultFlatParse(tokenSequence, partOfSpeech);

	if (parseNode == 0) {
		// looks like we had to bail out
		return 0;
	}

	SynNode *synNode = convertParseToSynNode(parseNode, 0, nameTheory, tokenSequence, 0);									
	results[0] = _new Parse(synNode, 1);
	
	return 1;
}

int KoreanParser::getParses(Parse **results, int max_num_parses,
					  TokenSequence *tokenSequence,
					  PartOfSpeechSequence *partOfSpeech,
					  NameTheory *nameTheory,
					  ValueMentionSet *valueMentionSet,
					  Constraint *constraints,
					  int n_constraints)

{
	if (_standardParser != 0) {
		return _standardParser->getParses(results, max_num_parses, tokenSequence, 
			partOfSpeech, nameTheory,
			valueMentionSet, constraints, n_constraints);
	}
	//int num_seg = _putTokenSequenceInLookupChart(tokenSequence, nameTheory);

	ParseNode* parseNode = getDefaultFlatParse(tokenSequence);
	if (parseNode == 0) {
		// looks like we had to bail out
		return 0;
	}
	SynNode *synNode = convertParseToSynNode(parseNode, 0, nameTheory, tokenSequence, 0);									
	results[0] = _new Parse(synNode, 1);

	return 1;
}

void KoreanParser::setMaxParserSeconds(int maxsecs) {
	if (_standardParser != 0)
		_standardParser->getDecoder()->setMaxParserSeconds(maxsecs);
}

/*
int KoreanParser::_putTokenSequenceInLookupChart(const TokenSequence* ts, const NameTheory* nameTheory){
	int chart_loc = 0;
	int max_entry = 0;
	int min_entry = 5000;
	int curr_nt = 0;
	int n_nt = nameTheory->n_name_spans;
	int tok_map[1300][2];
	int count = 0;
	for (int j = 0; j < MAX_SENTENCE_TOKENS; j++) {
		_lookupCount[j] = 0;
	}
	for (int i = 0; i < ts->getNTokens(); i++) {
		if (DBG == 1) std::cout << "Process Token Number: " << i << " Token " << ts->getToken(i)->getSymbol().to_debug_string() << std::endl;
		if ((curr_nt < n_nt) && (nameTheory->nameSpans[curr_nt]->start == i)) {
			int nw = i;
			_constraints[curr_nt].left = chart_loc;
			_constraints[curr_nt].type = LanguageSpecificFunctions::getNameLabel();
			while (nw <= nameTheory->nameSpans[curr_nt]->end) {
				_lookupChart[chart_loc][0].nvString = ts->getToken(nw)->getSymbol();
				_lookupChart[chart_loc][0].start = chart_loc;
				_lookupChart[chart_loc][0].end = chart_loc;
				_lookupChart[chart_loc][0].num_ent = 0;
				_lookupChart[chart_loc][0].pt[0].start_offset = ts->getToken(nw)->getStartOffset();
				_lookupChart[chart_loc][0].pt[0].end_offset = ts->getToken(nw)->getEndOffset();
				//need to add lexical entries for later processing!
				_lookupCount[chart_loc]++;
				_parserSeedingChart[count] = &_lookupChart[chart_loc][0];
				if (DBG == 1) {
					std::cout << "\tName Entry " << count << " - Token: " << i << " String: "
					<< _lookupChart[chart_loc][0].nvString.to_debug_string()
					<< " Start: " << _lookupChart[chart_loc][0].start 
					<< " End: " << _lookupChart[chart_loc][0].end << std::endl;
				}
				_constraints[curr_nt].right  = chart_loc;
				count++;
				chart_loc++;
				nw++;
				i++;
			}
			i--;
			curr_nt++;
		}
		else {
			_ps->processToken(ts->getToken(i));
			for (int j = 0; j < _ps->_num_grouped; j++) {
				//if there is only one entry must need only one chart loc
				//this isn't necessary, but it reduces the size of the chart
				int seg_start, seg_end;
				if (_ps->_num_grouped == 1) {
					seg_start = chart_loc;
					seg_end = chart_loc;
				}
				else {
					seg_start = _ps->_groupedWordChart[j].start + chart_loc;
					seg_end = _ps->_groupedWordChart[j].end +chart_loc;
				}
				if (min_entry < seg_start)
					min_entry = seg_start;
				if (seg_end > max_entry)
					max_entry = seg_end;

				_lookupChart[seg_start][_lookupCount[seg_start]] = _ps->_groupedWordChart[j];
				_lookupChart[seg_start][_lookupCount[seg_start]].start = seg_start;
				_lookupChart[seg_start][_lookupCount[seg_start]].end = seg_end;
				_parserSeedingChart[count] = &_lookupChart[seg_start][_lookupCount[seg_start]];
				if (DBG == 1) {
					std::cout << "\tReg  Entry " << count << " - Token: " << i << " String: "
					<< _parserSeedingChart[count]->nvString.to_debug_string()
					<< " Start: " << _parserSeedingChart[count]->start << " End: "
					<< _parserSeedingChart[count]->end
					<< " Breakdown - chart_loc: " << chart_loc
					<< " SegStart: " << _ps->_groupedWordChart[j].start
					<< " SegEnd: " << _ps->_groupedWordChart[j].end << std::endl;
				}
				count++;
				_lookupCount[seg_start]++;

			}
			tok_map[i][0] = min_entry;
			tok_map[i][1] = max_entry;
			chart_loc = max_entry + 1;
		}
	}	
	return count;
}
*/

ParseNode *KoreanParser::getDefaultFlatParse(const TokenSequence *tokenSequence,
									   const PartOfSpeechSequence *posSequence) 
{
	ParseNode *tree = _new ParseNode(ParserTags::FRAGMENTS);
	tree->headNode = _new ParseNode(posSequence->getPOS(0)->getLabel(0));
	tree->headNode->headNode = _new ParseNode(tokenSequence->getToken(0)->getSymbol());
	if (tokenSequence->getNTokens() > 1) {
		tree->postmods = _new ParseNode(posSequence->getPOS(1)->getLabel(0));
		tree->postmods->headNode = _new ParseNode(tokenSequence->getToken(1)->getSymbol());
		ParseNode *placeholder = tree->postmods;
		for (int i = 2; i < tokenSequence->getNTokens(); i++) {
			placeholder->next = _new ParseNode(posSequence->getPOS(i)->getLabel(0));
			placeholder->next->headNode = _new ParseNode(tokenSequence->getToken(i)->getSymbol());
			placeholder = placeholder->next;
		}
	}
	return tree;
}
ParseNode *KoreanParser::getDefaultFlatParse(const TokenSequence *tokenSequence) {
	ParseNode *tree = _new ParseNode(ParserTags::FRAGMENTS);
	ParseNode *placeholder = 0;
	int curr_tok = 0;

	_ps->processToken(tokenSequence->getToken(curr_tok));
	ParseSeeder::ParseToken pt = _ps->_mergedWordChart[0][0];

	tree->headNode = _new ParseNode(pt.pos);
	tree->headNode->headNode = _new ParseNode(pt.normString);
	if (_ps->_mergedNumSeg[0] > 1) {
		pt = _ps->_mergedWordChart[1][0];
		tree->postmods = _new ParseNode(pt.pos);
		tree->postmods->headNode = _new ParseNode(pt.normString);
		placeholder = tree->postmods;
		for (int i = 2; i < _ps->_mergedNumSeg[0]; i++) {
			pt = _ps->_mergedWordChart[i][0];
			placeholder->next = _new ParseNode(pt.pos);
			placeholder->next->headNode = _new ParseNode(pt.normString);
			placeholder = placeholder->next;
		}
	}
	else if (tokenSequence->getNTokens() > 1) {
		curr_tok++;
		_ps->processToken(tokenSequence->getToken(curr_tok));
		ParseSeeder::ParseToken pt = _ps->_mergedWordChart[0][0];
		tree->postmods = _new ParseNode(pt.pos);
		tree->postmods->headNode = _new ParseNode(pt.normString);
		placeholder = tree->postmods;
		for (int i = 1; i < _ps->_mergedNumSeg[0]; i++) {
			pt = _ps->_mergedWordChart[i][0];
			placeholder->next = _new ParseNode(pt.pos);
			placeholder->next->headNode = _new ParseNode(pt.normString);
			placeholder = placeholder->next;
		}
	}
	curr_tok++;
	for (int j = curr_tok; j < tokenSequence->getNTokens(); j++) {
		_ps->processToken(tokenSequence->getToken(j));
		ParseSeeder::ParseToken pt = _ps->_mergedWordChart[0][0];
		for (int i = 0; i < _ps->_mergedNumSeg[0]; i++) {
			pt = _ps->_mergedWordChart[i][0];
			placeholder->next = _new ParseNode(pt.pos);
			placeholder->next->headNode = _new ParseNode(pt.normString);
			placeholder = placeholder->next;
		}
	}
	return tree;
}

SynNode *KoreanParser::convertParseToSynNode(ParseNode* parse, 
									   SynNode* parent, 
									   const NameTheory *nameTheory,									   
									   const TokenSequence *tokenSequence,
									   int start_token)
{
	if (parse->headNode == 0) {
		SynNode *snode = _new SynNode(_nodeIDGenerator.getID(), parent, 
			parse->label, 0);
		snode->setTokenSpan(start_token, start_token);
		return snode;
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

	if (parse->label == ParserTags::LIST) {
		int list_start = start_token;
		int list_end = start_token + n_children - 1;
		
		int nkids = 0;
		for (int j = 0; j < nameTheory->n_name_spans; j++) {
			NameSpan *span = nameTheory->nameSpans[j];
			if (span->start >= list_start &&
				span->end <= list_end)
			{
				nkids++;	
			}
		}
		for (int k = list_start; k <= list_end; k++) {
			bool found_in_name = false;
			for (int j = 0; j < nameTheory->n_name_spans; j++) {
				NameSpan *span = nameTheory->nameSpans[j];
				if (k >= span->start && k <= span->end)
				{
					found_in_name = true;	
				}
			}
			if (!found_in_name)
				nkids++;
		}
		SynNode *snode = _new SynNode(_nodeIDGenerator.getID(), parent, 
			LanguageSpecificFunctions::getNPlabel(), nkids);
		snode->setHeadIndex(nkids - 1);
		snode->setTokenSpan(list_start, list_end);
		nkids = 0;
		int token_iter = list_start;
		for (int m = 0; m < nameTheory->n_name_spans; m++) {
			NameSpan *span = nameTheory->nameSpans[m];
			if (span->start < list_start ||	span->end > list_end)
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

			SynNode *childNode = _new SynNode(_nodeIDGenerator.getID(), snode, 
				LanguageSpecificFunctions::getNameLabel(), span->end - span->start + 1);
			childNode->setHeadIndex(span->end - span->start);
			childNode->setTokenSpan(span->start, span->end);
			snode->setChild(nkids++, childNode);
			for (int word = span->start; word <= span->end; word++) {
				SynNode *tagNode = _new SynNode(_nodeIDGenerator.getID(), childNode, 
					parse->headNode->label, 1);
				tagNode->setHeadIndex(0);
				tagNode->setTokenSpan(word, word);
				childNode->setChild(word - span->start, tagNode);
				SynNode *wordNode = _new SynNode(_nodeIDGenerator.getID(), tagNode, 
					LanguageSpecificFunctions::getSymbolForParseNodeLeaf(tokenSequence->getToken(word)->getSymbol()),
					0);
				wordNode->setTokenSpan(word, word);
				tagNode->setChild(0, wordNode);
			}

			token_iter = span->end + 1;
		}
		return snode;
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
			convertParseToSynNode(iterator, snode, nameTheory, tokenSequence, token_index));
		iterator = iterator->next;
		token_index = snode->getChild(child_index)->getEndToken() + 1;			
		child_index++;
	}
	snode->setChild(child_index, 
			convertParseToSynNode(parse->headNode, snode, nameTheory, tokenSequence, token_index));
	token_index = snode->getChild(child_index)->getEndToken() + 1;	
	child_index++;
	iterator = parse->postmods;
	while (iterator != 0) {
		snode->setChild(child_index, 
			convertParseToSynNode(iterator, snode, nameTheory, tokenSequence, token_index));
		iterator = iterator->next;
		token_index = snode->getChild(child_index)->getEndToken() + 1;			
		child_index++;
	}
	
	snode->setTokenSpan(start_token, token_index - 1);
	return snode;

}


/*
SynNode *KoreanParser::convertParseToSynNode(ParseNode* parse,
									   SynNode* parent,
									   int *old_token,
									   const TokenSequence *tokenSequence,
									   int *new_token,
									   Token **newTokens,
									   int *offset,
									   int *token_mapping, int* start_place)
{
	if (parse->headNode == 0) {
		if (*new_token >= MAX_SENTENCE_TOKENS) {
			throw InternalInconsistencyException(
				"KoreanParser::convertParseToSynNode()",
				"Sam forgot to make sure that the parser can't return parses with\n"
				"too many tokens.");
		}

		SynNode *snode = _new SynNode(_nodeIDGenerator.getID(), parent,
			parse->label, 0);
		//find the correct token in the lookup place
		int st = (*start_place);
		Token* token = NULL;
		for(int i =0; i< _lookupCount[st]; i++){
			if((_lookupChart[st][i].nvString == parse->label) &&
				(_lookupChart[st][i].end ==parse->chart_end_index))
			{
				int start_offset = _lookupChart[st][i].pt[0].start_offset;
				int end_offset = _lookupChart[st][i].pt[0].end_offset;
				const Token *oldToken = tokenSequence->getToken(*old_token);
				int orig_token = oldToken->getOriginalTokenIndex();

				int n_lex =0;
				
				//need to add lexical entry info here
//				LexicalEntry* lexArray[20];
//				LexcialEntry* results[20];
//				for(int k=0; k<_lookupChart[st][i].num_ent;k++){
//					if(_lookupChart[st][i].pt[k].num_lex_ent==1){
//						lexArray[n_lex]= _lookupChart[st][i].pt[k].lex_ents[0];
//					}
//					else if(_lex->hasKey(parse->label)){
//						int n = _lex->getEntriesByKey(parse->label, results);
//						for(int m =0; m<n; m++){
//							if(results[m]-> ==
//					}
//					else{
//						LexicalEntry* le;
//
//						le = new LexicalEntry(_lex->getNextID();
//						_lex->addEntry(le);

				token = _new Token(start_offset, end_offset, orig_token, parse->label, 0, NULL);
				snode->setTokenSpan(*new_token, *new_token);

				newTokens[*new_token] = token;
				token_mapping[*old_token] = *new_token;

				(*new_token)++;


				if (end_offset >= oldToken->getEndOffset()) {
					(*old_token)++;
				}
				(*start_place) = (_lookupChart[st][i].end+1);
				break;
			}

		}

		if(token ==NULL){
			char buffer[1000];
			sprintf(buffer,
				"couldn't match new parse node to old subtoken: parse label: %s, start place: %d",
				parse->label.to_debug_string(), st);

			throw InternalInconsistencyException(
				"KoreanParser::convertParseToSynNode()",
				buffer);
		}

		return snode;
	}

	int start_token = *new_token;

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
						convertParseToSynNode(iterator, snode, old_token,
											  tokenSequence, new_token, newTokens,
											  offset, token_mapping, start_place));
		iterator = iterator->next;
		child_index++;
	}
	snode->setChild(child_index,
					convertParseToSynNode(parse->headNode, snode, old_token,
										  tokenSequence, new_token, newTokens,
										  offset, token_mapping, start_place));
	child_index++;
	iterator = parse->postmods;
	while (iterator != 0) {
		snode->setChild(child_index,
						convertParseToSynNode(iterator, snode, old_token,
											  tokenSequence, new_token, newTokens,
											  offset, token_mapping, start_place));
		iterator = iterator->next;
		child_index++;
	}

	snode->setTokenSpan(start_token, *new_token - 1);
	return snode;

}*/











