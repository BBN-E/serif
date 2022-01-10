// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/parse/ar_Parser.h"

#include "Generic/parse/ParseNode.h"
#include "Generic/theories/Parse.h"
#include "Generic//theories/SynNode.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/parse/Constraint.h"
#include "Arabic/parse/ar_BWChartDecoder.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Arabic/BuckWalter/ar_Retokenizer.h"
#include "Generic/theories/LexicalTokenSequence.h"


ArabicParser::ArabicParser() : _docTheory(0), _debug(Symbol(L"parse_debug")) {
	_max_sentence_length = 100;
	_sentence = _new Symbol[_max_sentence_length];
	//this overides the Arabic ChartDecoder ArabicParser-
	if (ParamReader::isParamTrue("use_standard_parser")){
		_decoder = 0;
		_standardParser = _new DefaultParser();
		return;
	}
	_standardParser = 0;
	_ps = ParseSeeder::build();

	// read parser params
	std::string param_model_prefix =  ParamReader::getRequiredParam("parser_model");
	double frag_prob =  ParamReader::getRequiredFloatParam("parser_frag_prob");
	if (frag_prob < 0)
		throw UnexpectedInputException("ArabicParser::ArabicParser()",
			"Param `parser_frag_prob' less than zero");

	_decoder = _new BWArabicChartDecoder(param_model_prefix.c_str(), frag_prob);
}


void ArabicParser::resetForNewSentence() {
	_nodeIDGenerator.reset();
}

