// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/WordConstants.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/names/discmodel/PIdFModel.h"
#include "Chinese/names/discmodel/ch_PIdFCharModel.h"
#include "Chinese/values/ch_ValueRecognizer.h"
#include <boost/foreach.hpp> 

#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UnrecoverableException.h"

using namespace std;

ChineseValueRecognizer::ChineseValueRecognizer()
	:  _valueDecoder(0), _valueCharDecoder(0), _timexDecoder(0), _timexCharDecoder(0),
	   _otherValueDecoder(0), _otherValueCharDecoder(0), _timexTagSet(0), _otherValueTagSet(0), 
	   _tagSet(0), _sent_no(0), _tokenSequence(0), _docTheory(0), _use_dual_model(false),
	   DO_VALUES(true), _use_pidf(true), _use_rules(true)
{
	DO_VALUES = ParamReader::getOptionalTrueFalseParamWithDefaultVal("do_values_stage", true);
	if (!DO_VALUES) return;

	_use_rules = ParamReader::getOptionalTrueFalseParamWithDefaultVal("values_use_rules", true);
	_use_pidf = ParamReader::getOptionalTrueFalseParamWithDefaultVal("values_use_pidf", true);

	if (_use_pidf) {
		std::string values_mode = ParamReader::getParam("values_mode");
		if (values_mode == "tokens")
			_run_on_tokens = true;
		else
			_run_on_tokens = false;
		
		_use_dual_model = ParamReader::isParamTrue("values_use_dual_model");
	
		if (!_use_dual_model) {
			std::string tag_set_file = ParamReader::getRequiredParam("values_tag_set_file");
			std::string features_file = ParamReader::getRequiredParam("values_features_file");
			std::string model_file = ParamReader::getRequiredParam("values_model_file");
			
			if (_run_on_tokens) {
				_valueDecoder = _new PIdFModel(PIdFModel::DECODE, tag_set_file.c_str(),
												features_file.c_str(), model_file.c_str(), 0);
				_tagSet = _valueDecoder->getTagSet();
			}
			else {
				_valueCharDecoder = _new PIdFCharModel(tag_set_file.c_str(), features_file.c_str(), model_file.c_str());
				_tagSet = _valueCharDecoder->getTagSet();
			}
		}
		else {
			// Read parameters
			std::string tag_set_file = ParamReader::getRequiredParam("timex_tag_set_file");
			std::string features_file = ParamReader::getRequiredParam("timex_features_file");
			std::string model_file = ParamReader::getRequiredParam("timex_model_file");

			if (_run_on_tokens) {
				_timexDecoder = _new PIdFModel(PIdFModel::DECODE, tag_set_file.c_str(),
									  features_file.c_str(), model_file.c_str(), 0);
				//_timexDecoder->setWordFeaturesMode(IdFWordFeatures::TIMEX);
				_timexTagSet = _timexDecoder->getTagSet();
			}
			else {
				_timexCharDecoder = _new PIdFCharModel(tag_set_file.c_str(), features_file.c_str(), model_file.c_str());
				_timexTagSet = _timexCharDecoder->getTagSet();
			}

			
			tag_set_file = ParamReader::getRequiredParam("other_value_tag_set_file");
			features_file = ParamReader::getRequiredParam("other_value_features_file");
			model_file = ParamReader::getRequiredParam("other_value_model_file");

			if (_run_on_tokens) {
				_otherValueDecoder = _new PIdFModel(PIdFModel::DECODE, tag_set_file.c_str(),
									  features_file.c_str(), model_file.c_str(), 0);
				//_timexDecoder->setWordFeaturesMode(IdFWordFeatures::TIMEX);
				_otherValueTagSet = _otherValueDecoder->getTagSet();
			}
			else {
				_otherValueCharDecoder = _new PIdFCharModel(tag_set_file.c_str(), features_file.c_str(), model_file.c_str());
				_otherValueTagSet = _otherValueCharDecoder->getTagSet();
			}
		}
	}
}

