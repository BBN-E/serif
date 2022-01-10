// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/DefaultParser.h"

#include "Generic/parse/ParseNode.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/NestedNameTheory.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntityType.h"
#include "Generic/parse/Constraint.h"
#include "Generic/parse/ChartDecoder.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UnexpectedInputException.h"
#include <boost/scoped_ptr.hpp>



DefaultParser::DefaultParser() : _debug(Symbol(L"parse_debug")) {
	_sentence = _new Symbol[MAX_SENTENCE_TOKENS];

	// read parser params
	std::string param_model_prefix = ParamReader::getRequiredParam("parser_model");

	double frag_prob = ParamReader::getRequiredFloatParam("parser_frag_prob");
	if (frag_prob < 0)
		throw UnexpectedInputException("DefaultParser::DefaultParser()",
			"Parameter 'parser_frag_prob' should be >= 0");
	
	use_hyphen_constraints = ParamReader::isParamTrue("use_hyphen_constraints_in_parser");

	//read in  aux POS table if specified, else make one that is empty
	std::string aux_pos = ParamReader::getParam("aux_pos_table");
	if (!aux_pos.empty()) {
		boost::scoped_ptr<UTF8InputStream> pos_uis_scoped_ptr(UTF8InputStream::build(aux_pos.c_str()));
		UTF8InputStream& pos_uis(*pos_uis_scoped_ptr);
		_auxPosTable = _new PartOfSpeechTable(pos_uis);
		pos_uis.close();
	}
	else{
		_auxPosTable = _new PartOfSpeechTable();
	}

	skip_unimportant_sentences = ParamReader::isParamTrue("parser_skip_unimportant_sentences");
	unimportant_token_limit = ParamReader::getOptionalIntParamWithDefaultValue("unimportant_sentence_token_limit", 99999);

	_defaultDecoder = _new ChartDecoder(param_model_prefix.c_str(), frag_prob, _auxPosTable);
	_decoder = _defaultDecoder;
	
	std::string parser_type = ParamReader::getParam("parser_case_type");
	if (!parser_type.empty()) {
		if (parser_type == "UPPER")
			_defaultDecoder->setDecoderType(ChartDecoder::UPPER);
		else if (parser_type == "LOWER")
			_defaultDecoder->setDecoderType(ChartDecoder::LOWER);
	}
	if (_defaultDecoder) 
		_defaultDecoder->readCaches();

	param_model_prefix = ParamReader::getParam("lowercase_parser_model");
	if (!param_model_prefix.empty()) {
		_lowerCaseDecoder = _new ChartDecoder(param_model_prefix.c_str(), frag_prob, _auxPosTable);
		_lowerCaseDecoder->setDecoderType(ChartDecoder::LOWER);
	} else _lowerCaseDecoder = 0;
        if (_lowerCaseDecoder) 
          _lowerCaseDecoder->readCaches();

	param_model_prefix = ParamReader::getParam("uppercase_parser_model");
	if (!param_model_prefix.empty()) {
		_upperCaseDecoder = _new ChartDecoder(param_model_prefix.c_str(), frag_prob, _auxPosTable);
		_upperCaseDecoder->setDecoderType(ChartDecoder::UPPER);
	} else _upperCaseDecoder = 0;
        if (_upperCaseDecoder) 
          _upperCaseDecoder->readCaches();
}

DefaultParser::~DefaultParser() {
	if (_sentence != 0)         { delete[] _sentence; }
	if (_lowerCaseDecoder != 0) { delete _lowerCaseDecoder; }
	if (_upperCaseDecoder != 0) { delete _upperCaseDecoder; }
	if (_defaultDecoder != 0)   { delete _defaultDecoder; }
	if (_auxPosTable != 0)      { delete _auxPosTable;  }
	// We do *not* deallocate _decoder, since it's the same thing as _defaultDecoder,
	// and we don't want to deallocate it twice:
	// if (_decoder != 0)          { delete _decoder; }
}

