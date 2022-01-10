// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Arabic/descriptors/ar_DescriptorClassifier.h"
#include "Arabic/descriptors/ar_PMDescriptorClassifier.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/ProbModel.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Arabic/parse/ar_STags.h"
#include <math.h>
#include "Generic/common/DebugStream.h"
#include <boost/scoped_ptr.hpp>

const int ArabicPMDescriptorClassifier::_VOCAB_SIZE = 10000;

ArabicPMDescriptorClassifier::ArabicPMDescriptorClassifier()
	: _headModel(0), _priorModel(0), _postmodModel(0), _postmodBackoffModel(0),
	  _functionalParentModel(0),
	  _debug(Symbol(L"desc_class_debug"))
{

	std::string model_prefix = ParamReader::getRequiredParam("desc_classify_model");
	_initialize(model_prefix.c_str());

}

ArabicPMDescriptorClassifier::~ArabicPMDescriptorClassifier() {
	delete _headModel;
	delete _priorModel;
	delete _postmodModel;
	delete _postmodBackoffModel;
	delete _functionalParentModel;
}

int ArabicPMDescriptorClassifier::classifyDescriptor(MentionSet *currSolution, const SynNode* node, EntityType types[], double scores[], int max_results)
{
	// how this works:
	// for each available type determine a score based on:
	// 1) Prior probability of the type
	// 2) wb backed off probability of the stemmed head word
	// 3) modifier model, calculated in several parts for some weird reason
	//	  calculated by ArabicDescClassModifiers so that premod/postmod can be langauge specific -mrf
	// 4) wb backed off probability of the functional parent

	// the probability vectors. Each is of the appropriate length for its model
	Symbol priorVector[2];
	// prior's history is non-existant
	priorVector[0] = SymbolConstants::nullSymbol;
	Symbol headVector[2];
	Symbol postmodVector[3];
	Symbol postmodBackoffVector[2];
	Symbol functionalParentVector[2];
	// we will try to maximize this value for this index
	// scores are stored in the array of scores with corresponding array
	// of types. Scores are stored in sorted order
	int i;
	// initialize the score array
	for (i=0; i < max_results; i++) {
		scores[i] = ArabicDescriptorClassifier::_LOG_OF_ZERO;
		types[i] = EntityType::getUndetType();
	}
	// remember, it's a LOG prob!
//	double top_score = _LOG_OF_ZERO;
//	int top_index = -1;

	// head info for the vector
	Symbol headWord = node->getHeadWord();

	headVector[1] = headWord;
	postmodVector[1] = headWord;

	// functional parent info for the vector
	const SynNode* parent = _getFunctionalParentNode(node);
	if (parent == 0)
		functionalParentVector[1] = SymbolConstants::nullSymbol;
	else
		functionalParentVector[1] = parent->getHeadWord();

	// debug output - follows the format used in the sept. ace reldesc.desclog
	_debug << "CLASSIFYING " << headWord.to_debug_string() << " POSTMOD = ";

	int numChildren = node->getNChildren();
	int headIndex = node->getHeadIndex();

	//TODO: post mods also
	// for debugging, get premods here. We'll get them again later, which is
	// inefficient, but prevents using heap memory unnecessarily
	if (_debug.isActive()) {
		for (int k = headIndex+1; k < numChildren; k++) {
			_debug << node->getChild(k)->getHeadWord().to_debug_string() << " ";
		}
		_debug << "FUNCPARENT = " << functionalParentVector[1].to_debug_string() << "\n";
	}

	// how many results will we wind up with?
	int num_results = 0;
	for (i = 0; i < EntityType::getNTypes(); i++) {
		EntityType proposedType = EntityType::getType(i);
		// fill the vectors with data and get the probability components:
		double prior_score = 0;
		double head_score = 0;
		double postmod_score = 0;
		double functional_parent_score = 0;
		// prior is (null, type)
		priorVector[1] = proposedType.getName();
		prior_score = _priorModel->getProbability(priorVector);

		// head is (type, stemmed_head) [backed off?]
		// head word set outside of this loop
		headVector[0] = proposedType.getName();
		head_score = _headModel->getProbability(headVector);

		// for each postmod:
		// postmod is (type, stemmed_head, postmod_head)
		// head word set outside of this loop
		// postmod backoff is (type, postmod_head)
		postmodVector[0] = proposedType.getName();
		postmodBackoffVector[0] = proposedType.getName();
		// since these will be combined, convert the probs upon acquisition
		for (int j = headIndex+1; j < numChildren; j++) {

			const SynNode* currentChild = node->getChild(j);

			Symbol nodeType = currentChild->getTag();
			const wchar_t* nodeTypeString = nodeType.to_string();

			//eliminate phrases --  look at only preterminals which are nouns of some sort
			if(currentChild->isPreterminal() && (ArabicSTags::isAdjective(nodeType) || ArabicSTags::isNoun(nodeType))){

				postmodVector[2] = postmodBackoffVector[1] = node->getChild(j)->getHeadWord();
				double frontProb = exp(_postmodModel->getProbability(postmodVector));

				// WARNING: the incorrect lambda is being used for comparison to old system
				//double frontLambda = _postmodModel->getIncorrectLambda(postmodVector);
				double frontLambda = _postmodModel->getLambda(postmodVector);
				double backProb = exp(_postmodBackoffModel->getProbability(postmodBackoffVector));

				// WARNING: the incorrect lambda is being used for comparison to old system
				double backLambda = _postmodBackoffModel->getIncorrectLambda(postmodBackoffVector);
				double combined_score = (frontLambda*frontProb)+
					((1.0-frontLambda)*((backLambda*backProb) +
					((1.0-backLambda)* 1.0/_VOCAB_SIZE)));
				postmod_score += combined_score == 0 ? ArabicDescriptorClassifier::_LOG_OF_ZERO : log(combined_score);
			}
		}


		// functionalParent is (type, func_parent_word)
		functionalParentVector[0] = proposedType.getName();
		// functionalParent word is set outside of this loop.

		//if(parent != NULL) {

		//	Symbol nodeType = parent->getTag();
		//	if(!(ArabicSTags::isPreposition(nodeType))){
				functional_parent_score = _functionalParentModel->getProbability(functionalParentVector);
		//	}else {
		//		functional_parent_score = 0;
		//	}

		//}else{
		//	functional_parent_score = _functionalParentModel->getProbability(functionalParentVector);
		//}

		// those scores are all multiplied (log added) together. The type
		// with the highest total score is the return type
		double total_score = prior_score+head_score+postmod_score+functional_parent_score;

		// debug output like the old style
		_debug << proposedType.getName().to_debug_string() << ": "
			   << total_score << " -- "
			   << prior_score << "\t"
			   << head_score << "\t"
			   << postmod_score << "\t"
			   << functional_parent_score << "\n";

		if (total_score > ArabicDescriptorClassifier::_LOG_OF_ZERO) {
			num_results = ArabicDescriptorClassifier::insertScore(total_score, proposedType, scores, types, num_results, max_results);
		}
	}

	// this would be odd, I think
	if (num_results == 0)
		throw InternalInconsistencyException("ArabicPMDescriptorClassifier::_classifyDescriptor",
		"Couldn't find a decent score");

//	_debug << "ANSWER: " << answer.to_debug_string() << "\n\n";
	return num_results;
}

