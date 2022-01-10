// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "English/parse/en_STags.h"
#include "Generic/wordnet/xx_WordNet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/NodeInfo.h"
#include "English/parse/en_NodeInfo.h"
#include "English/descriptors/en_NomPremodClassifier.h"
#include "Generic/common/SymbolListMap.h"
#include <boost/scoped_ptr.hpp>
/* added for speed up
#include "English/descriptors/en_DescriptorClassifier.h"
*/
using namespace std;

void EnglishNomPremodClassifier::cleanup() {
	if (_p1DescriptorClassifier)
		_p1DescriptorClassifier->cleanup();
}

EnglishNomPremodClassifier::EnglishNomPremodClassifier()
	:  _dout_open(false), _certainNomPremods(0),
	_p1DescriptorClassifier(0)
{
	if (!EnglishNodeInfo::useNominalPremods()) return;

	_p1DescriptorClassifier = new P1DescriptorClassifier(P1DescriptorClassifier::PREMOD_CLASSIFY);

	std::string certain = ParamReader::getParam("certain_nominal_premods");
	if (!certain.empty()) {
		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(certain.c_str()));
		UTF8InputStream& in(*in_scoped_ptr);
		_certainNomPremods = _new SymbolListMap(in);
		in.close();
	} else _certainNomPremods = 0;

	std::string debugStreamParam = ParamReader::getParam("nom_premod_classifier_debug_file");
	if (!debugStreamParam.empty()) {
		_dout.open(debugStreamParam.c_str());
		if (_dout.fail()) {
			cerr << "could not open nom_premod_classifier_debug_file\n";
		}
		else {
			_dout_open = true;
		}
	}

	/* added for speed up
	char nomMentionFile[500];
	if(EnglishDescriptorClassifier::_nomMentionList == 0 && ParamReader::getParam("nom_mention_list",nomMentionFile, 499)){
		EnglishDescriptorClassifier::_nomMentionList = _new EnglishDescriptorClassifier::Table(5000);
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

EnglishNomPremodClassifier::~EnglishNomPremodClassifier() {
	if (_dout_open)
		_dout.close();
	if (_p1DescriptorClassifier) 
		delete _p1DescriptorClassifier;
	delete _certainNomPremods;
}

int EnglishNomPremodClassifier::classifyNomPremod(
	MentionSet *currSolution, const SynNode* node,
	EntityType types[], double scores[], int max_results)
{	
	if (!EnglishNodeInfo::useNominalPremods()) {
		types[0] = EntityType::getOtherType();
		scores[0] = 0;
		return 1;
	}

	int num_results = 0;
	
	if (_dout_open) {
		wstring nodeStr = node->getParent()->toString(0);
		_dout << "--- Classifying " << node->getHeadWord().to_string() << "\n"
			  << nodeStr << "\n";
	}

	Symbol headword = node->getHeadWord();

	if (_certainNomPremods != 0) {
		int numTypes = 0;
		const Symbol *candTypes = _certainNomPremods->lookup(headword, numTypes);
		if (numTypes != 0) {
			num_results = insertScore(1.0, EntityType(candTypes[0]),
				scores, types, num_results, max_results);
			if (_dout_open)
				_dout << "CERTAIN PREMOD " << candTypes[0] << "\n";
			return 1;
		}
	}

	// only classify the premod if the modifyee looks like a known type.
	// this will at least happen when we go through the second time
	const SynNode *modifyee = node->getParent()->getHead();
	EntityType modifyeeType = EntityType::getUndetType();

	while (!modifyee->hasMention() && modifyee->getParent() != 0 && 
		modifyee->getParent()->getHead() == modifyee)
	{
		modifyee = modifyee->getParent();
	}

	if (modifyee->hasMention()) {
		modifyeeType = currSolution->getMentionByNode(modifyee)->getEntityType();
	} 

	if (_dout_open)
		_dout << "Parent's type known: " << modifyeeType.getName().to_string() << "\n";

	// let's try only classifying it if it's the last premod
	int last_premod_index = node->getParent()->getHeadIndex() - 1;
	const SynNode *lastPremod = 0;
	if (last_premod_index >= 0)
		lastPremod = node->getParent()->getChild(last_premod_index);
	bool premod_is_last = false;
	if (lastPremod == node)
		premod_is_last = true;

	/*bool has_named_first_premod = false;
	if (premod_is_last && node->getParent() != 0) {
		const SynNode *parent = node->getParent();
		for (int i = 0; i < parent->getHeadIndex() - 1; i++) {
			if (parent->getChild(i)->hasMention() &&
				currSolution->getMentionByNode(parent->getChild(i))->getMentionType() == Mention::NAME)
			{
				has_named_first_premod = true;
			}
		}
	}

	if (WordNet::getInstance()->isHyponymOf(headword, "weapon")) {
		num_results = insertScore(1.0, EntityType(Symbol(L"WEA")),
			scores, types, num_results, max_results);
	} else if (has_named_first_premod &&
		WordNet::getInstance()->isHyponymOf(headword, "vehicle") &&
		WordNet::getInstance()->isHyponymOf(headword, "transport")) 
	{		
		num_results = insertScore(1.0, EntityType(Symbol(L"VEH")),
			scores, types, num_results, max_results);
	} else */if (premod_is_last && modifyeeType.isRecognized()) {
		//check whether the head word is in the nomMentionList, if so, return results directly 
		//if not, do classification
		/*EntityType answer;
		EnglishDescriptorClassifier::Table::iterator iter = EnglishDescriptorClassifier::_nomMentionList->find(node->getHeadWord());
		if (iter != EnglishDescriptorClassifier::_nomMentionList->end()){
			answer = EntityType((*iter).second);
		}else{
			answer = _p1DescriptorClassifier->classifyDescriptor(currSolution, node);
		}
		num_results = insertScore(1.0, answer,	scores, types, num_results, max_results);
		*/

		EntityType answer = _p1DescriptorClassifier->classifyDescriptor(
								currSolution, node);
		num_results = insertScore(1.0, answer,
			scores, types, num_results, max_results);
		

		if (_dout_open)
			_dout << "Classified as: "
				  << answer.getName().to_string() << "\n";
	} 
	else {
		// if modifyee is not of recognized type, then assume that
		// this premod is not important either
		num_results = insertScore(1.0, EntityType::getOtherType(),
			scores, types, num_results, max_results);
	}

	if (_dout_open) {
		_dout << "\n";
		_dout.flush();
	}

	if (num_results == 0) {
		throw InternalInconsistencyException(
			"EnglishNomPremodClassifier::classifyNomPremod",
			"Couldn't find a decent score");
	}

	return num_results;
}

