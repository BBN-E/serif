// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/names/IdFSentence.h"
#include "Generic/names/NameClassTags.h"
#include "Generic/sentences/StatSentBreakerTokens.h"
#include "Generic/sentences/StatSentBreakerFVecModel.h"
#include "Generic/sentences/StatSentBreakerTrainer.h"
#include <boost/scoped_ptr.hpp>


using namespace std;


const Symbol StatSentBreakerTrainer::START_SENTENCE(L"ST");
const Symbol StatSentBreakerTrainer::CONT_SENTENCE(L"CO");

StatSentBreakerTrainer::StatSentBreakerTrainer(int mode) : _mode(mode) {

	_training_file = ParamReader::getRequiredParam("training_file");
	_model_file = ParamReader::getRequiredParam("model_file");
	std::string sub_file = ParamReader::getRequiredParam("tokenizer_subst");
	StatSentBreakerTokens::initSubstitutionMap(sub_file.c_str());


	if (_mode == TRAIN) {
		_pruning_threshold = ParamReader::getRequiredIntParam("pruning_threshold");
	}

	if (_mode == DEVTEST) {
		_devtest_file = ParamReader::getRequiredParam("devtest_output");
	}

	if (_mode == TRAIN)
		_model = _new StatSentBreakerFVecModel();
	else if (_mode == DEVTEST) {
		boost::scoped_ptr<UTF8InputStream> modelStream_scoped_ptr(UTF8InputStream::build(_model_file.c_str()));
		UTF8InputStream& modelStream(*modelStream_scoped_ptr);
		_model = _new StatSentBreakerFVecModel(modelStream);
	}

}

void StatSentBreakerTrainer::train() {

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(_training_file.c_str());
	if (in.fail()) {
		throw UnexpectedInputException(
			"StatSentBreakerTrainer::train()",
			"Unable to open training file");
	}

	UTF8OutputStream out;
	out.open(_model_file.c_str());
	if (out.fail()) {
		throw UnexpectedInputException(
			"StatSentBreakerTrainer::train()",
			"Unable to create model file");
	}

	StatSentBreakerTokens currSent = StatSentBreakerTokens();
	StatSentBreakerTokens nextSent = StatSentBreakerTokens();
	currSent.readTrainingSentence(in);

	int sent_no = 0;

	for (;;) {
		try {
			if (!nextSent.readTrainingSentence(in))
				break;
		}
		catch (UnrecoverableException &) {
			cout << "Error! Sentence #" << sent_no+1 << "\n";
			throw;
		}

		if (sent_no % 1000 == 0) {
			cout << sent_no << " \r";
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

	_model->pruneVocab(_pruning_threshold);
	_model->deriveAndPrintModel(out);
}

void StatSentBreakerTrainer::devtest() {

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(_training_file.c_str());
	if (in.fail()) {
		throw UnexpectedInputException(
			"StatSentBreakerTrainer::devtest()",
			"Unable to open training file");
	}

	UTF8OutputStream devTestStream;
	devTestStream.open(_devtest_file.c_str());
	if (devTestStream.fail()) {
		throw UnexpectedInputException(
			"StatSentBreakerTrainer::devtest()",
			"Unable to create output file");
	}

	StatSentBreakerTokens currSent = StatSentBreakerTokens();
	StatSentBreakerTokens nextSent = StatSentBreakerTokens();
	currSent.readTrainingSentence(in);

	int sent_no = 0;
	double score;

	int correct = 0;
	int missed = 0;
	int spurious = 0;
	bool first = true;

	for (;;) {
		try {
			if (!nextSent.readTrainingSentence(in))
				break;
		}
		catch (UnrecoverableException &) {
			cout << "Error! Sentence #" << sent_no+1 << "\n";
			throw;
		}

		if (sent_no % 1000 == 0) {
			cout << sent_no << " \r";
		}

		Symbol word, word1, word2;

		// unfortunately, the way I did this, we have to throw out the
		// really short sentences
		if (currSent.getLength() >= 2 &&
			nextSent.getLength() >= 2)
		{
			word = currSent.getWord(1);
			word1 = currSent.getWord(0);

			if (first) {
				devTestStream << word1.to_string() << L" " << word.to_string() << L" ";
				first = false;
			}


			for (int i = 2; i < currSent.getLength(); i++) {
				word2 = word1; word1 = word;
				word = currSent.getWord(i);
				score = getSTScore(word, word1, word2);
				if (score > 0) {
					spurious++;
					devTestStream << L"<font color=\"blue\">SPURIOUS</font> ";
				}
				devTestStream << word.to_string() << L" ";
			}

			word2 = word1; word1 = word;
			word = nextSent.getWord(0);
			score = getSTScore(word, word1, word2);
			if (score > 0) {
				correct++;
				devTestStream << L"<font color=\"red\">CORRECT</font><br>\n";
			}
			else {
				missed++;
				devTestStream << L"<font color=\"purple\">MISSING</font><br>\n";
			}
			devTestStream << word.to_string() << L" ";

			word2 = word1; word1 = word;
			word = nextSent.getWord(1);
			score = getSTScore(word, word1, word2);
			if (score > 0) {
				spurious++;
				devTestStream << L"<font color=\"blue\">SPURIOUS</font> ";
			}
			devTestStream << word.to_string() << L" ";
		}

		currSent = nextSent;
		sent_no++;
	}
	double recall = (double) correct / (missed + correct);
	double precision = (double) correct / (spurious + correct);

	devTestStream << L"<br><br>\n";
	devTestStream << L"CORRECT: " << correct << L"<br>\n";
	devTestStream << L"MISSED: " << missed << L"<br>\n";
	devTestStream << L"SPURIOUS: " << spurious << L"<br>\n";

	devTestStream << L"RECALL: " << recall << L"<br>\n";
	devTestStream << L"PRECISION: " << precision << L"<br>\n";

}

void StatSentBreakerTrainer::learnInstance(
	Symbol tag, Symbol word, Symbol word1, Symbol word2, int tok_index)
{
	StatSentModelInstance instance(tag, word, word1, word2, tok_index);

	_model->addEvent(&instance);
}

double StatSentBreakerTrainer::getSTScore(Symbol word, Symbol word1, Symbol word2) {
	double p_st, p_co;
	StatSentModelInstance stInstance(START_SENTENCE, word, word1, word2, 0);
	StatSentModelInstance coInstance(CONT_SENTENCE, word, word1, word2, 0);

	p_st = _model->getProbability(&stInstance);
	p_co = _model->getProbability(&coInstance);

	return p_st - p_co;
}