ChineseValueRecognizer::~ChineseValueRecognizer() {
	delete _valueDecoder;
	delete _valueCharDecoder;
	delete _timexDecoder;
	delete _timexCharDecoder;
	delete _otherValueDecoder;
	delete _otherValueCharDecoder;
}


void ChineseValueRecognizer::resetForNewDocument(DocTheory *docTheory) {
	if (!DO_VALUES) return;
	
	_docTheory = docTheory;
	
	if (_use_pidf) {
		if (!_use_dual_model) {
			if (_run_on_tokens) 
				_valueDecoder->resetForNewDocument(docTheory);
			else
				_valueCharDecoder->resetForNewDocument(docTheory);
		}
		else {
			if (_run_on_tokens) {
				_timexDecoder->resetForNewDocument(docTheory);
				_otherValueDecoder->resetForNewDocument(docTheory);
			} else {
				_timexCharDecoder->resetForNewDocument(docTheory);
				_otherValueCharDecoder->resetForNewDocument(docTheory);
			}
		}
	}
}

int ChineseValueRecognizer::getValueTheories(ValueMentionSet **results, int max_theories, 
												 TokenSequence *tokenSequence) 
{
	if (!DO_VALUES) {
		results[0] = _new ValueMentionSet(tokenSequence, 0);
		return 1;
	}
	
	_tokenSequence = tokenSequence;
	_sent_no = tokenSequence->getSentenceNumber();

	// make the contents of POSTDATE regions automatically into dates
	if (_docTheory != NULL) {
		EDTOffset sent_offset = _docTheory->getSentence(_sent_no)->getStartEDTOffset();
		if (_docTheory->getMetadata() != 0 &&
			_docTheory->getMetadata()->getCoveringSpan(sent_offset, POSTDATE_SYM) != 0 &&
			tokenSequence->getNTokens() > 0) {
			try {
				ValueMentionUID uid = ValueMention::makeUID(_sent_no, 0);
				// T0 DO: replace TIMEX2 with something that isn't hard coded
				ValueMention *vm = _new ValueMention(_sent_no, uid, 0, 
					tokenSequence->getNTokens() - 1, Symbol(L"TIMEX2"));
				results[0] = _new ValueMentionSet(tokenSequence, 1);
				results[0]->takeValueMention(0, vm);
				return 1;
			} catch (InternalInconsistencyException &e) {
				// Should never happen unless max # of value mentions is strangely set to 0
				SessionLogger::err("value_mention_uid") << e;
			}
		}
	}

	SpanList allValues;
	float scorePenalty = 0.0;
	
	if (_use_pidf) {
		SpanList pidfValues;
		if (_use_dual_model) {
			pidfValues = getPIdFValuesDualModel(tokenSequence, scorePenalty);
		} else {
			pidfValues = getPIdFValuesSingleModel(tokenSequence, scorePenalty);
		}
		allValues.insert(allValues.end(), pidfValues.begin(), pidfValues.end());
	}

	if (_use_rules) {
		SpanList ruleValues = identifyRuleRepositoryValues(tokenSequence);
		allValues.insert(allValues.end(), ruleValues.begin(), ruleValues.end());
	}

	results[0] = createValueMentionSet(tokenSequence, allValues);
	if (scorePenalty != 0.0)
		results[0]->setScore(-scorePenalty);
	return 1;
}

ValueRecognizer::SpanList ChineseValueRecognizer::getPIdFValuesSingleModel(TokenSequence *tokenSequence,
																		   float& penalty) 
{
	SpanList valueSpans;
	if (_run_on_tokens) {
		PIdFSentence sentence(_valueDecoder->getTagSet(), *tokenSequence);
		_valueDecoder->decode(sentence);
		valueSpans = ValueRecognizer::collectValueSpans(sentence, _tagSet);
	}
	else {
		PIdFCharSentence sentence(_valueCharDecoder->getTagSet(), *tokenSequence);
		_valueCharDecoder->decode(sentence);
		correctSentence(sentence);
		valueSpans = collectValueSpans(sentence, _tagSet, penalty);
	}
	return valueSpans;
}

