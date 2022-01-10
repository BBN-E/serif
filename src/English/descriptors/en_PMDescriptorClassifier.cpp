// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/descriptors/en_DescriptorClassifier.h"
#include "English/descriptors/en_PMDescriptorClassifier.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/ProbModel.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include <math.h>
#include "Generic/common/DebugStream.h"
#include "English/descriptors/en_TemporalIdentifier.h"
#include "Generic/wordnet/xx_WordNet.h"
#include <boost/scoped_ptr.hpp>

using namespace std;

const int EnglishPMDescriptorClassifier::_VOCAB_SIZE = 10000;


EnglishPMDescriptorClassifier::EnglishPMDescriptorClassifier()
	: _headModel(0), _priorModel(0), _premodModel(0), _premodBackoffModel(0),
	  _functionalParentModel(0),
	  _debug(Symbol(L"desc_class_debug"))
{

	std::string model_prefix = ParamReader::getRequiredParam("desc_classify_model");
	initialize(model_prefix.c_str());

	TemporalIdentifier::loadTemporalHeadwordList();
}

EnglishPMDescriptorClassifier::~EnglishPMDescriptorClassifier() {
	delete _headModel;
	delete _priorModel;
	delete _premodModel;
	delete _premodBackoffModel;
	delete _functionalParentModel;
}