void DefaultParser::resetForNewDocument(DocTheory *docTheory) {
	_decoder = _defaultDecoder;
	if (docTheory != 0) {
		int doc_case = docTheory->getDocumentCase();
		if (doc_case == DocTheory::LOWER && _lowerCaseDecoder != 0) {
			SessionLogger::dbg("low_dec_0") << "Using lowercase decoder\n";
			_decoder = _lowerCaseDecoder;
		} else if (doc_case == DocTheory::UPPER && _upperCaseDecoder != 0) {
			SessionLogger::dbg("upp_dec_0") << "Using uppercase decoder\n";
			_decoder = _upperCaseDecoder;
		}
	}
}

int DefaultParser::getParses(Parse **results, int max_num_parses,
					  const TokenSequence *tokenSequence,
					  const PartOfSpeechSequence *partOfSpeech,
					  const NameTheory *nameTheory,
					  const NestedNameTheory *nestedNameTheory,
					  const ValueMentionSet *valueMentionSet)
{
	return getParses(results, max_num_parses, tokenSequence, partOfSpeech,
		nameTheory, nestedNameTheory, valueMentionSet, 0, 0);
}

int DefaultParser::getParses(Parse **results, int max_num_parses,
					  const TokenSequence *tokenSequence,
					  const PartOfSpeechSequence *partOfSpeech,
					  const NameTheory *nameTheory,
					  const NestedNameTheory *nestedNameTheory,
					  const ValueMentionSet *valueMentionSet,
					  Constraint *constraints,
					  int n_constraints)
{
	// set sentence tokens
	int sentenceLength = tokenSequence->getNTokens();

	if (sentenceLength > MAX_SENTENCE_TOKENS) {
		throw InternalInconsistencyException("DefaultParser::getParses()",
			"number of tokens in sentence > MAX_SENTENCE_TOKENS"); 
	}

	if (sentenceLength == 0) {
		results[0] = _new Parse(tokenSequence);
		return 1;
	}

	int hyphen_count = 0;
	bool disallow_subsumed_name[MAX_SENTENCE_TOKENS];
	for (int i = 0; i < sentenceLength; i++) {
		// set tokens
		_sentence[i] = tokenSequence->getToken(i)->getSymbol();
		// count hyphens so we know the maximum possible hyphen constraints
		if (use_hyphen_constraints && LanguageSpecificFunctions::isHyphen(_sentence[i]))
			hyphen_count++;
		// initialize this array
		disallow_subsumed_name[i] = false;
	}
	
	int max_fixed_mentions = nameTheory->getNNameSpans();
	if (valueMentionSet != 0)
		max_fixed_mentions += valueMentionSet->getNValueMentions();
	
	// disallow names with type OTH?
	for (int m = 0; m < nameTheory->getNNameSpans(); m++) {
		if (!nameTheory->getNameSpan(m)->type.isRecognized())
			disallow_subsumed_name[nameTheory->getNameSpan(m)->start] = true;
	}

	_constraints.clear();
	int j = 0;
	for (j = 0; j < nameTheory->getNNameSpans(); j++) {
		
		// this seems like it would never be true, but what do I know
		if (nameTheory->getNameSpan(j)->type.isRecognized() &&
			disallow_subsumed_name[nameTheory->getNameSpan(j)->start])
			continue;

		// at the moment, this is only implemented in English
		int list_end = LanguageSpecificFunctions::findNameListEnd(tokenSequence, 
			nameTheory, nameTheory->getNameSpan(j)->start);
		if (list_end != -1 && nameTheory->getNameSpan(j)->type.isRecognized()) {

			// add a list entry instead of the names in the list
			_constraints.push_back(
				Constraint(nameTheory->getNameSpan(j)->start,
						   list_end,
						   ParserTags::LIST,
						   nameTheory->getNameSpan(j)->type));
			for (int k = j + 1; k < nameTheory->getNNameSpans(); k++) {
				if (nameTheory->getNameSpan(k)->end == list_end) {
					j = k;
					break;
				}
			}
		} else {

			// add in constraints from the name theory
			_constraints.push_back(
				Constraint(nameTheory->getNameSpan(j)->start,
							nameTheory->getNameSpan(j)->end,
							((nameTheory->getNameSpan(j)->type.isRecognized()) ?
							 LanguageSpecificFunctions::getNameLabel() :
							 ParserTags::SPLIT),
							nameTheory->getNameSpan(j)->type));

		}
	}

	// Add constraints for nested names
	for (j = 0; j < nestedNameTheory->getNNameSpans(); j++) {
		_constraints.push_back(
			Constraint(nestedNameTheory->getNameSpan(j)->start,
			nestedNameTheory->getNameSpan(j)->end,
			ParserTags::NESTED_NAME_CONSTRAINT,
			nestedNameTheory->getNameSpan(j)->type));
	}

	/*for (int v = 0; v < valueMentionSet->getNValueMentions(); v++) {
		ValueMention *vm = valueMentionSet->getValueMention(v);
		if (vm->isTimexValue()) {
			Constraints.push_back(
				Constraint(vm->getStartToken(),
						   vm->getEndToken(),
						   ParserTags::DATE_CONSTRAINT,
						   EntityType::getUndetType()));
		}
}*/

	/* MRF: 10/29/09
	   This is a hack for when Serif is used to parse slots for GALE Distillation QueryInterpretation
	   Distillation slots are typically names or NPs (e.g. 'French president Nicolas Sarkozy' 
	   or 'looting in Iraq after the US invasion'). To make these NPs more likel to parse reasonably,
	   we append prefixes to a slot in QueryInterpreter (EnSlot::EnSlot(), so that the slots are Sentence-like.
	   Current Prefixes are: 
	     The topic is
	     He is discussing
	     Tell me about
	     It says that
	   The sentences are processed as both mixed and lower case.

	   Signficantly more complex transformations happen to questions that have SlotType UNKNOWN_QUESTION
	   The hack constrains the parser so that a constiuent cannot cross the prefix-slot_text boundary.

	   Since the constraint can be VERY detrimental to a parse, there are two checks on whether or not to use it:
	   (a) runtime_disitillation_serif must be set to TRUE in the parameter file
	   (b) the current sentences must start with one of the known prefix strings  

	   Check (b) means that this code needs to be updated if the prefixes in EnSlot::EnSlot() change.  
	*/

	if(ParamReader::getOptionalTrueFalseParamWithDefaultVal("runtime_distillation_serif", false) && tokenSequence->getNTokens() >= 4)
	{
		
		if( ((tokenSequence->getToken(0)->getSymbol() == Symbol(L"The") || tokenSequence->getToken(0)->getSymbol() == Symbol(L"the")) && 
				 tokenSequence->getToken(1)->getSymbol() == Symbol(L"topic") &&
				 tokenSequence->getToken(2)->getSymbol() == Symbol(L"is")) ||
			((tokenSequence->getToken(0)->getSymbol() == Symbol(L"He") || tokenSequence->getToken(0)->getSymbol() == Symbol(L"he")) &&
				 tokenSequence->getToken(1)->getSymbol() == Symbol(L"is") &&
				 tokenSequence->getToken(2)->getSymbol() == Symbol(L"discussing")) ||
			((tokenSequence->getToken(0)->getSymbol() == Symbol(L"Tell") || tokenSequence->getToken(0)->getSymbol() == Symbol(L"tell")) &&
				 tokenSequence->getToken(1)->getSymbol() == Symbol(L"me") &&
				 tokenSequence->getToken(2)->getSymbol() == Symbol(L"about")) ||
			((tokenSequence->getToken(0)->getSymbol() == Symbol(L"It") || tokenSequence->getToken(0)->getSymbol() == Symbol(L"it")) &&
				 tokenSequence->getToken(1)->getSymbol() == Symbol(L"says") &&
				 tokenSequence->getToken(2)->getSymbol() == Symbol(L"that")))
		{
			
			_constraints.push_back(
				Constraint(3, tokenSequence->getNTokens() - 1,
						   Symbol(), EntityType::getUndetType()));
		}
	}


	// add in constraints that were passed in as is
	std::copy(constraints, constraints+n_constraints,
			  std::back_inserter(_constraints));

	// add in hyphen constraints
	if (use_hyphen_constraints) {
		EntityType type;
		int end_of_last_hyphen_constraint = -1;
		for (int i = 0; i < sentenceLength; i++) {
			//if (numConstraints >= _max_constraints)
			//	continue;
			if (LanguageSpecificFunctions::isHyphen(_sentence[i])) {
				int left = -1;
				int right = -1;
				for (j = 0; j < nameTheory->getNNameSpans(); j++) {
					if (i + 1 == nameTheory->getNameSpan(j)->start) {
						right = nameTheory->getNameSpan(j)->end;
						type = nameTheory->getNameSpan(j)->type;
					}
					if (i - 1 == nameTheory->getNameSpan(j)->end) {
						left = nameTheory->getNameSpan(j)->start;
					}
				}
				if (left != -1 && 
					right != -1 &&
					left > end_of_last_hyphen_constraint) 
				{
					int count = 0;
					// no hyphens in the names!
					for (int k = left; k < right; k++) {
						if (LanguageSpecificFunctions::isHyphen(_sentence[k]))
							count++;
					}
					if (count == 1) {
						_constraints.push_back(
							Constraint(left, right, 
									   ParserTags::HYPHEN, type));
						i = right + 1;
						end_of_last_hyphen_constraint = right;
					}
				}
			}
		}
	}
	
	ParseNode *result;

	bool default_parse = false;
	if (skip_unimportant_sentences && 
		nameTheory->getNNameSpans() == 0 && 
		valueMentionSet->getNValueMentions() == 0 &&
		tokenSequence->getNTokens() <= unimportant_token_limit) 
	{
		result = _decoder->returnDefaultParse(_sentence, sentenceLength, 
											  _constraints, false);
		default_parse = true;
	} else {
		// JM: note that npa and nppos are now not replaced
		result =
			_decoder->decode(_sentence, sentenceLength,
							 _constraints, false, partOfSpeech);
	}

	ParseNode *iterator = result;
	int decoder_index = 0;
	
	// special case for 1-best (avoid wasteful sorting necessary for n-best)
	if (max_num_parses == 1) {
		while (iterator != 0) {
			if (decoder_index == _decoder->highest_scoring_final_theory) {
				SynNode *n = convertParseToSynNode(iterator, 0, nameTheory, nestedNameTheory, tokenSequence, 0);
				if (_debug.isActive())
					_debug << n->toFlatString() << "\n";
				Parse *p = _new Parse(tokenSequence, n, _decoder->theory_scores[decoder_index]);
				p->setDefaultParse(default_parse);
				results[0] = p;
				delete result;
				return 1;
			}
			iterator = iterator->next;
			decoder_index++;
		}
	}

	// n-best 
	int results_index = 0;
	float low_score = 0;
	int low_index = 0;
	while (iterator != 0) {
		if (results_index < max_num_parses) {
			SynNode *n = convertParseToSynNode(iterator, 0, nameTheory, nestedNameTheory, tokenSequence, 0);
			Parse *p = _new Parse(tokenSequence, n, _decoder->theory_scores[decoder_index]);
			p->setDefaultParse(default_parse);
			results[results_index] = p;
			if (_decoder->theory_scores[decoder_index] < low_score) {
				low_score = _decoder->theory_scores[decoder_index];
				low_index = results_index;
			}
			results_index++;
		} else if (_decoder->theory_scores[decoder_index] > low_score) {
			delete results[low_index];
			SynNode *n = convertParseToSynNode(iterator, 0, nameTheory, nestedNameTheory, tokenSequence, 0);
			Parse *p = _new Parse(tokenSequence, n, _decoder->theory_scores[decoder_index]);
			p->setDefaultParse(default_parse);
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

void DefaultParser::writeCaches() {
  if (_defaultDecoder)
    _defaultDecoder->writeCaches();
  if (_lowerCaseDecoder)
    _lowerCaseDecoder->writeCaches();
  if (_upperCaseDecoder)
    _upperCaseDecoder->writeCaches();
}

void DefaultParser::cleanup() {
  if (_defaultDecoder)
    _defaultDecoder->cleanup();
  if (_lowerCaseDecoder)
    _lowerCaseDecoder->cleanup();
  if (_upperCaseDecoder)
    _upperCaseDecoder->cleanup();
}
