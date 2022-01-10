// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/theories/Token.h"
#include "Generic/partOfSpeech/discmodel/PPartOfSpeechSentence.h"

#include "English/partOfSpeech/en_PartOfSpeechRecognizer.h"
#include "Generic/partOfSpeech/DefaultPartOfSpeechRecognizer.h"
#include "Generic/partOfSpeech/discmodel/PPartOfSpeechRecognizer.h"
#include "Generic/partOfSpeech/discmodel/PPartOfSpeechModel.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/parse/PartOfSpeechTable.h"
#include "English/parse/en_WordFeatures.h"
#include <boost/scoped_ptr.hpp>

EnglishPartOfSpeechRecognizer::EnglishPartOfSpeechRecognizer()
	: _partOfSpeechRecognizer(0), 
	_wordFeatures(WordFeatures::build())
{
	_modelType = ParamReader::getParam("part_of_speech_model_type");
	if (_modelType == "DISC") {
		_partOfSpeechRecognizer = _new PPartOfSpeechRecognizer();
	} else if (_modelType == "SIMPLE") {
		string posFile = ParamReader::getRequiredParam("ordered_part_of_speech_table_file");
		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(posFile.c_str()));
		UTF8InputStream& in(*in_scoped_ptr);
		_posTable = _new PartOfSpeechTable(in);
		in.close();
	} else if (_modelType != "NONE" && !_modelType.empty()) {
		string errMessage = string("Parameter 'part_of_speech_model_type' must be set to 'DISC', 'SIMPLE' or 'NONE'");
		throw UnexpectedInputException("EnglishPartOfSpeechRecognizer::EnglishPartOfSpeechRecognizer()", errMessage.c_str());
	} else {
		_partOfSpeechRecognizer = _new DefaultPartOfSpeechRecognizer();
	}
}

EnglishPartOfSpeechRecognizer::~EnglishPartOfSpeechRecognizer() {
	delete _partOfSpeechRecognizer;
}


int EnglishPartOfSpeechRecognizer::getPartOfSpeechTheories(PartOfSpeechSequence **results, int max_theories,
		TokenSequence *tokenSequence) 
{
	if (_modelType == "SIMPLE") {
		int length = tokenSequence->getNTokens();
		PartOfSpeechSequence *partOfSpeechSequence = _new PartOfSpeechSequence(tokenSequence);

		for (int i = 0; i < length; i++) {
			const Token *tok = tokenSequence->getToken(i);
			int num_tags;
			const Symbol *tags = _posTable->lookup(tok->getSymbol(), num_tags);
			
			if (num_tags == 0) {
			  bool firstWord = (i == 0);
			  Symbol features = _wordFeatures->features(tok->getSymbol(), firstWord);
			  tags = _posTable->lookup(features, num_tags);
			  
			}

			if (num_tags > 0) {
				partOfSpeechSequence->addPOS(tags[0], 1, i);
			} else {
			  partOfSpeechSequence->addPOS(Symbol(L"DummyPOS"), 1, i); 
			}
		}
		results[0] = partOfSpeechSequence;
		return 1;
	}

	return _partOfSpeechRecognizer->getPartOfSpeechTheories(results, max_theories, tokenSequence);
}