// "American /president/ George Bush", "his /homeland/ France"
EntityType EnglishNomPremodClassifier::getTitleType(MentionSet *currSolution, const SynNode* node)
{

	// OBVIOUSLY, this will overlink -- I'm not even looking at what the headword
	// of the mention is! But in most cases it will be OK; usually this construction
	// really does convey a title.

	const SynNode* parentNode = node->getParent();
	if (parentNode == 0 || !parentNode->hasMention() || parentNode->getNChildren() < 2)
		return EntityType::getUndetType();

	// the node in question must be second-to-rightmost child of its parent
	if (parentNode->getChild(parentNode->getNChildren() - 2) != node)
		return EntityType::getUndetType();

	// parent mention must be a name (no funny business with lists, etc.)
	const Mention* parentMention = currSolution->getMentionByNode(parentNode);
	if (parentMention->getMentionType() != Mention::NAME &&
		parentMention->getMentionType() != Mention::NONE)
		return EntityType::getUndetType();

	// no commas or CCs allowed
	for (int j = 0; j < parentNode->getNChildren(); j++) {
		const SynNode *child = parentNode->getChild(j);
		if (child->getTag() == EnglishSTags::CC || child->getTag() == EnglishSTags::COMMA) {
			return  EntityType::getUndetType();
		}
	}

	// rightmost child must be a name mention of recognized type
	const SynNode *child = parentNode->getChild(parentNode->getNChildren()-1);
	if (!child->hasMention())
		return  EntityType::getUndetType();

	const Mention *rightmost_ment = currSolution->getMentionByNode(child);
	if (rightmost_ment->getMentionType() != Mention::NAME ||
		!rightmost_ment->isOfRecognizedType())
	{
		return  EntityType::getUndetType();
	}

//	std::cout << "successful match: " << parentNode->toDebugTextString() << "\n\n";

	// any other necessary tests? Do I care what's to the left of this?
	return rightmost_ment->getEntityType();
}