int ArabicParser::getParses(Parse **results, int max_num_parses,
					  TokenSequence *tokenSequence,
					  PartOfSpeechSequence *partOfSpeech,
					  NameTheory *nameTheory,
					  NestedNameTheory *nestedNameTheory,
					  ValueMentionSet *valueMentionSet)
{
	//if there is no Arabic decoder, use the default parser
	if(_decoder == 0){
		return _standardParser->getParses(results, max_num_parses, tokenSequence,
			partOfSpeech, nameTheory,nestedNameTheory, valueMentionSet);
	}
	int numConstraints = nameTheory->getNNameSpans();
	_constraints.clear();
	int sentenceLength = tokenSequence->getNTokens();

	// eliminate these two loops if _max_sentence_length and _max_constraints
	// are actually constants
	/*
		if (sentenceLength > _max_sentence_length) {
		_max_sentence_length = sentenceLength + 25;
		_sentence = _new Symbol[_max_sentence_length];
	}
	*/
	/*
	for (int i = 0; i < sentenceLength; i++) {
		_sentence[i] = tokenSequence->getToken(i)->getSymbol();
	}
	for (int j = 0; j < numConstraints; j++) {
		Constraint constraint;
		constraint.left = nameTheory->getNameSpan(j)->start;
		constraint.right = nameTheory->getNameSpan(j)->end;
		constraint.type = LanguageSpecificFunctions::getNameLabel();
		_constraints.push_back(constraint);
	}
	*/
	/*
	// for testing memory usage
	for (int test = 0; test < 11000; test++) {
		if (test % 100 == 0)
			std::cerr << test << " ";
		ParseNode* result =
			_decoder->decode(_sentence, sentenceLength, _constraints);
		delete result;
	}
	*/

	// JM: note that npa and nppos are now not replaced
	int num_seg = _putTokenSequenceInLookupChart(tokenSequence, nameTheory);
	ParseNode* result =
		_decoder->decode(_parserSeedingChart, num_seg, _constraints, false);


	if (result == 0) {
		// looks like we had to bail out
		return 0;
	}

	ParseNode* iterator = result;


	int decoder_index = 0;

	// special case for 1-best (avoid wasteful sorting necessary for n-best)
	if (max_num_parses == 1) {
		while (iterator != 0) {
			if (decoder_index == _decoder->highest_scoring_final_theory) {
				int old_token = 0, n_new_tokens = 0, start_place =0;;
				Token *newTokens[MAX_SENTENCE_TOKENS];
				CharOffset offset = tokenSequence->getToken(0)->getStartCharOffset();
				int token_mapping[MAX_SENTENCE_TOKENS];
				SynNode *n = convertParseToSynNode(iterator, 0,
					&old_token, tokenSequence, &n_new_tokens, newTokens,
					&offset, token_mapping, &start_place);

				//tokenSequence->retokenize(n_new_tokens, newTokens);
				Retokenizer::getInstance().Retokenize(tokenSequence, newTokens, n_new_tokens);
				if(DBG == 1) std::cout<<"ArabicParser()- Retokenized"<<std::endl;

				fixNameTheory(nameTheory, token_mapping);
				//delete the new tokens
				for(int i =0; i< n_new_tokens; i++){
					delete newTokens[i];
				}
				if (_debug.isActive())
					_debug << n->toFlatString() << "\n";
				Parse *p = _new Parse(tokenSequence, n, _decoder->theory_scores[decoder_index]);
				//cerr << n->toDebugString(0) << endl;
				results[0] = p;
				delete result;
				if(DBG ==1) std::cout<<"ArabicParser()- Return"<<std::endl;
				return 1;
			}
			iterator = iterator->next;
			decoder_index++;
		}
		throw UnexpectedInputException("ArabicParser::getParses()",
			"No theory flagged as highest-scoring.");
	}
	else {
		throw UnexpectedInputException("ArabicParser::getParses()",
			"ArabicParser branching factor is not 1. The Arabic parser does not\n"
			"currently support multiple theories.");

		// n-best
		int results_index = 0;
		float low_score = 0;
		int low_index = 0;
		while (iterator != 0) {
			if (results_index < max_num_parses) {
				SynNode *n;// = convertParseToSynNode(iterator, 0, 0);
				Parse *p = _new Parse(tokenSequence, n, _decoder->theory_scores[decoder_index]);
				results[results_index] = p;
				if (_decoder->theory_scores[decoder_index] < low_score) {
					low_score = _decoder->theory_scores[decoder_index];
					low_index = results_index;
				}
				results_index++;
			} else if (_decoder->theory_scores[decoder_index] > low_score) {
				delete results[low_index];
				SynNode *n;// = convertParseToSynNode(iterator, 0, 0);
				Parse *p = _new Parse(tokenSequence, n, _decoder->theory_scores[decoder_index]);
				results[low_index] = p;
				low_score = 0;
				for (int k = 0; k < max_num_parses; k++) {
					if (results[k]->getScore() < low_score) {
						low_score = results[k]->getScore();
						low_index = k;
					}
				}
			}
			_nodeIDGenerator.reset();  // reset ids to start at 0
			iterator = iterator->next;
			decoder_index++;
		}

		delete result;
		return results_index;
	}
}/*

SynNode *ArabicParser::convertParseToSynNode(ParseNode* parse,
									   SynNode* parent,
									   int *old_token,
									   const TokenSequence *tokenSequence,
									   int *new_token,
									   Token **newTokens,
									   int *offset,
									   int *token_mapping)
{
	if (parse->headNode == 0) {
		if (*new_token >= MAX_SENTENCE_TOKENS) {
			throw InternalInconsistencyException(
				"ArabicParser::convertParseToSynNode()",
				"Sam forgot to make sure that the parser can't return parses with\n"
				"too many tokens.");
		}

		SynNode *snode = _new SynNode(_nodeIDGenerator.getID(), parent,
			parse->label, 0);
		snode->setTokenSpan(*new_token, *new_token);

		int start_offset = *offset;
		int end_offset = start_offset + static_cast<int>(wcslen(parse->label.to_string())) - 1;
		Token *token = _new LexicalToken(start_offset, end_offset, parse->label);

		newTokens[*new_token] = token;
		token_mapping[*old_token] = *new_token;

		(*new_token)++;

		const Token *oldToken = tokenSequence->getToken(*old_token);
		if (end_offset < oldToken->getEndEDTOffset()) {
			(*offset) = end_offset + 1;
		}
		else {
			(*old_token)++;
			if (*old_token < tokenSequence->getNTokens()) {
				const Token *nextOldToken = tokenSequence->getToken(*old_token);
				(*offset) = nextOldToken->getStartEDTOffset();
			}
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
											  offset, token_mapping));
		iterator = iterator->next;
		child_index++;
	}
	snode->setChild(child_index,
					convertParseToSynNode(parse->headNode, snode, old_token,
										  tokenSequence, new_token, newTokens,
										  offset, token_mapping));
	child_index++;
	iterator = parse->postmods;
	while (iterator != 0) {
		snode->setChild(child_index,
						convertParseToSynNode(iterator, snode, old_token,
											  tokenSequence, new_token, newTokens,
											  offset, token_mapping));
		iterator = iterator->next;
		child_index++;
	}

	snode->setTokenSpan(start_token, *new_token - 1);
	return snode;

}


*/

void ArabicParser::fixNameTheory(NameTheory *nameTheory, int *token_mapping) {
	for (int i = 0; i < nameTheory->getNNameSpans(); i++) {
		nameTheory->getNameSpan(i)->start =
			token_mapping[nameTheory->getNameSpan(i)->start];
		nameTheory->getNameSpan(i)->end =
			token_mapping[nameTheory->getNameSpan(i)->end];
		// TODO: must remove spans past end of truncated sentences -- SRS
	}
}

