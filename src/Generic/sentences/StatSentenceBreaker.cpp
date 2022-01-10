// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/sentences/StatSentenceBreaker.h"
#include "Generic/sentences/StatSentModelInstance.h"
#include "Generic/sentences/StatSentBreakerTokens.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/StringTransliterator.h"
#include "Generic/common/limits.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Metadata.h"
#include "Generic/theories/Region.h"
#include "Generic/theories/Zone.h"
#include "Generic/theories/Span.h"
#include <boost/scoped_ptr.hpp>
#include "Generic/reader/DefaultDocumentReader.h"

class Metadata;

const Symbol StatSentenceBreaker::START_SENTENCE(L"ST");
const Symbol StatSentenceBreaker::CONT_SENTENCE(L"CO");

#define TOKEN_SPLIT_PUNCTUATION        L".[](){},\"'`?!:;$"

StatSentenceBreaker::StatSentenceBreaker() : _model(0), _curRegion(0)
{
	std::string model_file = ParamReader::getParam("stat_sent_breaker_model");
	if (!model_file.empty()) {
		boost::scoped_ptr<UTF8InputStream> modelIn_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& modelIn(*modelIn_scoped_ptr);
		modelIn.open(model_file.c_str());
		if (modelIn.fail()) {
			throw UnexpectedInputException("StatSentenceBreaker::StatSentenceBreaker()",
										   "Unable to open model file");
		}
		_defaultModel = _new StatSentBreakerFVecModel(modelIn);
		modelIn.close();
	}
	else {
		_defaultModel = 0;
	}

	_model = _defaultModel;

	if (_model == 0) {
		throw UnexpectedInputException("StatSentenceBreaker::StatSentenceBreaker()",
									   "No model file parameter specified.");
	}

	model_file = ParamReader::getParam("lowercase_stat_sent_breaker_model");
	if (!model_file.empty()) {
		boost::scoped_ptr<UTF8InputStream> modelIn_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& modelIn(*modelIn_scoped_ptr);
		modelIn.open(model_file.c_str());
		if (modelIn.fail()) {
			throw UnexpectedInputException("StatSentenceBreaker::StatSentenceBreaker()",
										   "Unable to open lowercase model file");
		}
		_lowerCaseModel = _new StatSentBreakerFVecModel(modelIn);
		modelIn.close();
	}
	else {
		_lowerCaseModel = 0;
	}

	model_file = ParamReader::getParam("uppercase_fvec_backoff_model_file");
	if (!model_file.empty()) {
		boost::scoped_ptr<UTF8InputStream> modelIn_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& modelIn(*modelIn_scoped_ptr);
		modelIn.open(model_file.c_str());
		if (modelIn.fail()) {
			throw UnexpectedInputException("StatSentenceBreaker::StatSentenceBreaker()",
										   "Unable to open uppercase model file");
		}
		_upperCaseModel = _new StatSentBreakerFVecModel(modelIn);
		modelIn.close();
	}
	else {
		_upperCaseModel = 0;
	}

	std::string sub_file = ParamReader::getRequiredParam("tokenizer_subst");
	StatSentBreakerTokens::initSubstitutionMap(sub_file.c_str());
}



StatSentenceBreaker::~StatSentenceBreaker() {
	delete _defaultModel;
	delete _lowerCaseModel;
	delete _upperCaseModel;
}

/**
 * Sets the current document pointer to point to the
 * document passed in and resets the current sentence
 * number to 0.
 */
