// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/NameTheory.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/names/IdFSentenceTokens.h"
#include "Generic/names/IdFDecoder.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/WordConstants.h"
#include "Generic/names/NameClassTags.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/ValueType.h"
#include "Generic/names/IdFSentence.h"
#include "English/values/en_IdFValueRecognizer.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif

using namespace std;


IdFValueRecognizer::IdFValueRecognizer(const char *value_class_file,
						const char *model_file_prefix, const char *lc_model_file_prefix) 
						: _num_values(0), _valueClassTags(0)
{
	DEBUG = false;
	std::string debug_buffer = ParamReader::getParam("values_debug");
	if (!debug_buffer.empty()) {
		_debugStream.open(debug_buffer.c_str());
		DEBUG = true;
	}

	_valueClassTags = _new NameClassTags(value_class_file);
	_wordFeatures = IdFWordFeatures::build();
	_defaultDecoder = _new IdFDecoder(model_file_prefix, _valueClassTags, _wordFeatures);

	_decoder = _defaultDecoder;

	if (lc_model_file_prefix != 0) {
		_lowerCaseDecoder = _new IdFDecoder(lc_model_file_prefix, _valueClassTags, _wordFeatures);
	} else _lowerCaseDecoder = 0;

	_sentenceTokens = _new IdFSentenceTokens();
}

IdFValueRecognizer::~IdFValueRecognizer() {
	delete _valueClassTags;
	delete _wordFeatures;
	delete _defaultDecoder;
	delete _lowerCaseDecoder;
	delete _sentenceTokens;
}

void IdFValueRecognizer::resetForNewSentence() {
}

void IdFValueRecognizer::cleanUpAfterDocument() {
	// heap-allocated word lengths need cleanup.
	// everything else can persist, with just the counter reset
	int i;
	for (i=0; i < _num_values; i++)
		delete [] _valueWords[i].words;
	_num_values = 0;
}

void IdFValueRecognizer::resetForNewDocument(DocTheory *docTheory) {
	_decoder = _defaultDecoder;
	if (docTheory != 0) {
		int doc_case = docTheory->getDocumentCase();
		if (doc_case == DocTheory::LOWER && _lowerCaseDecoder != 0) {
			_decoder = _lowerCaseDecoder;
		}
	}
}

int IdFValueRecognizer::getValueTheories(IdFSentenceTheory **results, int max_theories,
									   TokenSequence *tokenSequence)
{
	int n_tokens = tokenSequence->getNTokens();

	for (int i = 0; i < n_tokens; i++) {
		_sentenceTokens->setWord(i, tokenSequence->getToken(i)->getSymbol());
	}
	_sentenceTokens->setLength(n_tokens);

	int num_theories = _decoder->decodeSentenceNBest(_sentenceTokens,
				results, max_theories);

	if (num_theories == 0) {
		// should never happen, but...
		results[0] = new IdFSentenceTheory(_sentenceTokens->getLength(),
			_valueClassTags->getNoneStartTagIndex());
		num_theories = 1;
	}

	if (DEBUG)  {
//		_decoder->printTrellis(_sentenceTokens, _debugStream);
		_debugStream << _valueClassTags->to_string(_sentenceTokens, results[0]);
		_debugStream << L"\n";
		_debugStream << _valueClassTags->to_enamex_sgml_string(_sentenceTokens, results[0]);
		_debugStream << L"\n\n";
	}

	return num_theories;
}