ValueRecognizer::SpanList ChineseValueRecognizer::getPIdFValuesDualModel(TokenSequence *tokenSequence,
																		 float& penalty) 
{
	SpanList valueSpans;
	if (_run_on_tokens) {
		PIdFSentence timexSentence(_timexDecoder->getTagSet(), *tokenSequence);		
		_timexDecoder->decode(timexSentence);
		SpanList timexSpans = ValueRecognizer::collectValueSpans(timexSentence, _tagSet);
		valueSpans.insert(valueSpans.end(), timexSpans.begin(), timexSpans.end());

		PIdFSentence otherValueSentence(_otherValueDecoder->getTagSet(), *tokenSequence);	
		_otherValueDecoder->decode(otherValueSentence);
		SpanList otherValueSpans = ValueRecognizer::collectValueSpans(otherValueSentence, _tagSet);
		valueSpans.insert(valueSpans.end(), otherValueSpans.begin(), otherValueSpans.end());
	}
	else {
		PIdFCharSentence timexSentence(_timexCharDecoder->getTagSet(), *tokenSequence);		
		_timexCharDecoder->decode(timexSentence);
		SpanList timexSpans = collectValueSpans(timexSentence, _tagSet, penalty);
		valueSpans.insert(valueSpans.end(), timexSpans.begin(), timexSpans.end());

		PIdFCharSentence otherValueSentence(_otherValueCharDecoder->getTagSet(), *tokenSequence);		
		_otherValueCharDecoder->decode(otherValueSentence);
		SpanList otherValueSpans = collectValueSpans(otherValueSentence, _tagSet, penalty);
		valueSpans.insert(valueSpans.end(), otherValueSpans.begin(), otherValueSpans.end());
	}
	return valueSpans;
}


ValueRecognizer::SpanList ChineseValueRecognizer::collectValueSpans(const PIdFCharSentence &sentence,
                                                                    const DTTagSet *tagSet, float& penalty) 
{
	int NONE_ST_tag = tagSet->getTagIndex(NONE_ST);
	SpanList results;

	int char_index = 0;
	int last_end_char = -1;
	while (char_index < sentence.getLength()) {

		while (char_index < sentence.getLength() &&
			!(sentence.getTag(char_index) != NONE_ST_tag &&
				 tagSet->isSTTag(sentence.getTag(char_index))))
		{ char_index++; }

		if (char_index >= sentence.getLength()) break;

		int tag = sentence.getTag(char_index);

		int end_index = char_index;
		while (end_index+1 < sentence.getLength() &&
			   tagSet->isCOTag(sentence.getTag(end_index+1)))
		{ end_index++; }

		// penalize if the value crosses a token boundary 
		if (char_index > 0 && (sentence.getTokenFromChar(char_index - 1) ==
			sentence.getTokenFromChar(char_index)))
		{
			penalty += 100.0;
		}
		if (end_index < sentence.getLength() - 1 &&
			(sentence.getTokenFromChar(end_index) == sentence.getTokenFromChar(end_index+1)))
		{
			penalty += 100.0;
		}

		// if we have overlap, figure out which value should get the token
		if (!results.empty() && last_end_char != -1 && 
		   (sentence.getTokenFromChar(char_index) == sentence.getTokenFromChar(last_end_char))) 
		{
			if (firstValueHasRightsToToken(sentence, last_end_char, char_index)) {
				while (char_index < sentence.getLength() - 1 && sentence.getTokenFromChar(char_index) == sentence.getTokenFromChar(last_end_char))
					char_index++;
			} else {
				while (last_end_char > 0 && sentence.getTokenFromChar(char_index) == sentence.getTokenFromChar(last_end_char))
					last_end_char--;
				results.back().end =  sentence.getTokenFromChar(last_end_char);
				// if the previous value is now empty, remove it.
				if (results.back().start > results.back().end)
					results.pop_back();
			}
		}

		int start_tok = sentence.getTokenFromChar(char_index);
		int end_tok = sentence.getTokenFromChar(end_index);

		last_end_char = end_index;
		char_index = end_index + 1;

		// throw out cases where overlap led to an empty span
		if (start_tok > end_tok) 
			continue;

		ValueSpan span(start_tok, end_tok, tagSet->getReducedTagSymbol(tag));
		results.push_back(span);		
	}

	return results;
}