int EnglishPMDescriptorClassifier::classifyDescriptor(MentionSet *currSolution, const SynNode* node, EntityType types[], double scores[], int max_results)
{
	// how this works:
	// for each available type determine a score based on:
	// 1) Prior probability of the type
	// 2) wb backed off probability of the stemmed head word
	// 3) modifier model, calculated in several parts for some weird reason
	//	  calculated by EnglishDescClassModifiers so that premod/postmod can be langauge specific -mrf
	// 4) wb backed off probability of the functional parent

	// the probability vectors. Each is of the appropriate length for its model
	Symbol priorVector[2];
	// prior's history is non-existant
	priorVector[0] = SymbolConstants::nullSymbol;
	Symbol headVector[2];
	Symbol premodVector[3];
	Symbol premodBackoffVector[2];
	Symbol functionalParentVector[2];
	// we will try to maximize this value for this index
	// scores are stored in the array of scores with corresponding array
	// of types. Scores are stored in sorted order
	int i;
	// initialize the score array
	for (i=0; i < max_results; i++) {
		scores[i] = EnglishDescriptorClassifier::_LOG_OF_ZERO;
		types[i] = EntityType::getUndetType();
	}
	// remember, it's a LOG prob!
//	double top_score = _LOG_OF_ZERO;
//	int top_index = -1;

	// head info for the vector
	Symbol headWord = node->getHeadWord();

	headVector[1] = headWord;
	premodVector[1] = headWord;

	// functional parent info for the vector
	const SynNode* parent = getFunctionalParentNode(node);
	if (parent == 0)
		functionalParentVector[1] = SymbolConstants::nullSymbol;
	else
		functionalParentVector[1] = parent->getHeadWord();

	// debug output - follows the format used in the sept. ace reldesc.desclog
	_debug << "CLASSIFYING " << headWord.to_debug_string() << " PREMOD = ";

	//TODO: post mods also
	// for debugging, get premods here. We'll get them again later, which is
	// inefficient, but prevents using heap memory unnecessarily
	if (_debug.isActive()) {
		for (int k = 0; k < node->getHeadIndex(); k++) {
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
		double premod_score = 0;
		double functional_parent_score = 0;
		// prior is (null, type)
		priorVector[1] = proposedType.getName();
		prior_score = _priorModel->getProbability(priorVector);

		// head is (type, stemmed_head) [backed off?]
		// head word set outside of this loop
		headVector[0] = proposedType.getName();
		head_score = _headModel->getProbability(headVector);

		// for each premod:
		// premod is (type, stemmed_head, premod_head)
		// head word set outside of this loop
		// premod backoff is (type, premod_head)
		premodVector[0] = proposedType.getName();
		premodBackoffVector[0] = proposedType.getName();
		// since these will be combined, convert the probs upon acquisition
		for (int j = 0; j < node->getHeadIndex(); j++) {
			premodVector[2] = premodBackoffVector[1] = node->getChild(j)->getHeadWord();
			double frontProb = exp(_premodModel->getProbability(premodVector));
			// WARNING: the incorrect lambda is being used for comparison to old system
			double frontLambda = _premodModel->getIncorrectLambda(premodVector);
			double backProb = exp(_premodBackoffModel->getProbability(premodBackoffVector));

			// WARNING: the incorrect lambda is being used for comparison to old system
			double backLambda = _premodBackoffModel->getIncorrectLambda(premodBackoffVector);
			double combined_score = (frontLambda*frontProb)+
				((1.0-frontLambda)*((backLambda*backProb) +
				((1.0-backLambda)* 1.0/_VOCAB_SIZE)));
			premod_score += combined_score == 0 ? EnglishDescriptorClassifier::_LOG_OF_ZERO : log(combined_score);
		}


		// functionalParent is (type, func_parent_word)
		functionalParentVector[0] = proposedType.getName();
		// functionalParent word is set outside of this loop.

		functional_parent_score = _functionalParentModel->getProbability(functionalParentVector);

		// those scores are all multiplied (log added) together. The type
		// with the highest total score is the return type
		double total_score = prior_score+head_score+premod_score+functional_parent_score;

		// debug output like the old style
		_debug << proposedType.getName().to_debug_string() << ": "
			   << total_score << " -- "
			   << prior_score << "\t"
			   << head_score << "\t"
			   << premod_score << "\t"
			   << functional_parent_score << "\n";

		if (total_score > EnglishDescriptorClassifier::_LOG_OF_ZERO) {

			// for EELD, we force things with temporal headwords to be temporals
			if (EntityType::getTempType().isRecognized()) {
				if (!proposedType.isTemp() &&
					TemporalIdentifier::looksLikeTemporal(node))
				{
					proposedType = EntityType::getTempType();
				}
			}

			num_results = EnglishDescriptorClassifier::insertScore(total_score, proposedType, scores, types, num_results, max_results);
		}
	}

	// wordnet coercing is fun!
	if (num_results > 1 &&
		types[0] == EntityType::getOtherType() &&
		scores[0] - scores[1] < 4)
	{
		if (types[1] == EntityType::getPERType() &&
			WordNet::getInstance()->isPerson(node->getHeadWord()))
		{
			types[0] = types[1];
			types[1] = EntityType::getOtherType();
			/*std::cerr << "changing type of " << node->getHeadWord().to_debug_string() << " to PER\n";
			std::cerr << "score of OTH: " << scores[0] << "\n";
			std::cerr << "score of PER: " << scores[1] << "\n";*/
		}
	}

	// this would be odd, I think
	if (num_results == 0)
		throw InternalInconsistencyException("EnglishPMDescriptorClassifier::_classifyDescriptor",
		"Couldn't find a decent score");

//	_debug << "ANSWER: " << answer.to_debug_string() << "\n\n";
	return num_results;
}

const SynNode* EnglishPMDescriptorClassifier::getFunctionalParentNode(const SynNode* node)
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
void EnglishPMDescriptorClassifier::initialize(const char* model_prefix)
{
	char buffer[501];

	sprintf(buffer,"%s.mnp1", model_prefix);
	verifyEntityTypes(buffer);
	boost::scoped_ptr<UTF8InputStream> priorModelStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& priorModelStream(*priorModelStream_scoped_ptr);
	priorModelStream.open(buffer);
	// no smoothing of prior model
	_priorModel = _new ProbModel(2, priorModelStream, false);
	priorModelStream.close();

	sprintf(buffer,"%s.mnp2", model_prefix);
	boost::scoped_ptr<UTF8InputStream> headModelStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& headModelStream(*headModelStream_scoped_ptr);
	headModelStream.open(buffer);
	_headModel = _new ProbModel(2, headModelStream, true);
	headModelStream.close();

	sprintf(buffer,"%s.mnp3", model_prefix);
	boost::scoped_ptr<UTF8InputStream> premodModelStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& premodModelStream(*premodModelStream_scoped_ptr);
	premodModelStream.open(buffer);
	// no smoothing of premod model (cause we do it by hand)
	// we need to track lambdas, though
	// front component has a kappa of 3.0
	_premodModel = _new ProbModel(3, premodModelStream, false, true, 3.0);
	premodModelStream.close();

	sprintf(buffer,"%s.mnp4", model_prefix);
	boost::scoped_ptr<UTF8InputStream> premodBackoffModelStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& premodBackoffModelStream(*premodBackoffModelStream_scoped_ptr);
	premodBackoffModelStream.open(buffer);
	// no smoothing of premod model (cause we do it by hand)
	// we need to track lambdas, though
	// back component has a kappa of 1.0
	_premodBackoffModel = _new ProbModel(2, premodBackoffModelStream, false, true, 1.0);
	premodBackoffModelStream.close();

	sprintf(buffer,"%s.mnp5", model_prefix);
	boost::scoped_ptr<UTF8InputStream> functionalParentModelStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& functionalParentModelStream(*functionalParentModelStream_scoped_ptr);
	functionalParentModelStream.open(buffer);
	_functionalParentModel = _new ProbModel(2, functionalParentModelStream, true);
	functionalParentModelStream.close();
}

void EnglishPMDescriptorClassifier::verifyEntityTypes(char *file_name) {
	boost::scoped_ptr<UTF8InputStream> priorModelStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& priorModelStream(*priorModelStream_scoped_ptr);
	priorModelStream.open(file_name);

	if (priorModelStream.fail()) {
		throw UnexpectedInputException(
			"EnglishPMDescriptorClassifier::verifyEntityTypes()",
			"Unable to open descriptor classifier model files");
	}

	int n_entries;
	priorModelStream >> n_entries;

	for (int i = 0; i < n_entries; i++) {
		UTF8Token token;
		priorModelStream >> token; // (
		priorModelStream >> token; // (
		priorModelStream >> token; // :NULL

		priorModelStream >> token; // enitity type
		try {
			EntityType(token.symValue());
		}
		catch (UnexpectedInputException &) {
			std::string message = "";
			message += "The descriptor classifier model refers to "
					   "at least one unrecognized\n"
					   "entity type: ";
			message += token.symValue().to_debug_string();
			throw UnexpectedInputException(
				"EnglishPMDescriptorClassifier::verifyEntityTypes()",
				message.c_str());
		}

		priorModelStream >> token; // )
		priorModelStream >> token; // number
		priorModelStream >> token; // number
		priorModelStream >> token; // number
		priorModelStream >> token; // )
	}

	priorModelStream.close();
}