int ArabicParser::getParses(Parse **results, int max_num_parses,
					  TokenSequence *tokenSequence,
					  PartOfSpeechSequence *partOfSpeech,
					  NameTheory *nameTheory,
					  NestedNameTheory *nestedNameTheory,
					  ValueMentionSet *valueMentionSet,
					  Constraint *constraints,
					  int n_constraints)

{
	//if there is no Arabic decoder, use the default parser
	if(_standardParser == 0){
//		std::cout<<"Don't use 6 argument getParses with the ArabicChartDecoder\n";
			throw InternalInconsistencyException(
				"ArabicParser::getParses()",
				"Don't use 6 argument getParses with the ArabicChartDecoder");
	}

	else{
		//Currently this function is only used by AnnotatedParsePreprocessor
		//*constraints are Desc Constraints.  In arabic appositives are marked as
		//(NP (NP)(NP) ), and (NP (N) (NP)) is a genetive construction, so
		// we don't want to force  1 word Descriptors their own NP's.
		// Remove Constraints that have only 1 word -
		//This should be done in the Annotated Parse Preprocessor, but I don't
		//want to make that language specific
		Constraint *multi_constraints  = new Constraint[n_constraints];
		int num_multi= 0;
		for(int i = 0; i< n_constraints; i++){
			if(constraints[i].left != constraints[i].right){
				multi_constraints[num_multi].right = constraints[i].right;
				multi_constraints[num_multi].left = constraints[i].left;
				multi_constraints[num_multi].type = constraints[i].type;
				num_multi++;
			}
		}
		return _standardParser->getParses(results, max_num_parses, tokenSequence, 
			partOfSpeech, nameTheory, nestedNameTheory,
			valueMentionSet, multi_constraints, num_multi);
		delete multi_constraints;
	}
}




