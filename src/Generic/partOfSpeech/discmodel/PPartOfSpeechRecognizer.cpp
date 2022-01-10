// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/partOfSpeech/discmodel/PPartOfSpeechSentence.h"

#include "Generic/partOfSpeech/discmodel/PPartOfSpeechRecognizer.h"
#include "Generic/partOfSpeech/discmodel/PPartOfSpeechModel.h"
#include "Generic/discTagger/DTTagSet.h"

PPartOfSpeechRecognizer::PPartOfSpeechRecognizer()
	: _partOfSpeechModel(0), _tagSet(0)
{
	string model_mode = ParamReader::getRequiredParam("ppartofspeech_trainer_standalone_mode");
	if (model_mode == "train")
		_partOfSpeechModel = _new PPartOfSpeechModel(PPartOfSpeechModel::TRAIN);
	else if (model_mode == "decode")
		_partOfSpeechModel = _new PPartOfSpeechModel(PPartOfSpeechModel::DECODE);
	else
		throw UnexpectedInputException("PPartOfSpeechRecognizer::PPartOfSpeechRecognizer()","Parameter 'ppartofspeech_trainer_standalone_mode' must be 'decode' or 'train'");
	_tagSet = _partOfSpeechModel->getTagSet();
}

PPartOfSpeechRecognizer::~PPartOfSpeechRecognizer() {
	delete _partOfSpeechModel;
}

void PPartOfSpeechRecognizer::resetForNewSentence(){
};

int PPartOfSpeechRecognizer::getPartOfSpeechTheories(PartOfSpeechSequence **results, int max_theories, 
												TokenSequence* tokenSequence)
{
	PPartOfSpeechSentence sentence(_tagSet, *tokenSequence);
	_partOfSpeechModel->decode(sentence);
	
	results[0] = makePartOfSpeechSequence(*tokenSequence, sentence);

	return 1;
}

PartOfSpeechSequence *PPartOfSpeechRecognizer::makePartOfSpeechSequence(const TokenSequence& tokenSequence, const PPartOfSpeechSentence& sentence) {
	int length = sentence.getLength();
	PartOfSpeechSequence *partOfSpeechSequence = _new PartOfSpeechSequence(&tokenSequence);

	for (int i = 0; i < length; i++) {
		partOfSpeechSequence->addPOS(_tagSet->getTagSymbol(sentence.getTag(i)), 1, i);
	}

	return partOfSpeechSequence;
}