bool ChineseValueRecognizer::firstValueHasRightsToToken(const PIdFCharSentence &sentence,
                                                 int first_token_char_index, 
                                                 int second_token_char_index) 
{
	int start_token_index = sentence.getTokenFromChar(first_token_char_index);
	for (int i = 0; i < sentence.getLength() - second_token_char_index; i++) {
		if (sentence.getTokenFromChar(second_token_char_index + i) != start_token_index) 
			return true;
		if (first_token_char_index - i < 0)
			return true;
		if (sentence.getTokenFromChar(first_token_char_index - i) != start_token_index) 
			return false;
	}
	return false;
}

void ChineseValueRecognizer::correctSentence(PIdFCharSentence &sent) {
	int URL_ST_tag = _tagSet->getTagIndex(Symbol(L"URL-ST"));
	int URL_CO_tag = _tagSet->getTagIndex(Symbol(L"URL-CO"));
	int PHONE_ST_tag = _tagSet->getTagIndex(Symbol(L"PHONE-ST"));
	int PHONE_CO_tag = _tagSet->getTagIndex(Symbol(L"PHONE-CO"));
	int PERCENT_ST_tag = _tagSet->getTagIndex(Symbol(L"PERCENT-ST"));
	int PERCENT_CO_tag = _tagSet->getTagIndex(Symbol(L"PERCENT-CO"));
	int MONEY_ST_tag = _tagSet->getTagIndex(Symbol(L"MONEY-ST"));
	int MONEY_CO_tag = _tagSet->getTagIndex(Symbol(L"MONEY-CO"));
	int TIMEX_ST_tag = _tagSet->getTagIndex(Symbol(L"TIMEX-ST"));
	int TIMEX_CO_tag = _tagSet->getTagIndex(Symbol(L"TIMEX-CO"));


	int length = sent.getLength();
	int start, end;

	// Merge any nearby URLs and phone numbers
	for (int i = 0; i < length; i++) {
		if (sent.getTag(i) == URL_ST_tag) {
			start = i;
			end = start;
			while (end + 1 < length && (isURLTag(sent, end + 1) || isNoneTag(sent, end + 1))) {
				Symbol ch = sent.getChar(end + 1);
				if (isNoneTag(sent, end + 1) && !WordConstants::isURLCharacter(ch))
					break;
				end++;
			}
			while (start > 0 && isNoneTag(sent, start - 1)) {
				Symbol ch = sent.getChar(start - 1);
				if (!WordConstants::isURLCharacter(ch))
					break;
				start--;
			}
			forceTag(sent, URL_ST_tag, URL_CO_tag, start, end); 
			i = end + 1;
		}
		else if (sent.getTag(i) == PHONE_ST_tag) {
			start = i;
			end = start;
			while (end + 1 < length && (isPhoneTag(sent, end + 1) || isNoneTag(sent, end + 1))) {
				Symbol ch = sent.getChar(end + 1);
				if (isNoneTag(sent, end + 1) && !WordConstants::isPhoneCharacter(ch))
					break;
				end++;
			}
			while (start > 0 && isNoneTag(sent, start - 1)) {
				Symbol ch = sent.getChar(start - 1);
				if (!WordConstants::isPhoneCharacter(ch))
					break;
				start--;
			}
			forceTag(sent, PHONE_ST_tag, PHONE_CO_tag, start, end); 
			i = end + 1;
		}
	}

	// attach numbers to nearby numerics and timexes
	for (int j = 0; j < length; j++) {
		Symbol ch = sent.getChar(j);
		if (WordConstants::isASCIINumericCharacter(ch) && isNoneTag(sent, j) &&
			((j - 1 > 0 && isPercentTag(sent, j - 1)) ||
			 (j + 1 < length && isPercentTag(sent, j + 1))))
		{
			forceTag(sent, PERCENT_ST_tag, PERCENT_CO_tag, j, j);
		}
		if (WordConstants::isASCIINumericCharacter(ch) && isNoneTag(sent, j) &&
			((j - 1 > 0 && isMoneyTag(sent, j - 1)) ||
			 (j + 1 < length && isMoneyTag(sent, j + 1))))
		{
			forceTag(sent, MONEY_ST_tag, MONEY_CO_tag, j, j);
		}
		if (WordConstants::isASCIINumericCharacter(ch) && isNoneTag(sent, j) &&
			((j - 1 > 0 && isTimexTag(sent, j - 1)) ||
			 (j + 1 < length && isTimexTag(sent, j + 1))))
		{
			forceTag(sent, TIMEX_ST_tag, TIMEX_CO_tag, j, j);
		}
	}

}