int ArabicParser::_putTokenSequenceInLookupChart(const TokenSequence* ts, const NameTheory* nameTheory){
	const LocatedString *sentenceString = _docTheory->getSentence(ts->getSentenceNumber())->getString();
	int chart_loc =0;
	int max_entry =0;
	int min_entry =5000;
	int curr_nt =0;
	int n_nt = nameTheory->getNNameSpans();
	int tok_map[1300][2];
	int count =0;
	for(int j=0; j<MAX_SENTENCE_TOKENS ;j++){
		_lookupCount[j] =0;
	}
	for(int i =0; i< ts->getNTokens(); i++){
		if(DBG == 2) std::cout<<"Process Token Number: "<<i<<" Token "<<ts->getToken(i)->getSymbol().to_debug_string()<<std::endl;
		if((curr_nt < n_nt) && (nameTheory->getNameSpan(curr_nt)->start == i)){
			int nw = i;
			_constraints[curr_nt].left = chart_loc;
			_constraints[curr_nt].type = LanguageSpecificFunctions::getNameLabel();
			while(nw <=nameTheory->getNameSpan(curr_nt)->end){
				_lookupChart[chart_loc][0].nvString = ts->getToken(nw)->getSymbol();
				_lookupChart[chart_loc][0].start = chart_loc;
				_lookupChart[chart_loc][0].end = chart_loc;
				_lookupChart[chart_loc][0].postags.resize(0);
				_lookupChart[chart_loc][0].start_offset = ts->getToken(nw)->getStartOffsetGroup();
				_lookupChart[chart_loc][0].end_offset = ts->getToken(nw)->getEndOffsetGroup();
				//need to add lexical entries for later processing!
				_lookupCount[chart_loc]++;
				_parserSeedingChart[count] = &_lookupChart[chart_loc][0];
				if (DBG == 1) std::cout<<"\tName Entry "<<count<<"- Token: "<<i<<" String: "
					": "<<_lookupChart[chart_loc][0].nvString.to_debug_string()
					<<" Start: "<<_lookupChart[chart_loc][0].start<<" End: "<<_lookupChart[chart_loc][0].end
					<<std::endl;
				count++;
				_constraints[curr_nt].right  = chart_loc;
				chart_loc++;
				nw++;
				i++;
			}
			i--;
			curr_nt++;
		}
		else{
			_ps->processToken(*sentenceString, ts->getToken(i));
			for(int j =0; j< _ps->numGrouped(); j++){
				//if there is only one entry must need only one chart loc
				//this isn't necessary, but it reduces the size of the chart
				int seg_start, seg_end;
				if(_ps->numGrouped() ==1){
					seg_start = chart_loc;
					seg_end = chart_loc;
				}
				else{
					seg_start = _ps->groupedWordChart(j).start + chart_loc;
					seg_end = _ps->groupedWordChart(j).end +chart_loc;
				}
				if(min_entry < seg_start)
					min_entry = seg_start;
				if(seg_end > max_entry)
					max_entry = seg_end;

				_lookupChart[seg_start][_lookupCount[seg_start]]= _ps->groupedWordChart(j);
				_lookupChart[seg_start][_lookupCount[seg_start]].start = seg_start;
				_lookupChart[seg_start][_lookupCount[seg_start]].end = seg_end;
				_parserSeedingChart[count] = &_lookupChart[seg_start][_lookupCount[seg_start]];
				if(DBG == 1) std::cout<<"\tReg  Entry "<<count<<"- Token: "<<i<<" String: "
					<<_parserSeedingChart[count]->nvString.to_debug_string()
					<<" Start: "<<_parserSeedingChart[count]->start<<" End: "
					<<_parserSeedingChart[count]->end
					<<" breakdown- chart_loc: "<<chart_loc<<
					" SegStart: "<<_ps->groupedWordChart(j).start<<
					" SegEnd: "<<_ps->groupedWordChart(j).end
					<<std::endl;
				count++;
				_lookupCount[seg_start]++;

			}
			tok_map[i][0] = min_entry;
			tok_map[i][1] = max_entry;
			chart_loc = max_entry+1;
		}
	}
	//std::cout<<"_putInLooupChart() returns: "<<count<<std::endl;
	return count;
}
SynNode *ArabicParser::convertParseToSynNode(ParseNode* parse,
									   SynNode* parent,
									   int *old_token,
									   const TokenSequence *tokenSequence,
									   int *new_token,
									   Token **newTokens,
									   CharOffset *offset,
									   int *token_mapping, int* start_place)
{
	const LexicalTokenSequence *lexicaTokenSequence = dynamic_cast<const LexicalTokenSequence*>(tokenSequence);
	if (!lexicaTokenSequence) throw InternalInconsistencyException("ArabicParser::convertParseToSynNode",
		"This ArabicMTNormalizer requires a LexicalTokenSequence.");
	if (parse->headNode == 0) {
		if (*new_token >= MAX_SENTENCE_TOKENS) {
			throw InternalInconsistencyException(
				"ArabicParser::convertParseToSynNode()",
				"Sam forgot to make sure that the parser can't return parses with\n"
				"too many tokens.");
		}

		SynNode *snode = _new SynNode(_nodeIDGenerator.getID(), parent,
			parse->label, 0);
		//find the correct token in the lookup place
		int st = (*start_place);
		LexicalToken* token = NULL;
		for(int i =0; i< _lookupCount[st]; i++){
			if((_lookupChart[st][i].nvString == parse->label) &&
				(_lookupChart[st][i].end ==parse->chart_end_index))
			{
				OffsetGroup start_offset = _lookupChart[st][i].start_offset;
				OffsetGroup end_offset = _lookupChart[st][i].end_offset;

				const LexicalToken *oldToken = lexicaTokenSequence->getToken(*old_token);
				int orig_token = oldToken->getOriginalTokenIndex();

				int n_lex =0;
				/*
				//need to add lexical entry info here
				LexicalEntry* lexArray[20];
				LexcialEntry* results[20];
				for(int k=0; k<_lookupChart[st][i].num_ent;k++){
					if(_lookupChart[st][i].pt[k].num_lex_ent==1){
						lexArray[n_lex]= _lookupChart[st][i].pt[k].lex_ents[0];
					}
					else if(_lex->hasKey(parse->label)){
						int n = _lex->getEntriesByKey(parse->label, results);
						for(int m =0; m<n; m++){
							if(results[m]-> ==
					}
					else{
						LexicalEntry* le;

						le = new LexicalEntry(_lex->getNextID();
						_lex->addEntry(le);
				*/
				token = _new LexicalToken(start_offset, end_offset, parse->label, orig_token);
				snode->setTokenSpan(*new_token, *new_token);

				newTokens[*new_token] = token;
				token_mapping[*old_token] = *new_token;

				(*new_token)++;


				if (end_offset.value<CharOffset>() >= oldToken->getEndOffsetGroup().value<CharOffset>()) {
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
				"ArabicParser::convertParseToSynNode()",
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
											  lexicaTokenSequence, new_token, newTokens,
											  offset, token_mapping, start_place));
		iterator = iterator->next;
		child_index++;
	}
	snode->setChild(child_index,
					convertParseToSynNode(parse->headNode, snode, old_token,
										  lexicaTokenSequence, new_token, newTokens,
										  offset, token_mapping, start_place));
	child_index++;
	iterator = parse->postmods;
	while (iterator != 0) {
		snode->setChild(child_index,
						convertParseToSynNode(iterator, snode, old_token,
											  lexicaTokenSequence, new_token, newTokens,
											  offset, token_mapping, start_place));
		iterator = iterator->next;
		child_index++;
	}

	snode->setTokenSpan(start_token, *new_token - 1);
	return snode;

}
void ArabicParser::setMaxParserSeconds(int maxsecs) {
		_decoder->setMaxParserSeconds(maxsecs);
}










