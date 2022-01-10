// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/ParamReader.h"
#include "Chinese/names/discmodel/ch_PIdFNameRecognizer.h"
#include "Chinese/names/ch_NameRecognizer.h"

#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UnrecoverableException.h"

using namespace std;

PIdFNameRecognizer::PIdFNameRecognizer() {

	std::string pidf_mode = ParamReader::getParam("pidf_mode");
	if (pidf_mode == "tokens")
		_run_on_tokens = true;
	else
		_run_on_tokens = false;

	if (_run_on_tokens) {
		_pidfDecoder = _new PIdFModel(PIdFModel::DECODE);
	}
	else {
		_pidfCharDecoder = _new PIdFCharModel();
	}
}

void PIdFNameRecognizer::resetForNewSentence(const Sentence *sentence) {
}

int PIdFNameRecognizer::getNameTheories(NameTheory **results, int max_theories,
									TokenSequence *tokenSequence)
{

	int num_theories;

	if (_run_on_tokens) {
		num_theories = _pidfDecoder->getNameTheories(results, max_theories, tokenSequence);

		/*for (int theorynum = 0; theorynum < num_theories; theorynum++) {
			results[theorynum]->score = (float)results[theorynum]->getNNameSpans();
		}*/
	}
	else {
		num_theories = _pidfCharDecoder->getNameTheories(results, max_theories, tokenSequence);
	}

	return num_theories;
}
