// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <boost/foreach.hpp>

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/Sexp.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/names/IdFWordFeatures.h"
#include "Generic/names/discmodel/PIdFModel.h"
#include "Generic/names/discmodel/PIdFSentence.h"
#include "Generic/names/IdFSentenceTheory.h"
#include "Generic/names/NameClassTags.h"
#include "English/values/en_ValueRecognizer.h"
#include "Generic/values/PatternValueFinder.h"
#include "English/common/en_WordConstants.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/ValueMentionSet.h"
#include "English/values/en_IdFValueRecognizer.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif

using namespace std;

Symbol EnglishValueRecognizer::TIME_TAG = Symbol(L"TIME");
Symbol EnglishValueRecognizer::TIME_TAG_ST = Symbol(L"TIME-ST");

EnglishValueRecognizer::EnglishValueRecognizer() : _sent_no(0), DO_VALUES(true), _docTheory(0), _timexWords(0), _patternValueFinder(0)
{

	DO_VALUES = ParamReader::getOptionalTrueFalseParamWithDefaultVal("do_values_stage", true);
	if (!DO_VALUES) return;

	for (int i = 0; i < N_DECODERS; i++) {
		_valuesDecoders[i] = 0;
		_idfValuesDecoders[i] = 0;
	}

	std::string valueFinderParam = ParamReader::getParam("value_finder_type");
	if (valueFinderParam.compare("idf") == 0) {
		_value_finder = IDF_VALUE_FINDER;
	}
	else  { //if (valueFinderParam.compare("pidf") == 0) {
		_value_finder = PIDF_VALUE_FINDER;
	}

	// parameters
	TIMEX_DECODER_INDEX = -1;

	char tagset_param[100];
	char featureset_param[100];
	char modelfile_param[100];
	char lc_modelfile_param[100];
	char vocabfile_param[100];
	char lc_vocabfile_param[100];

	for (int i = 0; i < N_DECODERS; i++) {
		
		if (_value_finder == IDF_VALUE_FINDER) {
			sprintf(tagset_param, "idf_values_%d_tag_set", i+1);
			sprintf(modelfile_param, "idf_values_%d_model", i+1);
			sprintf(lc_modelfile_param, "idf_lc_values_%d_model", i+1);
		
			// backwards compatibility hack
			bool force_timex = false;
			if (i == 0 && !ParamReader::hasParam(modelfile_param)) {
				force_timex = true;
				sprintf(tagset_param, "idf_timex_tag_set_file");
				sprintf(modelfile_param, "idf_timex_model_file");
				sprintf(lc_modelfile_param, "idf_lowercase_timex_model_file");
			} else if (i == 1 && !ParamReader::hasParam(modelfile_param)) {
				sprintf(tagset_param, "idf_other_value_tag_set_file");
				sprintf(modelfile_param, "idf_other_value_model_file");
				sprintf(lc_modelfile_param, "idf_lowercase_other_value_model_file");
			} else if (i > 1 && !ParamReader::hasParam(modelfile_param)) {
				// special values not used for IDF
				_idfValuesDecoders[i] = 0;
				continue;
			}

			std::string model_file = ParamReader::getRequiredParam(modelfile_param);
			std::string tag_set_file = ParamReader::getRequiredParam(tagset_param);

			std::string lc_model_file = ParamReader::getParam(lc_modelfile_param);
			if (!lc_model_file.empty()) {
				_idfValuesDecoders[i] = _new IdFValueRecognizer(tag_set_file.c_str(), model_file.c_str(), lc_model_file.c_str());
			} else {
				_idfValuesDecoders[i] = _new IdFValueRecognizer(tag_set_file.c_str(), model_file.c_str());
			}

			if (force_timex || _idfValuesDecoders[i]->getTagSet()->getIndexForTag(TIME_TAG_ST) != -1) {
				TIMEX_DECODER_INDEX = i;
				_idfValuesDecoders[i]->setWordFeaturesMode(IdFWordFeatures::TIMEX);
			} else {
				_idfValuesDecoders[i]->setWordFeaturesMode(IdFWordFeatures::OTHER_VALUE);		
			}
		} 
		else if (_value_finder == PIDF_VALUE_FINDER) {
			sprintf(tagset_param, "values_%d_tag_set", i+1);
			sprintf(featureset_param, "values_%d_feature_set", i+1);
			sprintf(modelfile_param, "values_%d_model", i+1);
			sprintf(lc_modelfile_param, "lc_values_%d_model", i+1);
			sprintf(vocabfile_param, "values_%d_vocab", i+1);
			sprintf(lc_vocabfile_param, "lc_values_%d_vocab", i+1);

			// backwards compatibility hack
			bool force_timex = false;
			if (i == 0 && !ParamReader::hasParam(modelfile_param)) {
				force_timex = true;
				sprintf(tagset_param, "timex_tag_set_file");
				sprintf(featureset_param, "timex_features_file");
				sprintf(modelfile_param, "timex_model_file");
				sprintf(lc_modelfile_param, "lowercase_timex_model_file");
				sprintf(vocabfile_param, "timex_vocab_file");
				sprintf(lc_vocabfile_param, "lowercase_timex_vocab_file");
			} else if (i == 1 && !ParamReader::hasParam(modelfile_param)) {
				sprintf(tagset_param, "other_value_tag_set_file");
				sprintf(featureset_param, "other_value_features_file");
				sprintf(modelfile_param, "other_value_model_file");
				sprintf(lc_modelfile_param, "lowercase_other_value_model_file");
				sprintf(vocabfile_param, "other_value_vocab_file");
				sprintf(lc_vocabfile_param, "lowercase_other_value_vocab_file");
			} else if (i == 2 && !ParamReader::hasParam(modelfile_param)) {
				sprintf(tagset_param, "special_value_tag_set_file");
				sprintf(featureset_param, "special_value_features_file");
				sprintf(modelfile_param, "special_value_model_file");
				sprintf(lc_modelfile_param, "lowercase_special_value_model_file");
				sprintf(vocabfile_param, "special_value_vocab_file");
				sprintf(lc_vocabfile_param, "lowercase_special_value_vocab_file");
			}

			std::string model_file = ParamReader::getParam(modelfile_param);			
			if (!model_file.empty()) {
				std::string tag_set_file = ParamReader::getRequiredParam(tagset_param);
				std::string features_file = ParamReader::getRequiredParam(featureset_param);
				std::string vocab = ParamReader::getParam(vocabfile_param);
				const char *vocab_char = vocab.c_str();

				std::string lc_model_file = ParamReader::getParam(lc_modelfile_param);
				if (!lc_model_file.empty()) {
					std::string lc_vocab = ParamReader::getParam(lc_vocabfile_param);
					const char *lc_vocab_char = 0;
					if (!lc_vocab.empty())
						lc_vocab_char = lc_vocab.c_str();
					_valuesDecoders[i] = _new PIdFModel(PIdFModel::DECODE, tag_set_file.c_str(),
							features_file.c_str(), model_file.c_str(), vocab_char, lc_model_file.c_str(), lc_vocab_char);
					
				} else {
					_valuesDecoders[i] = 
						_new PIdFModel(PIdFModel::DECODE, tag_set_file.c_str(), features_file.c_str(), model_file.c_str(), vocab_char);
				}

				if (force_timex || _valuesDecoders[i]->getTagSet()->getTagIndex(TIME_TAG_ST) != -1) {
					TIMEX_DECODER_INDEX = i;
					_valuesDecoders[i]->setWordFeaturesMode(IdFWordFeatures::TIMEX);
				} else {
					_valuesDecoders[i]->setWordFeaturesMode(IdFWordFeatures::OTHER_VALUE);			
				}
			} 
		}
	}

	std::string timex_indicator_words = ParamReader::getRequiredParam("timex_indicator_words");
	_timexWords = _new SymbolHash(timex_indicator_words.c_str());

	if (ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_value_finding_patterns", false))
		_patternValueFinder = _new PatternValueFinder();
}

EnglishValueRecognizer::~EnglishValueRecognizer() {
	if (DO_VALUES) {
		delete _timexWords;

		for (int i = 0; i < N_DECODERS; i++) {
			if (_valuesDecoders[i]) {
				delete _valuesDecoders[i];
			}
			if (_idfValuesDecoders[i]) {
				delete _idfValuesDecoders[i];
			}
		}
	}
	delete _patternValueFinder;
}

void EnglishValueRecognizer::resetForNewDocument(DocTheory *docTheory) {
	if (!DO_VALUES) return;
	
	for (int i = 0; i < N_DECODERS; i++) {
		if (_value_finder == IDF_VALUE_FINDER) {
			if (_idfValuesDecoders[i]) {
				_idfValuesDecoders[i]->resetForNewDocument(docTheory);
			}
		} else { // (_value_finder == PIDF_VALUE_FINDER
			if (_valuesDecoders[i]) {
				_valuesDecoders[i]->resetForNewDocument(docTheory);
			}
		}
	}

	if (_patternValueFinder)
		_patternValueFinder->resetForNewDocument(docTheory);

	_docTheory = docTheory;
}

int EnglishValueRecognizer::getValueTheories(ValueMentionSet **results, int max_theories, 
									  TokenSequence *tokenSequence)
{
	if (!DO_VALUES) {
		results[0] = _new ValueMentionSet(tokenSequence, 0);
		return 1;
	}

	_sent_no = tokenSequence->getSentenceNumber();

	// make the contents of POSTDATE regions automatically into dates
	if (_docTheory != NULL) {
		EDTOffset sent_offset = _docTheory->getSentence(_sent_no)->getStartEDTOffset();
		if (TIMEX_DECODER_INDEX != -1 &&
			_docTheory->getMetadata() != 0 &&
			_docTheory->getMetadata()->getCoveringSpan(sent_offset, POSTDATE_SYM) != 0 &&
			tokenSequence->getNTokens() > 0) {
			try {
				ValueMentionUID uid = ValueMention::makeUID(_sent_no, 0);
				ValueMention *vm = _new ValueMention(_sent_no, uid, 0, tokenSequence->getNTokens() - 1, TIME_TAG);
				results[0] = _new ValueMentionSet(tokenSequence, 1);
				results[0]->takeValueMention(0, vm);
				return 1;
			} catch (InternalInconsistencyException &e) {
				// Should never happen unless max # of value mentions is strangely set to 0
				SessionLogger::err("value_mention_uid") << e;
			}
		}
	}

	SpanList valueSpans;
	if (_value_finder == IDF_VALUE_FINDER) {
		valueSpans = getIdFValues(tokenSequence);
	}
	else { //if (_value_finder == PIDF_VALUE_FINDER)
		valueSpans = getPIdFValues(tokenSequence);
	}

	SpanList ruleValueSpans = identifyRuleRepositoryValues(tokenSequence);
	valueSpans.insert(valueSpans.end(), ruleValueSpans.begin(), ruleValueSpans.end());

	if (_patternValueFinder) {
		SpanList patternValueSpans = _patternValueFinder->findValueMentions(_sent_no);
		valueSpans.insert(valueSpans.end(), patternValueSpans.begin(), patternValueSpans.end());
	}

	results[0] = createValueMentionSet(tokenSequence, valueSpans);
	return 1;
}

ValueRecognizer::SpanList EnglishValueRecognizer::getIdFValues(TokenSequence *tokenSequence) {
	SpanList results;

	IdFSentenceTheory *valueSentences[N_DECODERS];
	for (int i = 0; i < N_DECODERS; i++) {
		if (_idfValuesDecoders[i] != 0) {
			_idfValuesDecoders[i]->getValueTheories(&valueSentences[i], 1, tokenSequence);
		} else {
			valueSentences[i] = 0;
		}
	}

	for (int j = 0; j < tokenSequence->getNTokens(); j++) {
		for (int k = 0; k < N_DECODERS; k++) {			
			if (_idfValuesDecoders[k] != 0 &&
				valueSentences[k]->getTag(j) != _idfValuesDecoders[k]->getTagSet()->getIndexForTag(NONE_ST) &&
				_idfValuesDecoders[k]->getTagSet()->isStart(valueSentences[k]->getTag(j)))
			{
				int tag = valueSentences[k]->getTag(j);
				int start = j;
				int end = start;
				while (end+1 < valueSentences[k]->getLength() &&
					_idfValuesDecoders[k]->getTagSet()->isContinue(valueSentences[k]->getTag(end+1)))
					end++;

				ValueSpan span;
				span.start = start;
				span.end = end;
				span.tag = _idfValuesDecoders[k]->getTagSet()->getReducedTagSymbol(tag);
				results.push_back(span);

				j = end;
				break;
			} 
		}
	}

	for (int i = 0; i < N_DECODERS; i++) {
		delete valueSentences[i];
	}

	return results;
}

ValueRecognizer::SpanList EnglishValueRecognizer::getPIdFValues(TokenSequence *tokenSequence) {
	SpanList results;

	PIdFSentence *valueSentences[N_DECODERS];
	int tokenSequenceSize = tokenSequence->getNTokens();
	for (int i = 0; i < N_DECODERS; i++) {
		if (_valuesDecoders[i] != 0) {
			valueSentences[i] = _new PIdFSentence(_valuesDecoders[i]->getTagSet(), *tokenSequence);
			_valuesDecoders[i]->decode(*valueSentences[i]);
			if (i == TIMEX_DECODER_INDEX)
				correctTimexSentence(*valueSentences[i]);
		} else {
			valueSentences[i] = 0;
		}
	}

	int j;
	int jstart;
	int jend;
	for (j = 0; j < tokenSequenceSize; j++) {
		for (int k = 0; k < N_DECODERS; k++) {			
			if (_valuesDecoders[k] != 0 &&
				valueSentences[k]->getTag(j) != _valuesDecoders[k]->getTagSet()->getTagIndex(NONE_ST) &&
				_valuesDecoders[k]->getTagSet()->isSTTag(valueSentences[k]->getTag(j)))
			{
				int tag = valueSentences[k]->getTag(j);
				jstart = j;
				jend = jstart;
				while (jend+1 < valueSentences[k]->getLength() &&
					_valuesDecoders[k]->getTagSet()->isCOTag(valueSentences[k]->getTag(jend+1)))
					jend++;

				ValueSpan span;
				span.start = jstart;
				span.end = jend;
				span.tag = _valuesDecoders[k]->getTagSet()->getReducedTagSymbol(tag);
				results.push_back(span);

				j = jend;
				break;
			} 
		}
	}

	for (int i = 0; i < N_DECODERS; i++) {
		delete valueSentences[i];
	}

	return results;
}

void EnglishValueRecognizer::correctTimexSentence(PIdFSentence &sent)
{
	int length = sent.getLength();
	int start, end, i;

	// general complex times
	for (i = 0; i < length; i++) {
		Symbol word = sent.getWord(i);
		//cout << "Checking on " << word.to_debug_string() << "\n";
		if (WordConstants::isTimeExpression(word) ||
			WordConstants::isDashedDuration(word) ||
			WordConstants::isDecade(word) ||
			WordConstants::isDateExpression(word))
		{
			//cout << "is time expression\n";
			start = i;
			if (i + 1 < length && WordConstants::isTimeModifier(sent.getWord(i + 1))) 
				end = i + 1;
			else 
				end = start;
			forceTimex(sent, start, end);
		}
	}

	// commonly missed postfixes
	bool previous = false;
	for (i = 0; i < length; i++) {
		Symbol word = sent.getWord(i);
		
		if (previous) {
			if (word == EnglishWordConstants::OLD) {
				forceTimex(sent, i, i);
			}
		}
		
		if (!isTimexNone(sent, i)) 
			previous = true;
		else 
			previous = false;
	}

	// attach years to previous timexes
	for (i = 0; i < length; i++) {
		Symbol word = sent.getWord(i);
		
		if (WordConstants::isFourDigitYear(word)) {
			forceTimex(sent, i, i);

			if (i - 2 > 0 &&
				!isTimexNone(sent, i - 2) &&
				sent.getWord(i - 1) == EnglishWordConstants::_COMMA_)
			{
				forceTimex(sent, i - 1, i);
			}
		}
	}

/*	// attach numbers to nearby timexes
	for (i = 0; i < length; i++) {
		Symbol word = sent.getWord(i);
		if (WordConstants::isNumeric(word) && isTimexNone(sent, i) &&
			(i - 1 > 0 && !isTimexNone(sent, i - 1) ||
			 i + 1 < length && !isTimexNone(sent, i + 1))
			) 
		{
			forceTimex(sent, i, i);
		}
	}
*/
	// single words on list
	for (i = 0; i < length; i++) {
		Symbol word = sent.getWord(i);
		if (_timexWords->lookup(word))
			forceTimex(sent, i, i);
	}
}

bool EnglishValueRecognizer::isTimexNone(PIdFSentence &sent, int i)
{
	if (TIMEX_DECODER_INDEX == -1)
		return false;

	DTTagSet *tagset = _valuesDecoders[TIMEX_DECODER_INDEX]->getTagSet();
	return (tagset->getReducedTagSymbol(sent.getTag(i)) == tagset->getNoneTag());
}

void EnglishValueRecognizer::forceTimex(PIdFSentence &sent, int start, int end)
{
	if (TIMEX_DECODER_INDEX == -1)
		return;

	DTTagSet *tagset = _valuesDecoders[TIMEX_DECODER_INDEX]->getTagSet();

	int TIMEX_ST_timex_tag = tagset->getTagIndex(Symbol(L"TIME-ST"));
	int TIMEX_CO_timex_tag = tagset->getTagIndex(Symbol(L"TIME-CO"));
	int i;

	if (TIMEX_ST_timex_tag == -1 || TIMEX_CO_timex_tag == -1) return;

	bool all_timex = true;
	// if all tokens are times already, then skip
	for (i = start; i <= end; i++) {
		if (isTimexNone(sent, i))
		{
			all_timex = false;
			break;
		}
	}
	if (all_timex) return;

	// change them all the timexes
	for (i = start; i <= end; i++) {
		if (i - 1 >= 0 && 
			!isTimexNone(sent, i - 1)) 
		{
			sent.setTag(i, TIMEX_CO_timex_tag);
		} else {
			sent.setTag(i, TIMEX_ST_timex_tag);
		}
	}

	// fix tag after timex expression
	if (end + 1 < sent.getLength()) {
		if (sent.getTag(end + 1) == TIMEX_ST_timex_tag)
			sent.setTag(end + 1, TIMEX_CO_timex_tag);
		if (sent.getTag(end + 1) == tagset->getTagIndex(tagset->getNoneCOTag()))
			sent.setTag(end + 1, tagset->getTagIndex(tagset->getNoneSTTag()));
	}
}
