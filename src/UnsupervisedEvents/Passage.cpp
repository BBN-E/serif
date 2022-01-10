/*#include "Generic/common/leak_detection.h"
#include <sstream>
#include <string>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include "Generic/common/Symbol.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Parse.h"
#include "../GraphicalModels/distributions/MultinomialDistribution.h"
#include "Passage.h"
#include "PassageDescription.h"
#include "variables/RelationTypeVar.h"
#include "factors/RelationTypeBagOfWordsFactor.h"

using std::wstringstream;
using std::wstring;
using boost::make_shared;
using boost::shared_ptr;
using GraphicalModel::SymmetricDirichlet;
using GraphicalModel::SymmetricDirichlet_ptr;

const double RelationTypePrior::SMOOTHING = 5.0;

Passage_ptr Passage::create(const PassageDescription_ptr& pd, 
		const DocTheory* dt, unsigned int n_relation_types, bool keep_passage)
{
	if (pd->startSentence() < 0 || pd->endSentence() < pd->startSentence() 
			|| pd->endSentence() >= (unsigned int)dt->getNSentences())
	{
		wstringstream err;
		err << L"Passage indices out of bounds for docid = " << pd->docid()
			<< L", start=" << pd->startSentence() << L", end=" << pd->endSentence()
			<< L"; num sentences in doc is " << dt->getNSentences() ;
		throw UnexpectedInputException("Passage::create", err);
	}

	Passage_ptr passage = make_shared<Passage>();
	passage->_relTypeBOWFact = 
		make_shared<RelationTypeBagOfWordsFactor>(n_relation_types);

	for (unsigned int sentence = pd->startSentence(); sentence<=pd->endSentence(); ++sentence) {
		const SentenceTheory* st = dt->getSentenceTheory(sentence);
		const SynNode* root = st->getPrimaryParse()->getRoot();
		passage->_relTypeBOWFact->gatherWords(root);
	}

	connect(*passage->_relTypeBOWFact, *passage->_relTypeVar);
	passage->_relTypePrior = make_shared<RelationTypePrior>(n_relation_types);
	connect(*passage->_relTypePrior, *passage->_relTypeVar);

	if (keep_passage) {
		passage->setDescription(pd);
	}

	return passage;
}

void Passage::finishedLoading(unsigned int n_relation_classes, 
		const SymmetricDirichlet_ptr& asymDir) 
{
	const double relationGenWordsSmoothing = 100;
	//asymDir->rescaleToMax(relationGenWordsSmoothing);
	RelationTypeBagOfWordsFactor::finishedLoading(n_relation_classes, true,
			asymDir);
	RelationTypePrior::finishedLoading(n_relation_classes);
}

void Passage::initializeRandomly() {
	std::vector<double> tmp(_relTypeVar->nValues());
	double x = 1.0/_relTypeVar->nValues();
	BOOST_FOREACH(double& v, tmp) {
		v = x;
	}

	//double sum = 0;
	//BOOST_FOREACH(double& x, tmp) {
		//x = rand() % 25 + 5;
		//sum += x;
	//}
	//BOOST_FOREACH(double& x, tmp) {
		//x/=sum;
	//}
	_relTypeVar->setMarginals(tmp);
}

void Passage::observeImpl() {
	_relTypeBOWFact->observe(*_relTypeVar);
	_relTypePrior->observe(*_relTypeVar);


//	RelationTypeBagOfWordsFactor::observe(_words, *_relTypeVar);
	//RelationTypePrior::observe(*_relTypeVar);
}

void Passage::clearCounts() {
	RelationTypeBagOfWordsFactor::clearCounts();
	RelationTypePrior::clearCounts();
}

*/
