// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <math.h>
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/ProbModel.h"
#include "Generic/trainers/ProbModelWriter.h"
#include "Generic/theories/SynNode.h"


#include "Arabic/descriptors/ar_DescClassModifiers.h"
#include <boost/scoped_ptr.hpp>

const int ArabicDescClassModifiers::_LOG_OF_ZERO = -10000;
const int ArabicDescClassModifiers::_VOCAB_SIZE = 10000;

ArabicDescClassModifiers::ArabicDescClassModifiers()
	:  _modModel(0), _modBackoffModel(0),
		_modWriter(0), _modBackoffWriter(0)
	{ }

//to do Does this cause exceptions?
ArabicDescClassModifiers::~ArabicDescClassModifiers(){
	delete _modModel;
	delete _modBackoffModel;
	delete _modWriter;
	delete _modBackoffWriter;
}

void ArabicDescClassModifiers::initialize(std::string model_prefix)
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

void ArabicDescClassModifiers::initializeForTraining(std::string model_prefix) 
{
	 _modWriter = _new ProbModelWriter(3, 500);
	 _modBackoffWriter = _new ProbModelWriter(2, 500);	 
	 std::string file = model_prefix + ".mnp3";
	_modStream.open(file.c_str());
	file = model_prefix + ".mnp4";
	_modBackoffStream.open(file.c_str());

}

void ArabicDescClassModifiers::writeModifiers(){
	_modWriter->writeModel(_modStream);
	_modBackoffWriter->writeModel(_modBackoffStream);
}

void ArabicDescClassModifiers::closeFiles(){
	_modStream.close();
	_modBackoffStream.close();
}


//Real Language Specific Functions
double ArabicDescClassModifiers::getModifierScore(const SynNode* node, Symbol proposedType){
	// Modifier  is (type, stemmed_head, mod_head)
	// mod backoff is (type, mod_head)
	Symbol postmodVector[3];
	Symbol postmodBackoffVector[2];
	Symbol headWord = node->getHeadWord();

	postmodVector[0] = proposedType;
	postmodBackoffVector[0] = proposedType;

	postmodVector[1] = headWord;
	double postmod_score = 0;
	int numChildren = node->getNChildren();
	int headIndex = node->getHeadIndex();
	for (int j = headIndex+1; j < numChildren; j++) {
    //double combined_score = 0;

		const SynNode* currentChild = node->getChild(j); 
	    
		Symbol nodeType = currentChild->getTag();
		const wchar_t* nodeTypeString = nodeType.to_string();
		
		//eliminate phrases --  look at only preterminals which are nouns of some sort				
		//if(currentChild->isPreterminal() && (nodeType.to_debug_string()[0] == 'A' || nodeType.to_debug_string()[0] == 'N')){

			postmodVector[2] = postmodBackoffVector[1] = node->getChild(j)->getHeadWord();
			double frontProb = exp(_modModel->getProbability(postmodVector));

			// WARNING:	the incorrect lambda is being used for comparison to old system
			//double frontLambda = _modModel->getIncorrectLambda(postmodVector);
			double frontLambda = _modModel->getLambda(postmodVector);
			double backProb = exp(_modBackoffModel->getProbability(postmodBackoffVector));

			// WARNING: the incorrect lambda is being used for comparison to old system
			double backLambda = _modBackoffModel->getIncorrectLambda(postmodBackoffVector);
			double combined_score = (frontLambda*frontProb)+ 
				((1.0-frontLambda)*((backLambda*backProb) + 
				((1.0-backLambda)* 1.0/_VOCAB_SIZE)));
			postmod_score += combined_score == 0 ? _LOG_OF_ZERO : log(combined_score);
		//}		
	}
	return postmod_score;
}
void ArabicDescClassModifiers::addModifers(const SynNode *node, Symbol type){
	Symbol postmodArr[3];
	Symbol postmodBackArr[2];

	postmodArr[0] = postmodBackArr[0] = type;

	int headIndex = node->getHeadIndex();
	Symbol termToHead[100];
	int termSize = node->getTerminalSymbols(termToHead, 100);
	postmodArr[1] = node->getHeadWord();
	for (int j = headIndex+1; j < termSize; j++) {
		postmodArr[2] = postmodBackArr[1] = termToHead[j];
		_modWriter->registerTransition(postmodArr);
		_modBackoffWriter->registerTransition(postmodBackArr);
	}
}