bool ChineseValueRecognizer::isNoneTag(PIdFCharSentence &sent, int i) {
	return
		(_tagSet->getReducedTagSymbol(sent.getTag(i)) == _tagSet->getNoneTag());
}

bool ChineseValueRecognizer::isURLTag(PIdFCharSentence &sent, int i) {
	return
		(_tagSet->getReducedTagSymbol(sent.getTag(i)) == Symbol(L"URL"));
}

bool ChineseValueRecognizer::isPhoneTag(PIdFCharSentence &sent, int i) {
	return
		(_tagSet->getReducedTagSymbol(sent.getTag(i)) == Symbol(L"PHONE"));
}

bool ChineseValueRecognizer::isEmailTag(PIdFCharSentence &sent, int i) {
	return 
		(_tagSet->getReducedTagSymbol(sent.getTag(i)) == Symbol(L"EMAIL"));
}

bool ChineseValueRecognizer::isPercentTag(PIdFCharSentence &sent, int i) {
	return 
		(_tagSet->getReducedTagSymbol(sent.getTag(i)) == Symbol(L"PERCENT"));
}

bool ChineseValueRecognizer::isMoneyTag(PIdFCharSentence &sent, int i) {
	return 
		(_tagSet->getReducedTagSymbol(sent.getTag(i)) == Symbol(L"MONEY"));
}

bool ChineseValueRecognizer::isTimexTag(PIdFCharSentence &sent, int i) {
	return 
		(_tagSet->getReducedTagSymbol(sent.getTag(i)) == Symbol(L"TIMEX"));
}

void ChineseValueRecognizer::forceTag(PIdFCharSentence &sent, int forced_ST_tag, int forced_CO_tag,
							   int start, int end) 
{
	int i;
	if (forced_ST_tag == -1 || forced_CO_tag == -1) return;

	if (_tagSet->getReducedTagSymbol(forced_ST_tag) != _tagSet->getReducedTagSymbol(forced_CO_tag))
		return;

	bool all_marked = true;
	// if all tokens are marked already, then skip
	for (i = start; i <= end; i++) {
		if (!(sent.getTag(i) == forced_ST_tag || sent.getTag(i) == forced_CO_tag))
		{
			all_marked = false;
			break;
		}
	}
	if (all_marked) return;

	// change them all to match tag
	for (i = start; i <= end; i++) {
		if (i - 1 >= 0 && 
    	   (sent.getTag(i - 1) == forced_ST_tag || sent.getTag(i - 1) == forced_CO_tag))
		{
			sent.setTag(i, forced_CO_tag);
		} else {
			sent.setTag(i, forced_ST_tag);
		}
	}

	// fix tag after forced expression
	if (end + 1 < sent.getLength()) {
		if (sent.getTag(end + 1) == forced_ST_tag)
			sent.setTag(end + 1, forced_CO_tag);
		if (sent.getTag(end + 1) != forced_CO_tag && 
			_tagSet->isCOTag(sent.getTag(end + 1))) 
		{
			Symbol reduced_sym = _tagSet->getReducedTagSymbol(sent.getTag(end + 1));
			for (int j = 0; j < _tagSet->getNTags(); j++) {
				if (_tagSet->getReducedTagSymbol(j) == reduced_sym && _tagSet->isSTTag(j)) {
					sent.setTag(end + 1, j);
					break;
				}
			}
		}
	}
}
