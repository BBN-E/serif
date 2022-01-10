// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
// For documentation of this class, please see http://speechweb/text_group/default.aspx

#include "Generic/common/leak_detection.h"

#include "Generic/common/limits.h"

#include "Generic/common/Symbol.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/propositions/PropositionFinder.h"
#include "Generic/propositions/sem_tree/SemNode.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Parse.h"
#include "Generic/common/IDGenerator.h"
#include "Generic/propositions/PropositionStatusClassifier.h"
#include "Generic/propositions/SemTreeBuilder.h"
#include "Generic/propositions/LinearPropositionFinder.h"

#include <iostream>

//#define PROP_DEBUG "c:/Temp/serif/dumps/props.debug.txt"

using namespace std;


// run-time parameters
bool PropositionFinder::_use_nominal_premods;
bool PropositionFinder::_unify_appositives;
bool PropositionFinder::_use_2009_props;

PropositionFinder::PropositionFinder() {
	_linearPropositionFinder = LinearPropositionFinder::build();
	_semTreeBuilder = SemTreeBuilder::get();

	SemTreeBuilder::get()->initialize();

	if (ParamReader::isParamTrue("use_nominal_premods")) {
		_use_nominal_premods = true;
	} else {
		_use_nominal_premods = false;
	}

	if (ParamReader::isParamTrue("use_2009_props")) {
		_use_2009_props = true;
	} else {
		_use_2009_props = false;
	}

	_unify_appositives = ParamReader::getRequiredTrueFalseParam(L"unify_appositives");
}


void PropositionFinder::resetForNewSentence(int sentence_number) {
	_sentence_number = sentence_number;
}

PropositionSet *PropositionFinder::getPropositionTheory(const Parse *parse, const MentionSet *mentionSet) {

		// don't try to find propositions when sentence wasn't parsed
	if (parse->isDefaultParse()) {
			return _new PropositionSet(mentionSet);
	}

	return getPropositionTheory(parse->getRoot(), mentionSet);
		
}

PropositionSet *PropositionFinder::getPropositionTheory(const SynNode* root, const MentionSet *mentionSet) {

#ifdef PROP_DEBUG
	ofstream debug;
	debug.open(PROP_DEBUG, ios_base::app);
	debug << "---\nSentence " << _sentence_number << ":\n";
#endif

	// build initial semantic tree
	SemNode *semTree = _semTreeBuilder->buildSemTree(root, mentionSet);

#ifdef PROP_DEBUG
	debug << "Initial tree:\n"
		  << *semTree << "\n";
	debug.flush();
#endif

	// if result is null, produce a warning and return an empty prop set 
	if (semTree == 0) {
		SessionLogger::warn("sem_tree") << "Could not build semantic tree for parse:\n"
			   << root->toString(2).c_str() << "\n";

		return _new PropositionSet(mentionSet);
	}
	if (!semTree->verify(true)) { 
		std::ostringstream ostr;
		ostr << *semTree << "\n"; 
		SessionLogger::info("SERIF") << ostr.str();
		throw; 
	}

	// Simplification pass
	// 1. Non-root branches whose sole child is a branch get collapsed.
	//
	//   (Branch1) {
	//     (Branch2) { ... }}
	//
	//   (Branch2) { ... }
	// 
	// 2. Degenerate links with no head symbol get collapsed.
	//
	//   (Link) {
	//     -symbol: ""
	//     (Branch) { ... }}
	//
	//   (Branch) { ... }
	//
	// 3. Appositive mentions are replaced with their most name-like child.
	//
	//   (Mention1) {
	//     (Mention2) { ... }
	//     (Mention3) { ... }}
	//
	//   (Mention2) { ... }
	semTree->simplify();
	if (!semTree->verify(true)) { 
		ostringstream ostr;
		ostr << *semTree << "\n"; 
		SessionLogger::info("SERIF") << ostr.str();
		throw; 
	}

#ifdef PROP_DEBUG
	debug << "After simplify():\n"
		  << *semTree << "\n";
	debug.flush();
#endif

	// Move links around
	// 1. Branches with a verb or compound OPP get any child links moved under the OPP.
	//   
	//   (Branch) {
	//     (Link1) { ... }
	//     (Link2) { ... }
	//     (OPP) { ... }}
	//
	//   (Branch) {
	//     (OPP) {
	//       (Link1) { ... }
	//       (Link2) { ... }}}
	//
	// 2. Links under mentions look for a noun OPP to attach to.
	//    If sole child of a branch or no noun OPP found, then OPP gets created with link as child.
	//
	//   (Mention) {
	//     (Link) { ... }
	//     (OPP) {
	//       -type: noun 
	//     }}
	//
	//   (Mention) {
	//     (OPP) {
	//       -type: noun
	//       (Link) { ... }
	//     }}
	semTree->fixLinks();
#ifdef PROP_DEBUG
	debug << "After fixLinks():\n"
		  << *semTree << "\n";
	debug.flush();
#endif
	
	if (!semTree->verify(true)) { 
		std::ostringstream ostr;
		ostr << *semTree << "\n"; 
		SessionLogger::info("SERIF") << ostr.str();
		throw; 
	}

	// Move from list-of-children representation to statically defined child slots
	// OPPs have _links, _arg1, and _arg2 filled with their children.
	// Mentions have _opps and _branch populated.
	// Links have _object set to their last mention/branch child.
	// Branches have _ref set to their last pre-opp child that is a reference.  _opp is set to the last non-reference OPP child.
	//
	// Tangentials get set to false for things that "participate".
	semTree->regularize();

#ifdef PROP_DEBUG
	debug << "After regularization:\n"
		  << *semTree << "\n";
	debug.flush();
#endif

	// Create and resolve traces
	// Find subjects for verbs without traces (a.k.a. a Branch with an _opp but no _ref)
	semTree->createTraces();
#ifdef PROP_DEBUG
	debug << "Final tree:\n"
		  << *semTree << "\n";
	debug.flush();
#endif

	// creates propositions
	IDGenerator propIDGenerator(_sentence_number*MAX_SENTENCE_PROPS);
	semTree->createPropositions(propIDGenerator);


	PropositionSet *result = _new PropositionSet(mentionSet);
	semTree->listPropositions(*result);

	SemNode::deallocateAllSemNodes();

	_linearPropositionFinder->augmentPropositionTheory(result, root, mentionSet);

#ifdef PROP_DEBUG
	debug.close();
#endif

	return result;
}