const SynNode* ArabicPMDescriptorClassifier::_getFunctionalParentNode(const SynNode* node)
{
	if( node == 0 )
		return 0;
	const SynNode* nd = node;
	while( nd != 0 &&
		nd->getParent() != 0 &&
		nd->getParent()->getHead() == nd )
		nd = nd->getParent();
	if( nd != 0 && nd->getParent() != 0 )
		return nd->getParent();
	return 0; // otherwise we can't use it anyway
}



// given a param file of files, read them in and initialize models.
void ArabicPMDescriptorClassifier::_initialize(const char* model_prefix)
{

	std::string model_prefix_str(model_prefix);

	std::string buffer = model_prefix_str + ".mnp1";
	boost::scoped_ptr<UTF8InputStream> priorModelStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& priorModelStream(*priorModelStream_scoped_ptr);
	priorModelStream.open(buffer.c_str());
	// no smoothing of prior model
	_priorModel = _new ProbModel(2, priorModelStream, false);
	priorModelStream.close();

	buffer = model_prefix_str + ".mnp2";
	boost::scoped_ptr<UTF8InputStream> headModelStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& headModelStream(*headModelStream_scoped_ptr);
	headModelStream.open(buffer.c_str());
	_headModel = _new ProbModel(2, headModelStream, true);
	headModelStream.close();

	buffer = model_prefix_str + ".mnp3";
	boost::scoped_ptr<UTF8InputStream> postmodModelStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& postmodModelStream(*postmodModelStream_scoped_ptr);
	postmodModelStream.open(buffer.c_str());
	// no smoothing of postmod model (cause we do it by hand)
	// we need to track lambdas, though
	// front component has a kappa of 3.0
	_postmodModel = _new ProbModel(3, postmodModelStream, false, true, 3.0);
	postmodModelStream.close();

	buffer = model_prefix_str + ".mnp4";
	boost::scoped_ptr<UTF8InputStream> postmodBackoffModelStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& postmodBackoffModelStream(*postmodBackoffModelStream_scoped_ptr);
	postmodBackoffModelStream.open(buffer.c_str());
	// no smoothing of postmod model (cause we do it by hand)
	// we need to track lambdas, though
	// back component has a kappa of 1.0
	_postmodBackoffModel = _new ProbModel(2, postmodBackoffModelStream, false, true, 1.0);
	postmodBackoffModelStream.close();

	buffer = model_prefix_str + ".mnp5";
	boost::scoped_ptr<UTF8InputStream> functionalParentModelStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& functionalParentModelStream(*functionalParentModelStream_scoped_ptr);
	functionalParentModelStream.open(buffer.c_str());
	_functionalParentModel = _new ProbModel(2, functionalParentModelStream, true);
	functionalParentModelStream.close();
}