void StatSentenceBreaker::resetForNewDocument(const Document *doc) {
	_curDoc = doc;
	_curRegion = 0;
	_cur_sent_no = 0;
	
	_model = _defaultModel;

	bool is_all_upper = true;
	bool is_all_lower = true;
	const Region* const* sents = _curDoc->getRegions();
	for (int j = 0; j < _curDoc->getNRegions(); j++) {
		_curRegion = sents[j];
		const LocatedString *sent = _curRegion->getString();
		for (int k = 0; k < sent->length(); k++) {
			if (iswupper(sent->charAt(k))) {
				is_all_lower = false;
				if (!is_all_upper)
					break;
			} else if (iswlower(sent->charAt(k))) {
				is_all_upper = false;
				if (!is_all_lower)
					break;
			}
		}
	}

	if (is_all_lower && _lowerCaseModel != 0) {
		SessionLogger::dbg("low_brkr_0") << "Using lowercase sentence breaker model\n";
		_model = _lowerCaseModel;
	}
	else if (is_all_upper && _upperCaseModel != 0) {
		SessionLogger::dbg("upp_brkr_0") << "Using uppercase sentence breaker model\n";
		_model = _upperCaseModel;
	}
}

/**
 * Takes an array of pointers to LocatedStrings corresponding to the regions of
 * the document text and the number of regions. It puts an array of pointers to
 * Sentences where specified by results arg, and returns its size.
 *
 * Note that it is assumed that every region boundary is also a sentence
 * boundary, that is, no single sentence can span two regions.
 *
 * The caller is responsible for deleting the array of Sentences and all the
 * Sentence objects stored in it.
 *
 * @param results an output parameter containing an array of
 *                pointers to Sentences that will be filled in
 *                with the sentences found by the sentence breaker.
 * @param max_sentences the size of the results arrays, i.e., the maximum number
 *                      of sentences that can broken up and placed in the array.
 * @param text an array of pointers to text regions from the input file.
 * @param num_regions the size of the text array, i.e., the number of text regions.
 * @return the number of sentences found and placed in the results array.
 * @throws UnexpectedInputException if more than max_sentences sentences are found.
 */
int StatSentenceBreaker::getSentencesRaw(Sentence **results,
									 int max_sentences,
									 const Region* const* regions,
									 int num_regions)
{
	if (num_regions < 1) {
		SessionLogger::warn_user("no_document_regions") << "No readable content in this document.";
		return 0;
	}

	StatSentBreakerTokens *regionTokens = _new StatSentBreakerTokens();

	for (int i = 0; i < num_regions; i++) {
		_curRegion = regions[i];

		LocatedString *region = _new LocatedString(*(_curRegion->getString()));
		regionTokens->init(region);

		if (regionTokens->getLength() < 3) {
			results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, region);
			_cur_sent_no++;
		}
		else {
			Symbol word, word1, word2;
			int start = regionTokens->getTokenStart(0);

			int sent_len = 2;
			word1 = regionTokens->getWord(0);
			word = regionTokens->getWord(1);

			for (int i = 2; i < regionTokens->getLength(); i++) {
				word2 = word1;
				word1 = word;
				word = regionTokens->getWord(i);

				double score = getSTScore(word, word1, word2, sent_len);

				if (score > 0) {
					if (sent_len > MAX_SENTENCE_TOKENS) {
						breakLongSentence(results, region, regionTokens, i - sent_len, i);
					}
					else {
						LocatedString *sentenceString = region->substring(start, regionTokens->getTokenEnd(i-1));
						results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, sentenceString);
						delete sentenceString;
						_cur_sent_no++;
					}
					start = regionTokens->getTokenEnd(i-1);
					sent_len = 0;
				}
				else {
					sent_len++;
				}
			}
			// handle last sentence
			LocatedString *sentenceString = region->substring(start, regionTokens->getTokenEnd(regionTokens->getLength() - 1));
			results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, sentenceString);
			delete sentenceString;
			_cur_sent_no++;
		}

		delete region;
	}
	delete regionTokens;
	return _cur_sent_no;
}

