// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/names/IdFSentence.h"
#include "Generic/names/NameClassTags.h"
#include "Generic/ASR/sentBreaker/ASRSentBreakerFVecModel.h"
#include "Generic/ASR/sentBreaker/ASRSentBreakerTrainer.h"
#include <boost/scoped_ptr.hpp>


using namespace std;


const Symbol ASRSentBreakerTrainer::START_SENTENCE(L"ST");
const Symbol ASRSentBreakerTrainer::CONT_SENTENCE(L"CO");

ASRSentBreakerTrainer::ASRSentBreakerTrainer() {
	_training_file = ParamReader::getRequiredParam("training_file");
	_model_file = ParamReader::getRequiredParam("model_file");
	_nameClassTags = _new NameClassTags();
	_model = _new ASRSentBreakerFVecModel();
}

void ASRSentBreakerTrainer::train() {
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(_training_file.c_str());
	if (in.fail()) {
		throw UnexpectedInputException(
			"ASRSentBreakerTrainer::train()",
			"Unable to open training file");
	}

	UTF8OutputStream out;
	out.open(_model_file.c_str());
	if (out.fail()) {
		throw UnexpectedInputException(
			"ASRSentBreakerTrainer::train()",
			"Unable to create model file");
	}

	UTF8OutputStream outUnderived;
	std::string underived_model_file = _model_file + ".underived";
	outUnderived.open(underived_model_file.c_str());
	if (outUnderived.fail()) {
		throw UnexpectedInputException(
			"ASRSentBreakerTrainer::train()",
			"Unable to create underived model file");
	}

	IdFSentence currSent(_nameClassTags);
	IdFSentence nextSent(_nameClassTags);
	currSent.readTrainingSentence(in);

	int sent_no = 0;

	for (;;) {
		try {
			if (!nextSent.readTrainingSentence(in))
				break;
		}
		catch (UnrecoverableException &) {
			SessionLogger::err("SERIF") << "Sentence #" << sent_no+1 << "\n";
			throw;
		}

		if (sent_no % 1000 == 0) {
			cout << sent_no << " \r"; // should "cout" be replaced by "out" or "SessionLogger::info("SERIF")"?
		}

		Symbol word, word1, word2;

		// unfortunately, the way I did this, we have to throw out the
		// really short sentences
		if (currSent.getLength() >= 2 &&
			nextSent.getLength() >= 2)
		{
			word = currSent.getWord(1);
			word1 = currSent.getWord(0);

			for (int i = 2; i < currSent.getLength(); i++) {
				word2 = word1; word1 = word;
				word = currSent.getWord(i);
				learnInstance(CONT_SENTENCE, word, word1, word2, i);
			}

			word2 = word1; word1 = word;
			word = nextSent.getWord(0);
			learnInstance(START_SENTENCE, word, word1, word2,
						currSent.getLength());

			word2 = word1; word1 = word;
			word = nextSent.getWord(1);
			learnInstance(CONT_SENTENCE, word, word1, word2, 0);
		}

		currSent = nextSent;
		sent_no++;
	}

	_model->printUnderivedTables(outUnderived);
	SessionLogger::info("SERIF") << "Done printing underived tables.\n";

	_model->deriveModel();
	_model->print(out);
}

void ASRSentBreakerTrainer::learnInstance(
	Symbol tag, Symbol word, Symbol word1, Symbol word2, int tok_index)
{
	ASRSentModelInstance instance(tag, word, word1, word2, tok_index);

	_model->addEvent(&instance);
}

