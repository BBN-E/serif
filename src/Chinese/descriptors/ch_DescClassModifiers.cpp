// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <math.h>
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/ProbModel.h"
#include "Generic/trainers/ProbModelWriter.h"
#include "Generic/theories/SynNode.h"


#include "Chinese/descriptors/ch_DescClassModifiers.h"
#include <boost/scoped_ptr.hpp>

const int ChineseDescClassModifiers::_LOG_OF_ZERO = -10000;
const int ChineseDescClassModifiers::_VOCAB_SIZE = 10000;

ChineseDescClassModifiers::ChineseDescClassModifiers()
	:  _modModel(0), _modBackoffModel(0),
		_modWriter(0), _modBackoffWriter(0)
	{ }

ChineseDescClassModifiers::~ChineseDescClassModifiers(){
	delete _modModel;
	delete _modBackoffModel;
	delete _modWriter;
	delete _modBackoffWriter;
}

void ChineseDescClassModifiers::initialize(std::string model_prefix)
{
	std::string file = model_prefix + ".mnp3";
	boost::scoped_ptr<UTF8InputStream> modModelStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& modModelStream(*modModelStream_scoped_ptr);
	modModelStream.open(file.c_str());
	// no smoothing of premod model (cause we do it by hand)
	// we need to track lambdas, though
	// front component has a kappa of 3.0
	_modModel = _new ProbModel(3, modModelStream, false, true, 3.0);
	modModelStream.close();

	file = model_prefix + ".mnp4";
	boost::scoped_ptr<UTF8InputStream> modBackoffModelStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& modBackoffModelStream(*modBackoffModelStream_scoped_ptr);
	modBackoffModelStream.open(file.c_str());
	// no smoothing of premod model (cause we do it by hand)
	// we need to track lambdas, though
	// back component has a kappa of 1.0
	_modBackoffModel = _new ProbModel(2, modBackoffModelStream, false, true, 1.0);
	modBackoffModelStream.close();
}

void ChineseDescClassModifiers::initializeForTraining(std::string model_prefix) 
{
	 _modWriter = _new ProbModelWriter(3, 500);
	 _modBackoffWriter = _new ProbModelWriter(2, 500);	 
	 std::string file = model_prefix + ".mnp3";
	_modStream.open(file.c_str());
	file = model_prefix + ".mnp4";
	_modBackoffStream.open(file.c_str());

}

void ChineseDescClassModifiers::writeModifiers(){
	_modWriter->writeModel(_modStream);
	_modBackoffWriter->writeModel(_modBackoffStream);
}

void ChineseDescClassModifiers::closeFiles(){
	_modStream.close();
	_modBackoffStream.close();
}

//Calculate the PreModifier portion of the Descriptor Classifier Score
double ChineseDescClassModifiers::getModifierScore(const SynNode* node, Symbol proposedType){
	// Modifier  is (type, stemmed_head, mod_head)
	// mod backoff is (type, mod_head)
	Symbol premodVector[3];
	Symbol premodBackoffVector[2];
	Symbol headWord = node->getHeadWord();

	premodVector[0] = proposedType;
	premodBackoffVector[0] = proposedType;

	premodVector[1] = headWord;
	double premod_score = 0;
	int headIndex = node->getHeadIndex();

	for (int j = 0; j < headIndex; j++) {
		premodVector[2] = premodBackoffVector[1] = node->getChild(j)->getHeadWord();
		double frontProb = exp(_modModel->getProbability(premodVector));
		// WARNING: the incorrect lambda is being used for comparison to old system
		double frontLambda = _modModel->getIncorrectLambda(premodVector);
		double backProb = exp(_modBackoffModel->getProbability(premodBackoffVector));

		// WARNING: the incorrect lambda is being used for comparison to old system
		double backLambda = _modBackoffModel->getIncorrectLambda(premodBackoffVector);
		double combined_score = (frontLambda*frontProb)+ 
			((1.0-frontLambda)*((backLambda*backProb) + 
			((1.0-backLambda)* 1.0/_VOCAB_SIZE)));
		premod_score += combined_score == 0 ? _LOG_OF_ZERO : log(combined_score);
	}
	return premod_score;
}

//For Training add the PreModifiers  of the current node
void ChineseDescClassModifiers::addModifers(const SynNode *node, Symbol type){
	Symbol premodArr[3];
	Symbol premodBackArr[2];

	premodArr[0] = premodBackArr[0] = type;

	int headIndex = node->getHeadIndex();
	Symbol termToHead[100];
	int termSize = node->getTerminalSymbols(termToHead, 100);
	premodArr[1] = node->getHeadWord();

	for (int j = 0; j < termSize; j++) {
		if(termToHead[j]==premodArr[1])
			break;
		premodArr[2] = premodBackArr[1] = termToHead[j];
		_modWriter->registerTransition(premodArr);
		_modBackoffWriter->registerTransition(premodBackArr);
	}
}
