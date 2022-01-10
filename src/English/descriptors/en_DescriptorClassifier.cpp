// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"

#include "English/descriptors/en_DescriptorClassifier.h"
#include "English/descriptors/en_PMDescriptorClassifier.h"
#include "Generic/descriptors/discmodel/P1DescriptorClassifier.h"
#include <boost/scoped_ptr.hpp>

/* added for speed up
// static member initialization
bool EnglishDescriptorClassifier::_initialized = false;
EnglishDescriptorClassifier::Table* EnglishDescriptorClassifier::_nomMentionList = 0;
*/

void EnglishDescriptorClassifier::cleanup() {
	if (_p1DescriptorClassifier)
		_p1DescriptorClassifier->cleanup();
}

EnglishDescriptorClassifier::EnglishDescriptorClassifier()
{

	_pmDescriptorClassifier = 0;
	_p1DescriptorClassifier = 0;
	std::string model_type = ParamReader::getRequiredParam("desc_classify_model_type");
	if (model_type == "PM") {
		_classifier_type = PM;
		_pmDescriptorClassifier = _new EnglishPMDescriptorClassifier();
	} else if (model_type == "P1") {
		_classifier_type = P1;
		_p1DescriptorClassifier = _new P1DescriptorClassifier();
	} else {
		std::string error = "Parameter 'desc_classify_model_type' must be set to 'PM' or 'P1'";
		throw UnexpectedInputException("EnglishDescriptorClassifier::EnglishDescriptorClassifier()", error.c_str());
	}

	/* added for speed up
	char nomMentionFile[500];
	if(!ParamReader::getParam("nom_mention_list",nomMentionFile, 499)){
		cout << "nom-mention-list undefined ! \n";
	}else if (_nomMentionList == 0){
		EnglishDescriptorClassifier::_nomMentionList = _new Table(5000);
		boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& uis(*uis_scoped_ptr);
		uis.open(nomMentionFile);
		UTF8Token word;
		UTF8Token label;
		while(!uis.eof()){
			uis >> word;
			uis >> label;
			cout << word.symValue() << "\t"  << label.symValue() << endl;
			(* EnglishDescriptorClassifier::_nomMentionList)[word.symValue()] = label.symValue();
		}
		EnglishDescriptorClassifier::_initialized = true;
	}

	*/
}

EnglishDescriptorClassifier::~EnglishDescriptorClassifier() {
	delete _pmDescriptorClassifier;
	delete _p1DescriptorClassifier;
}

int EnglishDescriptorClassifier::classifyDescriptor(MentionSet *currSolution, const SynNode* node,
											 EntityType types[], double scores[], int max_results)
{
	
	if (_classifier_type == PM) {
		return _pmDescriptorClassifier->classifyDescriptor(currSolution, node,
		types, scores, max_results);
	} else if (_classifier_type == P1) {
		//check whether the head word is in the nomMentionList, if so, return results directly 
		//if not, do classification
		/*
		EnglishDescriptorClassifier::Table::iterator iter = EnglishDescriptorClassifier::_nomMentionList->find(node->getHeadWord());
		if (iter != EnglishDescriptorClassifier::_nomMentionList->end()){
			EntityType proposedType = EntityType((*iter).second);
			return EnglishDescriptorClassifier::insertScore(1.0, proposedType, scores, types, 0, 1);
		}
		*/

		return _p1DescriptorClassifier->classifyDescriptor(currSolution, node,
			types, scores, max_results);

	}

	return 0;
}

/* added for speed up
#ifdef DO_DESCRIPTOR_CLASSIFIER_TRACE
void EnglishDescriptorClassifier::printTrace() {
#ifdef DO_P1_DESCRIPTOR_CLASSIFIER_TRACE
	_p1DescriptorClassifier->printTrace();
#endif
}
#endif
*/