/*int StatSentenceBreaker::getSentencesRaw(Sentence **results,
									 int max_sentences,
									 const Zone* const* zones,
									 int num_zones)
{
	if (num_zones < 1) {
		SessionLogger::warn_user("no_document_regions") << "No readable content in this document.";
		return 0;
	}

	StatSentBreakerTokens *zoneTokens = _new StatSentBreakerTokens();

	for (int i = 0; i < num_zones; i++) {
		_curZone = zones[i];

		LocatedString *zone = _new LocatedString(*(_curZone->getString()));
		zoneTokens->init(zone);

		if (zoneTokens->getLength() < 3) {
			results[_cur_sent_no] = _new Sentence(_curDoc, _curZone, _cur_sent_no, zone);
			_cur_sent_no++;
		}
		else {
			Symbol word, word1, word2;
			int start = zoneTokens->getTokenStart(0);

			int sent_len = 2;
			word1 = zoneTokens->getWord(0);
			word = zoneTokens->getWord(1);

			for (int i = 2; i < zoneTokens->getLength(); i++) {
				word2 = word1;
				word1 = word;
				word = zoneTokens->getWord(i);

				double score = getSTScore(word, word1, word2, sent_len);

				if (score > 0) {
					if (sent_len > MAX_SENTENCE_TOKENS) {
						breakLongSentence(results, zone, zoneTokens, i - sent_len, i);
					}
					else {
						LocatedString *sentenceString = zone->substring(start, zoneTokens->getTokenEnd(i-1));
						results[_cur_sent_no] = _new Sentence(_curDoc, _curZone, _cur_sent_no, sentenceString);
						delete sentenceString;
						_cur_sent_no++;
					}
					start = zoneTokens->getTokenEnd(i-1);
					sent_len = 0;
				}
				else {
					sent_len++;
				}
			}
			// handle last sentence
			LocatedString *sentenceString = zone->substring(start, zoneTokens->getTokenEnd(zoneTokens->getLength() - 1));
			results[_cur_sent_no] = _new Sentence(_curDoc, _curZone, _cur_sent_no, sentenceString);
			delete sentenceString;
			_cur_sent_no++;
		}

		delete zone;
	}
	delete zoneTokens;
	return _cur_sent_no;
}*/

void StatSentenceBreaker::breakLongSentence(Sentence **results,
											const LocatedString *string,
											StatSentBreakerTokens *tokens,
											int start, int end)
{
	SessionLogger::warn("break_long_sentence") << "Breaking long sentence from " << start << " to " << end << ".\n";

	// we don't want to break right near the ends (this also assures that the 2 seed tokens precede my_start)
	int my_start = start + 5;
	int my_end = end - 5;

	Symbol word, word1, word2;

	double best_score = -10000;
	int best_sent = -1;
	word1 = tokens->getWord(my_start-2);
	word = tokens->getWord(my_start-1);
	for (int i = my_start; i < my_end; i++) {
		word2 = word1;
		word1 = word;
		word = tokens->getWord(i);

		double score = getSTScore(word, word1, word2, i - start);

		if (score > best_score) {
			best_sent = i;
			best_score = score;
		}
	}

	if (best_sent != -1) { // surely this is the case
		// if we still have really long sentences, we must recurse
		if (best_sent - start > MAX_SENTENCE_TOKENS) {
			breakLongSentence(results, string, tokens, start, best_sent);
		}
		else {
			LocatedString *sentenceString = string->substring(tokens->getTokenStart(start), tokens->getTokenEnd(best_sent-1));
			results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, sentenceString);
			delete sentenceString;
			_cur_sent_no++;
		}
		if (end - best_sent > MAX_SENTENCE_TOKENS) {
			breakLongSentence(results, string, tokens, best_sent, end);
		}
		else {
			LocatedString *sentenceString = string->substring(tokens->getTokenEnd(best_sent-1) + 1, tokens->getTokenEnd(end-1));
			results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, sentenceString);
			delete sentenceString;
			_cur_sent_no++;
		}
	}
}



double StatSentenceBreaker::getSTScore(Symbol word, Symbol word1, Symbol word2,
								  int sent_len)
{
	double p_st, p_co;
	StatSentModelInstance stInstance(START_SENTENCE, word, word1, word2, 0);
	StatSentModelInstance coInstance(CONT_SENTENCE, word, word1, word2, 0);

	p_st = _model->getProbability(&stInstance);
	p_co = _model->getProbability(&coInstance);

	return p_st - p_co;
}
