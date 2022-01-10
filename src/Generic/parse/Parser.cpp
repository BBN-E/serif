// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "parse/Parser.h"
#include "parse/xx_Parser.h"

//#include "parse/ParseNode.h"
//#include "theories/Parse.h"
//#include "theories/SynNode.h"
//#include "parse/ParserTags.h"
//#include "theories/TokenSequence.h"
//#include "theories/NameTheory.h"
//#include "parse/Constraint.h"
//#include "parse/ChartDecoder.h"
//#include "parse/LanguageSpecificFunctions.h"
//#include "common/ParamReader.h"
//#include "common/UnexpectedInputException.h"
//
//
//Parser::Parser() : _debug(Symbol(L"parse_debug")) {
//	_max_sentence_length = 100;
//	_max_constraints = 50;
//	_sentence = _new Symbol[_max_sentence_length];
//	_constraints = _new Constraint[_max_constraints];
//
//	// read parser params
//	char param_model_prefix[501];
//	if (!ParamReader::getParam("parser_model",param_model_prefix,									 500))	{
//		throw UnexpectedInputException("Parser::Parser()",
//									   "Param `parser_model' not defined");
//	}
//	char frag_prob_str[500];
//	if (!ParamReader::getParam("parser_frag_prob",frag_prob_str,		500))	{
//		throw UnexpectedInputException("ChartDecoder::ChartDecoder()",
//			"Param `parser_frag_prob' not defined");
//	}
//	double frag_prob = atof(frag_prob_str);
//	if (frag_prob < 0)
//		throw UnexpectedInputException("ChartDecoder::ChartDecoder()",
//			"Param `parser_frag_prob' less than zero");
//
//	_decoder = _new ChartDecoder(param_model_prefix, frag_prob);
//}
//
//
//void Parser::resetForNewSentence() {
//	_nodeIDGenerator = IDGenerator(0);
//}
//
//int Parser::getParses(Parse **results, int max_num_parses,
//					  const TokenSequence *tokenSequence,
//					  const NameTheory *nameTheory)
//{
//	int numConstraints = nameTheory->n_name_spans;
//	int sentenceLength = tokenSequence->getNTokens();
//
//	// eliminate these two loops if _max_sentence_length and _max_constraints
//	// are actually constants
//	if (sentenceLength > _max_sentence_length) {
//		_max_sentence_length = sentenceLength + 25;
//		_sentence = _new Symbol[_max_sentence_length];
//	}
//
//	if (numConstraints > _max_constraints) {
//		_max_constraints = numConstraints + 10;
//		_constraints = _new Constraint[_max_constraints];
//	}
//
//	for (int i = 0; i < sentenceLength; i++) {
//		_sentence[i] = tokenSequence->getToken(i)->getSymbol();
//	}
//	for (int j = 0; j < numConstraints; j++) {
//		_constraints[j].left = nameTheory->nameSpans[j]->start;
//		_constraints[j].right = nameTheory->nameSpans[j]->end;
//		_constraints[j].type = LanguageSpecificFunctions::getNameLabel();
//	}
//
//	//for testing memory usage
//	//
//	//for (int test = 0; test < 11000; test++) {
//	//	if (test % 100 == 0)
//	//		std::cerr << test << " ";
//	//	ParseNode* result =
//	//		_decoder->decode(_sentence, sentenceLength,
//	//		_constraints, numConstraints);
//	//	delete result;
//	//}
//
//	// JM: note that npa and nppos are now not replaced
//	ParseNode* result =
//		_decoder->decode(_sentence, sentenceLength,
//		_constraints, numConstraints, false);
//
//	ParseNode* iterator = result;
//	int decoder_index = 0;
//
//	// special case for 1-best (avoid wasteful sorting necessary for n-best)
//	if (max_num_parses == 1) {
//		while (iterator != 0) {
//			if (decoder_index == _decoder->highest_scoring_final_theory) {
//				SynNode *n = convertParseToSynNode(iterator, 0, 0);
//				if (_debug.isActive())
//					_debug << n->toFlatString() << "\n";
//				Parse *p = _new Parse(n, _decoder->theory_scores[decoder_index]);
//				//cerr << n->toDebugString(0) << endl;
//				results[0] = p;
//				delete result;
//				return 1;
//			}
//			iterator = iterator->next;
//			decoder_index++;
//		}
//	}
//
//	// n-best
//	int results_index = 0;
//	float low_score = 0;
//	int low_index = 0;
//	while (iterator != 0) {
//		if (results_index < max_num_parses) {
//			SynNode *n = convertParseToSynNode(iterator, 0, 0);
//			Parse *p = _new Parse(n, _decoder->theory_scores[decoder_index]);
//			results[results_index] = p;
//			if (_decoder->theory_scores[decoder_index] < low_score) {
//				low_score = _decoder->theory_scores[decoder_index];
//				low_index = results_index;
//			}
//			results_index++;
//		} else if (_decoder->theory_scores[decoder_index] > low_score) {
//			delete results[low_index];
//			SynNode *n = convertParseToSynNode(iterator, 0, 0);
//			Parse *p = _new Parse(n, _decoder->theory_scores[decoder_index]);
//			results[low_index] = p;
//			low_score = 0;
//			for (int k = 0; k < max_num_parses; k++) {
//				if (results[k]->getScore() < low_score) {
//					low_score = results[k]->getScore();
//					low_index = k;
//				}
//			}
//		}
//		iterator = iterator->next;
//		decoder_index++;
//	}
//
//	delete result;
//	return results_index;
//
//}
//
//SynNode *Parser::convertParseToSynNode(ParseNode* parse,
//									   SynNode* parent,
//									   int start_token)
//{
//
//	if (parse->headNode == 0) {
//		SynNode *snode = _new SynNode(_nodeIDGenerator.getID(), parent,
//			parse->label, 0);
//		snode->setTokenSpan(start_token, start_token);
//		return snode;
//	}
//
//	// count children, store last premod node
//	int n_children = 0;
//	ParseNode* iterator = parse->premods;
//	ParseNode* last_premod = parse->premods;
//	while (iterator != 0) {
//		n_children++;
//		last_premod = iterator;
//		iterator = iterator->next;
//	}
//	int head_index = n_children;
//	n_children++;
//	iterator = parse->postmods;
//	while (iterator != 0) {
//		n_children++;
//		iterator = iterator->next;
//	}
//
//	SynNode *snode = _new SynNode(_nodeIDGenerator.getID(), parent,
//		parse->label, n_children);
//	snode->setHeadIndex(head_index);
//
//	int token_index = start_token;
//
//	// make life easier
//	if (parse->premods != 0) {
//		parse->premods_reverse(parse->premods, parse->premods->next);
//		parse->premods->next = 0;
//		// must assign premods as well, since otherwise this branch won't be deleted
//		parse->premods = last_premod;
//	}
//
//	// create children
//	int child_index = 0;
//	iterator = last_premod;
//	while (iterator != 0) {
//		snode->setChild(child_index,
//			convertParseToSynNode(iterator, snode, token_index));
//		iterator = iterator->next;
//		token_index = snode->getChild(child_index)->getEndToken() + 1;
//		child_index++;
//	}
//	snode->setChild(child_index,
//			convertParseToSynNode(parse->headNode, snode, token_index));
//	token_index = snode->getChild(child_index)->getEndToken() + 1;
//	child_index++;
//	iterator = parse->postmods;
//	while (iterator != 0) {
//		snode->setChild(child_index,
//			convertParseToSynNode(iterator, snode, token_index));
//		iterator = iterator->next;
//		token_index = snode->getChild(child_index)->getEndToken() + 1;
//		child_index++;
//	}
//
//	snode->setTokenSpan(start_token, token_index - 1);
//	return snode;
//
//}
//
//

boost::shared_ptr<Parser::Factory> &Parser::_factory() {
	static boost::shared_ptr<Parser::Factory> factory(new GenericParserFactory());
	return factory;
}

